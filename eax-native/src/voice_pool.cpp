#include "voice_pool.h"
#include "context.h"
#include "spatializer.h"
#include <cstring>
#include <cmath>

void eax_buffer_s::init_openal(eax_context_s* ctx) noexcept {
    context = ctx;
    if (al_source == 0) {
        alGenSources(1, &al_source);
    }
    if (al_buffer == 0) {
        alGenBuffers(1, &al_buffer);
    }

    auto& ext = eax::get_al_ext();
    if (ext.has_efx && ext.alGenFilters) {
        if (al_direct_filter == 0) ext.alGenFilters(1, &al_direct_filter);
        if (al_send_filter == 0) ext.alGenFilters(1, &al_send_filter);
    }
}

void eax_buffer_s::destroy_openal() noexcept {
    auto& ext = eax::get_al_ext();
    if (al_source != 0) {
        alSourceStop(al_source);
        alDeleteSources(1, &al_source);
        al_source = 0;
    }
    if (al_buffer != 0) {
        alDeleteBuffers(1, &al_buffer);
        al_buffer = 0;
    }
    if (al_direct_filter != 0 && ext.alDeleteFilters) {
        ext.alDeleteFilters(1, &al_direct_filter);
        al_direct_filter = 0;
    }
    if (al_send_filter != 0 && ext.alDeleteFilters) {
        ext.alDeleteFilters(1, &al_send_filter);
        al_send_filter = 0;
    }
}

void eax_buffer_s::reset() noexcept {
    caps = {};
    format = {};
    volume_mb.store(0, std::memory_order_relaxed);
    pan_mb.store(0, std::memory_order_relaxed);
    frequency_hz.store(44100, std::memory_order_relaxed);
    status.store(0, std::memory_order_relaxed);
    is_looping.store(false, std::memory_order_relaxed);
    playback_cursor.store(0.0, std::memory_order_relaxed);

    ds3d_seqlock.write_begin();
    ds3d = {};
    ds3d.dwSize = sizeof(eax_ds3dbuffer_t);
    ds3d.flMinDistance = 1.0f;
    ds3d.flMaxDistance = 1000.0f;
    ds3d.dwInsideConeAngle = 360;
    ds3d.dwOutsideConeAngle = 360;
    ds3d.dwMode = EAX_DS3DMODE_NORMAL;
    ds3d_seqlock.write_end();
    ds3d_deferred = ds3d;
    ds3d_has_deferred = false;

    eax20 = {};
    eax20_deferred = {};
    eax20_has_deferred = false;

    if (al_source != 0) {
        alSourceStop(al_source);
        alSourcei(al_source, AL_BUFFER, 0);
    }
}

void eax_buffer_s::upload_sample_data() noexcept {
    if (al_buffer == 0 || !sample_buffer.data() || sample_buffer.capacity() == 0 || format.nSamplesPerSec == 0) {
        return;
    }

    ALenum al_fmt = AL_FORMAT_MONO16;
    if (format.wBitsPerSample == 16) {
        al_fmt = (format.nChannels == 2) ? AL_FORMAT_STEREO16 : AL_FORMAT_MONO16;
    } else {
        al_fmt = (format.nChannels == 2) ? AL_FORMAT_STEREO8 : AL_FORMAT_MONO8;
    }

    alBufferData(al_buffer, al_fmt, sample_buffer.data(),
                 static_cast<ALsizei>(sample_buffer.capacity()),
                 static_cast<ALsizei>(format.nSamplesPerSec));

    if (al_source != 0) {
        alSourcei(al_source, AL_BUFFER, al_buffer);
    }
}

void eax_buffer_s::sync_to_openal() noexcept {
    if (al_source == 0) return;

    alSourcef(al_source, AL_GAIN, eax::mb_to_gain(volume_mb.load(std::memory_order_relaxed)));

    const uint32_t freq = frequency_hz.load(std::memory_order_relaxed);
    const float pitch = static_cast<float>(freq) / static_cast<float>(format.nSamplesPerSec ? format.nSamplesPerSec : 44100);
    alSourcef(al_source, AL_PITCH, pitch);

    alSourcei(al_source, AL_LOOPING, is_looping.load(std::memory_order_relaxed) ? AL_TRUE : AL_FALSE);

    const bool is_3d = (caps.dwFlags & EAX_DSBCAPS_CTRL3D) != 0;
    if (is_3d) {
        const auto snap = get_ds3d_snapshot();
        // DirectSound (+X right, +Y up, +Z forward) -> OpenAL (+X right, +Y up, -Z forward)
        alSource3f(al_source, AL_POSITION, snap.vPosition.x, snap.vPosition.y, -snap.vPosition.z);
        alSource3f(al_source, AL_VELOCITY, snap.vVelocity.x, snap.vVelocity.y, -snap.vVelocity.z);
        alSource3f(al_source, AL_DIRECTION, snap.vConeOrientation.x, snap.vConeOrientation.y, -snap.vConeOrientation.z);

        alSourcef(al_source, AL_REFERENCE_DISTANCE, snap.flMinDistance);
        alSourcef(al_source, AL_MAX_DISTANCE, snap.flMaxDistance);
        const float rolloff = context ? context->get_listener_snapshot().flRolloffFactor : 1.0f;
        alSourcef(al_source, AL_ROLLOFF_FACTOR, rolloff);

        alSourcef(al_source, AL_CONE_INNER_ANGLE, static_cast<float>(snap.dwInsideConeAngle));
        alSourcef(al_source, AL_CONE_OUTER_ANGLE, static_cast<float>(snap.dwOutsideConeAngle));
        alSourcef(al_source, AL_CONE_OUTER_GAIN, eax::mb_to_gain(snap.lConeOutsideVolume));

        alSourcei(al_source, AL_SOURCE_RELATIVE, (snap.dwMode != EAX_DS3DMODE_NORMAL) ? AL_TRUE : AL_FALSE);
    } else {
        alSourcef(al_source, AL_ROLLOFF_FACTOR, 0.0f);
        alSourcei(al_source, AL_SOURCE_RELATIVE, AL_TRUE);

        auto& ext = eax::get_al_ext();
        if (ext.has_source_panning) {
            alSourcei(al_source, AL_PANNING_ENABLED_SOFT, AL_TRUE);
            alSourcef(al_source, AL_PAN_SOFT, eax::calculate_panning_soft(pan_mb.load(std::memory_order_relaxed)));
        }
    }

    // EAX EFX Filters (Obstruction & Occlusion)
    auto& ext = eax::get_al_ext();
    if (ext.has_efx && context) {
        if (al_direct_filter != 0 && ext.alFilteri && ext.alFilterf) {
            ext.alFilteri(al_direct_filter, AL_FILTER_TYPE, AL_FILTER_LOWPASS);
            ext.alFilterf(al_direct_filter, AL_LOWPASS_GAIN, 1.0f);
            ext.alFilterf(al_direct_filter, AL_LOWPASS_GAINHF, eax::mb_to_gain(eax20.lObstruction));
            alSourcei(al_source, AL_DIRECT_FILTER, al_direct_filter);
        }

        if (al_send_filter != 0 && ext.alFilteri && ext.alFilterf && context->m_eax_slot != 0) {
            ext.alFilteri(al_send_filter, AL_FILTER_TYPE, AL_FILTER_LOWPASS);
            ext.alFilterf(al_send_filter, AL_LOWPASS_GAIN, eax::mb_to_gain(eax20.lRoom));
            ext.alFilterf(al_send_filter, AL_LOWPASS_GAINHF, eax::mb_to_gain(eax20.lOcclusion));
            alSource3i(al_source, AL_AUXILIARY_SEND_FILTER, context->m_eax_slot, 0, al_send_filter);
        }
    }
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

VoicePool::~VoicePool() noexcept {
    for (auto& voice : m_voices) {
        voice.destroy_openal();
    }
}

void VoicePool::init(eax_context_s* ctx) noexcept {
    m_context = ctx;
    for (auto& voice : m_voices) {
        voice.init_openal(ctx);
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
    return nullptr;
}

void VoicePool::release_voice(eax_buffer_s* voice) noexcept {
    if (!voice) return;
    voice->status.store(0, std::memory_order_release);
    if (voice->al_source != 0) {
        alSourceStop(voice->al_source);
        alSourcei(voice->al_source, AL_BUFFER, 0);
    }
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
