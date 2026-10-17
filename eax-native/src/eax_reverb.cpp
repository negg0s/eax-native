#include "eax_reverb.h"
#include "spatializer.h"
#include <cmath>
#include <algorithm>

namespace eax {

ReverbDSP::ReverbDSP() noexcept {
    m_predelay_buffer.reserve(19201);
    for (auto& comb : m_combs) comb.reserve(19201);
    for (auto& allpass : m_allpasses) allpass.reserve(19201);
    configure(PRESET_GENERIC, 44100);
}

void ReverbDSP::configure(const eax20_listener_properties_t& props, uint32_t sample_rate) noexcept {
    m_sample_rate = sample_rate ? sample_rate : 44100;
    m_room_gain = mb_to_gain(props.lRoom);
    m_reverb_gain = mb_to_gain(props.lReverb);
    m_reflections_gain = mb_to_gain(props.lReflections);
    m_decay_time = std::clamp(props.flDecayTime, 0.1f, 20.0f);

    // Damping factor from decay HF ratio
    const float damping = std::clamp(1.0f - (1.0f / (props.flDecayHFRatio > 0.0f ? props.flDecayHFRatio : 1.0f)), 0.0f, 0.9f);

    // Feedback scalar based on decay time RT60:
    // RT60 = -3 * delay / log10(feedback) => feedback = 10^(-3 * delay / RT60)
    const float sr_scale = static_cast<float>(m_sample_rate) / 44100.0f;

    // Comb delays (in samples at 44.1kHz, scaled by environment size)
    const float size_factor = std::clamp(props.flEnvironmentSize / 7.5f, 0.5f, 2.5f);
    const size_t delays_comb[4] = {
        static_cast<size_t>(1116 * sr_scale * size_factor),
        static_cast<size_t>(1188 * sr_scale * size_factor),
        static_cast<size_t>(1277 * sr_scale * size_factor),
        static_cast<size_t>(1356 * sr_scale * size_factor)
    };

    for (size_t i = 0; i < 4; ++i) {
        const float delay_sec = static_cast<float>(delays_comb[i]) / static_cast<float>(m_sample_rate);
        const float feedback = std::pow(0.001f, delay_sec / m_decay_time);
        m_combs[i].init(delays_comb[i], std::clamp(feedback, 0.1f, 0.98f), damping);
    }

    // Allpass delays
    const size_t delays_allpass[2] = {
        static_cast<size_t>(225 * sr_scale),
        static_cast<size_t>(556 * sr_scale)
    };
    m_allpasses[0].init(delays_allpass[0], 0.5f);
    m_allpasses[1].init(delays_allpass[1], 0.5f);

    // Pre-delay buffer (combining reflections and reverb delay)
    const float predelay_sec = std::clamp(props.flReverbDelay, 0.0f, 0.1f);
    const size_t predelay_samples = static_cast<size_t>(predelay_sec * static_cast<float>(m_sample_rate)) + 1;
    m_predelay_buffer.assign(predelay_samples, 0.0f);
    m_predelay_idx = 0;
}

void ReverbDSP::process(float input_mono, float& out_left, float& out_right) noexcept {
    if (m_room_gain <= 0.00001f || m_predelay_buffer.empty()) {
        out_left = 0.0f;
        out_right = 0.0f;
        return;
    }

    // Read delayed sample from predelay buffer
    const float delayed_input = m_predelay_buffer[m_predelay_idx];
    m_predelay_buffer[m_predelay_idx] = input_mono;
    m_predelay_idx = (m_predelay_idx + 1) % m_predelay_buffer.size();

    // Sum through 4 parallel comb filters
    float comb_sum = 0.0f;
    for (size_t i = 0; i < 4; ++i) {
        comb_sum += m_combs[i].process(delayed_input);
    }
    comb_sum *= 0.25f;

    // Run through 2 cascaded allpass filters for diffusion
    float diffused = comb_sum;
    for (size_t i = 0; i < 2; ++i) {
        diffused = m_allpasses[i].process(diffused);
    }

    // Combine early reflections and diffuse reverb
    const float wet = diffused * m_room_gain * m_reverb_gain;
    const float early = delayed_input * m_room_gain * m_reflections_gain;

    out_left = early * 0.7f + wet;
    out_right = early * 0.3f + wet * 0.95f; // subtle decorrelation
}

void ReverbDSP::reset() noexcept {
    std::fill(m_predelay_buffer.begin(), m_predelay_buffer.end(), 0.0f);
    m_predelay_idx = 0;
}

} // namespace eax
