#include "context.h"
#include <algorithm>
#include <vector>

eax_context_s::eax_context_s() noexcept {
    listener.dwSize = sizeof(eax_ds3dlistener_t);
    listener.vPosition = { 0.0f, 0.0f, 0.0f };
    listener.vVelocity = { 0.0f, 0.0f, 0.0f };
    listener.vOrientFront = { 0.0f, 0.0f, 1.0f }; // DS3D +Z forward
    listener.vOrientTop = { 0.0f, 1.0f, 0.0f };
    listener.flDistanceFactor = 1.0f;
    listener.flRolloffFactor = 1.0f;
    listener.flDopplerFactor = 1.0f;
    listener_deferred = listener;
    listener_has_deferred = false;

    eax_listener = eax::PRESET_GENERIC;
    eax_listener_deferred = eax_listener;
    eax_listener_has_deferred = false;
    reverb_dsp.configure(eax_listener, 44100);
}

void eax_context_s::commit_deferred_settings() noexcept {
    std::lock_guard<std::mutex> lock(config_mutex);
    if (listener_has_deferred) {
        listener = listener_deferred;
        listener_has_deferred = false;
    }
    if (eax_listener_has_deferred) {
        eax_listener = eax_listener_deferred;
        eax_listener_has_deferred = false;
        reverb_dsp.configure(eax_listener, 44100);
    }

    // Apply deferred buffer settings across all voices
    for (auto& voice : voice_pool.get_all_voices()) {
        if (!voice.in_use.load(std::memory_order_relaxed)) continue;
        if (voice.ds3d_has_deferred) {
            voice.ds3d = voice.ds3d_deferred;
            voice.ds3d_has_deferred = false;
        }
        if (voice.eax20_has_deferred) {
            voice.eax20 = voice.eax20_deferred;
            voice.eax20_has_deferred = false;
        }
    }
}

void eax_context_s::render_offline(float* out_stereo_pcm, size_t frame_count, uint32_t sample_rate) noexcept {
    if (!out_stereo_pcm || frame_count == 0) return;

    // Zero output
    std::fill_n(out_stereo_pcm, frame_count * 2, 0.0f);

    std::vector<float> voice_l(frame_count, 0.0f);
    std::vector<float> voice_r(frame_count, 0.0f);
    std::vector<float> reverb_send(frame_count, 0.0f);

    for (auto& voice : voice_pool.get_all_voices()) {
        if (!voice.in_use.load(std::memory_order_relaxed)) continue;
        if ((voice.status.load(std::memory_order_relaxed) & EAX_DSBSTATUS_PLAYING) == 0) continue;

        voice.read_samples(voice_l.data(), voice_r.data(), frame_count, sample_rate);

        const bool is_3d = (voice.caps.dwFlags & EAX_DSBCAPS_CTRL3D) != 0;
        eax::SpatialGains gains;

        float p_left = 1.0f;
        float p_right = 1.0f;

        if (is_3d) {
            gains = eax::Spatializer::compute_gains(listener, voice.ds3d, voice.volume_mb.load(std::memory_order_relaxed));
            p_left = gains.left_gain;
            p_right = gains.right_gain;
        } else {
            const float vol = eax::mb_to_gain(voice.volume_mb.load(std::memory_order_relaxed));
            const float pan = eax::calculate_panning_soft(voice.pan_mb.load(std::memory_order_relaxed));
            // Constant-power panning law
            p_left = vol * std::sqrt(0.5f * (1.0f - pan));
            p_right = vol * std::sqrt(0.5f * (1.0f + pan));
        }

        for (size_t i = 0; i < frame_count; ++i) {
            const float sample_mono = 0.5f * (voice_l[i] + voice_r[i]);
            out_stereo_pcm[2 * i] += voice_l[i] * p_left;
            out_stereo_pcm[2 * i + 1] += voice_r[i] * p_right;

            // Reverb send (scaled by distance attenuation if 3D)
            const float send_gain = is_3d ? gains.distance_gain : 1.0f;
            reverb_send[i] += sample_mono * send_gain;
        }
    }

    // Process Environmental Reverb
    for (size_t i = 0; i < frame_count; ++i) {
        float rev_l = 0.0f;
        float rev_r = 0.0f;
        reverb_dsp.process(reverb_send[i], rev_l, rev_r);

        out_stereo_pcm[2 * i] += rev_l;
        out_stereo_pcm[2 * i + 1] += rev_r;
    }
}
