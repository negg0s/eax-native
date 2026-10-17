#include "voice_pool.h"
#include <cstring>
#include <cmath>

void eax_buffer_s::reset() noexcept {
    caps = {};
    format = {};
    volume_mb.store(0, std::memory_order_relaxed);
    pan_mb.store(0, std::memory_order_relaxed);
    frequency_hz.store(44100, std::memory_order_relaxed);
    status.store(0, std::memory_order_relaxed);
    is_looping.store(false, std::memory_order_relaxed);
    playback_cursor.store(0.0, std::memory_order_relaxed);

    ds3d = {};
    ds3d.dwSize = sizeof(eax_ds3dbuffer_t);
    ds3d.flMinDistance = 1.0f;
    ds3d.flMaxDistance = 1000.0f;
    ds3d.dwInsideConeAngle = 360;
    ds3d.dwOutsideConeAngle = 360;
    ds3d.dwMode = EAX_DS3DMODE_NORMAL;
    ds3d_deferred = ds3d;
    ds3d_has_deferred = false;

    eax20 = {};
    eax20_deferred = {};
    eax20_has_deferred = false;
}

void eax_buffer_s::read_samples(float* out_left, float* out_right, size_t frame_count, uint32_t output_sample_rate) noexcept {
    if ((status.load(std::memory_order_acquire) & EAX_DSBSTATUS_PLAYING) == 0) {
        std::fill_n(out_left, frame_count, 0.0f);
        std::fill_n(out_right, frame_count, 0.0f);
        return;
    }

    const uint8_t* pcm_data = sample_buffer.data();
    const size_t total_bytes = sample_buffer.capacity();
    if (!pcm_data || total_bytes == 0 || format.nBlockAlign == 0 || format.nSamplesPerSec == 0) {
        std::fill_n(out_left, frame_count, 0.0f);
        std::fill_n(out_right, frame_count, 0.0f);
        return;
    }

    const size_t total_frames = total_bytes / format.nBlockAlign;
    if (total_frames == 0) {
        std::fill_n(out_left, frame_count, 0.0f);
        std::fill_n(out_right, frame_count, 0.0f);
        return;
    }

    const uint32_t current_freq = frequency_hz.load(std::memory_order_relaxed);
    const double pitch_ratio = static_cast<double>(current_freq) / static_cast<double>(output_sample_rate ? output_sample_rate : 44100);
    double cursor = playback_cursor.load(std::memory_order_relaxed);
    const bool looping = is_looping.load(std::memory_order_relaxed);

    const bool is_16bit = (format.wBitsPerSample == 16);
    const bool is_stereo = (format.nChannels == 2);

    for (size_t i = 0; i < frame_count; ++i) {
        if (cursor >= static_cast<double>(total_frames)) {
            if (looping) {
                cursor = std::fmod(cursor, static_cast<double>(total_frames));
            } else {
                // Buffer finished playback
                status.store(status.load(std::memory_order_relaxed) & ~EAX_DSBSTATUS_PLAYING, std::memory_order_release);
                for (; i < frame_count; ++i) {
                    out_left[i] = 0.0f;
                    out_right[i] = 0.0f;
                }
                break;
            }
        }

        const size_t frame_idx = static_cast<size_t>(cursor) % total_frames;
        const size_t byte_offset = frame_idx * format.nBlockAlign;

        float s_l = 0.0f;
        float s_r = 0.0f;

        if (is_16bit) {
            const auto* s16 = reinterpret_cast<const int16_t*>(pcm_data + byte_offset);
            if (is_stereo) {
                s_l = static_cast<float>(s16[0]) / 32768.0f;
                s_r = static_cast<float>(s16[1]) / 32768.0f;
            } else {
                s_l = static_cast<float>(s16[0]) / 32768.0f;
                s_r = s_l;
            }
        } else {
            // 8-bit unsigned PCM [0..255], center at 128
            const uint8_t* s8 = pcm_data + byte_offset;
            if (is_stereo) {
                s_l = (static_cast<float>(s8[0]) - 128.0f) / 128.0f;
                s_r = (static_cast<float>(s8[1]) - 128.0f) / 128.0f;
            } else {
                s_l = (static_cast<float>(s8[0]) - 128.0f) / 128.0f;
                s_r = s_l;
            }
        }

        out_left[i] = s_l;
        out_right[i] = s_r;

        cursor += pitch_ratio;
    }

    playback_cursor.store(cursor, std::memory_order_relaxed);
    sample_buffer.set_play_pos(static_cast<size_t>(cursor) * format.nBlockAlign);
}

namespace eax {

VoicePool::VoicePool() noexcept {
    for (uint32_t i = 0; i < MAX_VOICES; ++i) {
        m_voices[i].id = i;
        m_voices[i].in_use.store(false, std::memory_order_relaxed);
    }
}

eax_buffer_s* VoicePool::allocate_voice() noexcept {
    for (auto& voice : m_voices) {
        bool expected = false;
        if (voice.in_use.compare_exchange_strong(
                expected, true, std::memory_order_acq_rel, std::memory_order_relaxed)) {
            voice.reset();
            return &voice;
        }
    }
    return nullptr; // No free voices
}

void VoicePool::release_voice(eax_buffer_s* voice) noexcept {
    if (!voice) return;
    voice->status.store(0, std::memory_order_release);
    voice->in_use.store(false, std::memory_order_release);
}

size_t VoicePool::active_voice_count() const noexcept {
    size_t count = 0;
    for (const auto& voice : m_voices) {
        if (voice.in_use.load(std::memory_order_relaxed)) {
            ++count;
        }
    }
    return count;
}

} // namespace eax
