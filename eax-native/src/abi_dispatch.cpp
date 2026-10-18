#include "eax_native_abi.h"
#include "context.h"
#include <cstring>
#include <new>

#ifndef AL_PANNING_ENABLED_SOFT
#define AL_PANNING_ENABLED_SOFT 0x19EC
#endif
#ifndef AL_PAN_SOFT
#define AL_PAN_SOFT 0x19ED
#endif

extern "C" {

EAX_API eax_result_t eax_context_create(eax_context_h* out_context) {
    if (!out_context) return EAX_ERR_POINTER;
    auto* ctx = new (std::nothrow) eax_context_s();
    if (!ctx) return EAX_ERR_OUT_OF_MEMORY;
    *out_context = ctx;
    return EAX_OK;
}

EAX_API eax_result_t eax_context_create_offline(eax_context_h* out_context, uint32_t sample_rate) {
    if (!out_context) return EAX_ERR_POINTER;
    auto* ctx = new (std::nothrow) eax_context_s(true, sample_rate ? sample_rate : 44100);
    if (!ctx) return EAX_ERR_OUT_OF_MEMORY;
    *out_context = ctx;
    return EAX_OK;
}

EAX_API eax_result_t eax_context_destroy(eax_context_h context) {
    if (!context) return EAX_ERR_POINTER;
    delete context;
    return EAX_OK;
}

EAX_API eax_result_t eax_context_get_caps(eax_context_h context, eax_dscaps_t* out_caps) {
    if (!context || !out_caps) return EAX_ERR_POINTER;
    std::memset(out_caps, 0, sizeof(eax_dscaps_t));
    out_caps->dwSize = sizeof(eax_dscaps_t);
    out_caps->dwFlags = 0x00000001 /* DSCAPS_PRIMARYMONO */ |
                        0x00000002 /* DSCAPS_PRIMARYSTEREO */ |
                        0x00000008 /* DSCAPS_PRIMARY16BIT */ |
                        0x00000040 /* DSCAPS_SECONDARYMONO */ |
                        0x00000080 /* DSCAPS_SECONDARYSTEREO */ |
                        0x00000200 /* DSCAPS_SECONDARY16BIT */ |
                        0x00000800 /* DSCAPS_HARDWARE (EAX accelerated) */;
    out_caps->dwMinSecondarySampleRate = 4000;
    out_caps->dwMaxSecondarySampleRate = 192000;
    out_caps->dwPrimaryBuffers = 1;
    out_caps->dwMaxHwMixingAllBuffers = static_cast<uint32_t>(eax::VoicePool::MAX_VOICES);
    out_caps->dwMaxHwMixingStaticBuffers = static_cast<uint32_t>(eax::VoicePool::MAX_VOICES);
    out_caps->dwMaxHwMixingStreamingBuffers = static_cast<uint32_t>(eax::VoicePool::MAX_VOICES);
    out_caps->dwFreeHwMixingAllBuffers = static_cast<uint32_t>(eax::VoicePool::MAX_VOICES - context->voice_pool.active_voice_count());
    out_caps->dwMaxHw3DAllBuffers = static_cast<uint32_t>(eax::VoicePool::MAX_VOICES);
    out_caps->dwMaxHw3DStaticBuffers = static_cast<uint32_t>(eax::VoicePool::MAX_VOICES);
    out_caps->dwMaxHw3DStreamingBuffers = static_cast<uint32_t>(eax::VoicePool::MAX_VOICES);
    out_caps->dwFreeHw3DAllBuffers = out_caps->dwFreeHwMixingAllBuffers;
    return EAX_OK;
}

EAX_API eax_result_t eax_context_get_speaker_config(eax_context_h context, uint32_t* out_speaker_config) {
    if (!context || !out_speaker_config) return EAX_ERR_POINTER;
    *out_speaker_config = context->speaker_config;
    return EAX_OK;
}

EAX_API eax_result_t eax_context_set_speaker_config(eax_context_h context, uint32_t speaker_config) {
    if (!context) return EAX_ERR_POINTER;
    context->speaker_config = speaker_config;
    return EAX_OK;
}

EAX_API eax_result_t eax_context_render_offline(eax_context_h context, float* out_stereo_pcm, size_t frame_count, uint32_t sample_rate) {
    if (!context || !out_stereo_pcm) return EAX_ERR_POINTER;
    context->render_offline(out_stereo_pcm, frame_count, sample_rate);
    return EAX_OK;
}

/* ---------------- Listener ---------------- */

EAX_API eax_result_t eax_listener_get_all_parameters(eax_context_h context, eax_ds3dlistener_t* out_params) {
    if (!context || !out_params) return EAX_ERR_POINTER;
    *out_params = context->get_listener_snapshot();
    return EAX_OK;
}

EAX_API eax_result_t eax_listener_set_all_parameters(eax_context_h context, const eax_ds3dlistener_t* params, uint32_t apply_mode) {
    if (!context || !params) return EAX_ERR_POINTER;
    if (apply_mode == EAX_DS3D_DEFERRED) {
        context->listener_deferred = *params;
        context->listener_has_deferred = true;
    } else {
        context->listener_seqlock.write_begin();
        context->listener = *params;
        context->listener_seqlock.write_end();
        context->sync_listener_to_openal();
    }
    return EAX_OK;
}

EAX_API eax_result_t eax_listener_get_position(eax_context_h context, eax_vector_t* out_pos) {
    if (!context || !out_pos) return EAX_ERR_POINTER;
    *out_pos = context->get_listener_snapshot().vPosition;
    return EAX_OK;
}

EAX_API eax_result_t eax_listener_set_position(eax_context_h context, float x, float y, float z, uint32_t apply_mode) {
    if (!context) return EAX_ERR_POINTER;
    if (apply_mode == EAX_DS3D_DEFERRED) {
        context->listener_deferred.vPosition = { x, y, z };
        context->listener_has_deferred = true;
    } else {
        context->listener_seqlock.write_begin();
        context->listener.vPosition = { x, y, z };
        context->listener_seqlock.write_end();
        context->sync_listener_to_openal();
    }
    return EAX_OK;
}

EAX_API eax_result_t eax_listener_get_velocity(eax_context_h context, eax_vector_t* out_vel) {
    if (!context || !out_vel) return EAX_ERR_POINTER;
    *out_vel = context->get_listener_snapshot().vVelocity;
    return EAX_OK;
}

EAX_API eax_result_t eax_listener_set_velocity(eax_context_h context, float x, float y, float z, uint32_t apply_mode) {
    if (!context) return EAX_ERR_POINTER;
    if (apply_mode == EAX_DS3D_DEFERRED) {
        context->listener_deferred.vVelocity = { x, y, z };
        context->listener_has_deferred = true;
    } else {
        context->listener_seqlock.write_begin();
        context->listener.vVelocity = { x, y, z };
        context->listener_seqlock.write_end();
        context->sync_listener_to_openal();
    }
    return EAX_OK;
}

EAX_API eax_result_t eax_listener_get_orientation(eax_context_h context, eax_vector_t* out_front, eax_vector_t* out_top) {
    if (!context) return EAX_ERR_POINTER;
    const auto snap = context->get_listener_snapshot();
    if (out_front) *out_front = snap.vOrientFront;
    if (out_top) *out_top = snap.vOrientTop;
    return EAX_OK;
}

EAX_API eax_result_t eax_listener_set_orientation(eax_context_h context, float fx, float fy, float fz, float tx, float ty, float tz, uint32_t apply_mode) {
    if (!context) return EAX_ERR_POINTER;
    if (apply_mode == EAX_DS3D_DEFERRED) {
        context->listener_deferred.vOrientFront = { fx, fy, fz };
        context->listener_deferred.vOrientTop = { tx, ty, tz };
        context->listener_has_deferred = true;
    } else {
        context->listener_seqlock.write_begin();
        context->listener.vOrientFront = { fx, fy, fz };
        context->listener.vOrientTop = { tx, ty, tz };
        context->listener_seqlock.write_end();
        context->sync_listener_to_openal();
    }
    return EAX_OK;
}

EAX_API eax_result_t eax_listener_get_distance_factor(eax_context_h context, float* out_factor) {
    if (!context || !out_factor) return EAX_ERR_POINTER;
    *out_factor = context->get_listener_snapshot().flDistanceFactor;
    return EAX_OK;
}

EAX_API eax_result_t eax_listener_set_distance_factor(eax_context_h context, float factor, uint32_t apply_mode) {
    if (!context || factor <= 0.0f) return EAX_ERR_INVALID_PARAM;
    if (apply_mode == EAX_DS3D_DEFERRED) {
        context->listener_deferred.flDistanceFactor = factor;
        context->listener_has_deferred = true;
    } else {
        context->listener_seqlock.write_begin();
        context->listener.flDistanceFactor = factor;
        context->listener_seqlock.write_end();
        context->sync_listener_to_openal();
    }
    return EAX_OK;
}

EAX_API eax_result_t eax_listener_get_rolloff_factor(eax_context_h context, float* out_factor) {
    if (!context || !out_factor) return EAX_ERR_POINTER;
    *out_factor = context->get_listener_snapshot().flRolloffFactor;
    return EAX_OK;
}

EAX_API eax_result_t eax_listener_set_rolloff_factor(eax_context_h context, float factor, uint32_t apply_mode) {
    if (!context || factor < 0.0f) return EAX_ERR_INVALID_PARAM;
    if (apply_mode == EAX_DS3D_DEFERRED) {
        context->listener_deferred.flRolloffFactor = factor;
        context->listener_has_deferred = true;
    } else {
        context->listener_seqlock.write_begin();
        context->listener.flRolloffFactor = factor;
        context->listener_seqlock.write_end();
        context->sync_listener_to_openal();
    }
    return EAX_OK;
}

EAX_API eax_result_t eax_listener_get_doppler_factor(eax_context_h context, float* out_factor) {
    if (!context || !out_factor) return EAX_ERR_POINTER;
    *out_factor = context->get_listener_snapshot().flDopplerFactor;
    return EAX_OK;
}

EAX_API eax_result_t eax_listener_set_doppler_factor(eax_context_h context, float factor, uint32_t apply_mode) {
    if (!context || factor < 0.0f) return EAX_ERR_INVALID_PARAM;
    if (apply_mode == EAX_DS3D_DEFERRED) {
        context->listener_deferred.flDopplerFactor = factor;
        context->listener_has_deferred = true;
    } else {
        context->listener_seqlock.write_begin();
        context->listener.flDopplerFactor = factor;
        context->listener_seqlock.write_end();
        context->sync_listener_to_openal();
    }
    return EAX_OK;
}

EAX_API eax_result_t eax_listener_commit_deferred(eax_context_h context) {
    if (!context) return EAX_ERR_POINTER;
    context->commit_deferred_settings();
    return EAX_OK;
}

/* ---------------- Buffer Management ---------------- */

EAX_API eax_result_t eax_buffer_create(eax_context_h context, const eax_dsbcaps_t* caps, const eax_waveformat_ex_t* format, eax_buffer_h* out_buffer) {
    if (!context || !caps || !format || !out_buffer) return EAX_ERR_POINTER;

    auto* voice = context->voice_pool.allocate_voice();
    if (!voice) return EAX_ERR_OUT_OF_MEMORY;

    voice->caps = *caps;
    voice->format = *format;
    voice->frequency_hz.store(format->nSamplesPerSec, std::memory_order_relaxed);

    void* mem = context->arena.allocate(caps->dwBufferBytes);
    if (!mem && caps->dwBufferBytes > 0) {
        context->voice_pool.release_voice(voice);
        return EAX_ERR_OUT_OF_MEMORY;
    }
    voice->sample_buffer.initialize(mem, caps->dwBufferBytes);
    voice->owns_arena_memory = (mem != nullptr);
    voice->init_openal(context);

    *out_buffer = voice;
    return EAX_OK;
}

EAX_API eax_result_t eax_buffer_destroy(eax_buffer_h buffer) {
    if (!buffer) return EAX_ERR_POINTER;
    buffer->status.store(0, std::memory_order_release);
    if (buffer->al_source != 0) {
        alSourceStop(buffer->al_source);
        alSourcei(buffer->al_source, AL_BUFFER, 0);
    }
    if (buffer->context && buffer->owns_arena_memory && buffer->sample_buffer.data()) {
        buffer->context->arena.free(buffer->sample_buffer.data(), buffer->sample_buffer.capacity());
    }
    buffer->owns_arena_memory = false;
    buffer->sample_buffer.initialize(nullptr, 0);
    buffer->in_use.store(false, std::memory_order_release);
    return EAX_OK;
}

EAX_API eax_result_t eax_buffer_duplicate(eax_context_h context, eax_buffer_h source_buffer, eax_buffer_h* out_buffer) {
    if (!context || !source_buffer || !out_buffer) return EAX_ERR_POINTER;

    auto* voice = context->voice_pool.allocate_voice();
    if (!voice) return EAX_ERR_OUT_OF_MEMORY;

    voice->caps = source_buffer->caps;
    voice->format = source_buffer->format;
    voice->volume_mb.store(source_buffer->volume_mb.load(std::memory_order_relaxed), std::memory_order_relaxed);
    voice->pan_mb.store(source_buffer->pan_mb.load(std::memory_order_relaxed), std::memory_order_relaxed);
    voice->frequency_hz.store(source_buffer->frequency_hz.load(std::memory_order_relaxed), std::memory_order_relaxed);
    voice->ds3d_seqlock.write_begin();
    voice->ds3d = source_buffer->get_ds3d_snapshot();
    voice->ds3d_seqlock.write_end();

    const size_t buffer_bytes = source_buffer->sample_buffer.capacity();
    void* mem = context->arena.allocate(buffer_bytes);
    if (!mem && buffer_bytes > 0) {
        context->voice_pool.release_voice(voice);
        return EAX_ERR_OUT_OF_MEMORY;
    }
    if (mem && buffer_bytes > 0 && source_buffer->sample_buffer.data()) {
        std::memcpy(mem, source_buffer->sample_buffer.data(), buffer_bytes);
    }
    voice->sample_buffer.initialize(mem, buffer_bytes);
    voice->owns_arena_memory = (mem != nullptr);
    voice->init_openal(context);

    if (voice->al_source != 0 && source_buffer->al_buffer != 0) {
        alSourcei(voice->al_source, AL_BUFFER, static_cast<ALint>(source_buffer->al_buffer));
    }
    voice->sync_to_openal();

    *out_buffer = voice;
    return EAX_OK;
}

EAX_API eax_result_t eax_buffer_get_caps(eax_buffer_h buffer, eax_dsbcaps_t* out_caps) {
    if (!buffer || !out_caps) return EAX_ERR_POINTER;
    *out_caps = buffer->caps;
    return EAX_OK;
}

EAX_API eax_result_t eax_buffer_get_format(eax_buffer_h buffer, eax_waveformat_ex_t* out_format) {
    if (!buffer || !out_format) return EAX_ERR_POINTER;
    *out_format = buffer->format;
    return EAX_OK;
}

EAX_API eax_result_t eax_buffer_lock(eax_buffer_h buffer, uint32_t offset, uint32_t bytes, void** ptr1, uint32_t* bytes1, void** ptr2, uint32_t* bytes2, uint32_t /*flags*/) {
    if (!buffer) return EAX_ERR_POINTER;
    if (!buffer->sample_buffer.lock_region(offset, bytes, ptr1, bytes1, ptr2, bytes2)) {
        return EAX_ERR_INVALID_PARAM;
    }
    return EAX_OK;
}

EAX_API eax_result_t eax_buffer_unlock(eax_buffer_h buffer, void* /*ptr1*/, uint32_t /*bytes1*/, void* /*ptr2*/, uint32_t /*bytes2*/) {
    if (!buffer) return EAX_ERR_POINTER;
    buffer->upload_sample_data();
    buffer->sync_to_openal();
    return EAX_OK;
}

EAX_API eax_result_t eax_buffer_play(eax_buffer_h buffer, uint32_t /*priority*/, uint32_t flags) {
    if (!buffer) return EAX_ERR_POINTER;
    const bool looping = (flags & EAX_DSBPLAY_LOOPING) != 0;
    buffer->is_looping.store(looping, std::memory_order_relaxed);

    uint32_t st = EAX_DSBSTATUS_PLAYING;
    if (looping) st |= EAX_DSBSTATUS_LOOPING;
    buffer->status.store(st, std::memory_order_release);

    if (buffer->al_source != 0) {
        alSourcei(buffer->al_source, AL_LOOPING, looping ? AL_TRUE : AL_FALSE);
        alSourcePlay(buffer->al_source);
    }
    return EAX_OK;
}

EAX_API eax_result_t eax_buffer_stop(eax_buffer_h buffer) {
    if (!buffer) return EAX_ERR_POINTER;
    buffer->status.store(0, std::memory_order_release);
    if (buffer->al_source != 0) {
        alSourceStop(buffer->al_source);
    }
    return EAX_OK;
}

EAX_API eax_result_t eax_buffer_get_status(eax_buffer_h buffer, uint32_t* out_status) {
    if (!buffer || !out_status) return EAX_ERR_POINTER;
    if (buffer->al_source != 0) {
        ALint state = 0;
        alGetSourcei(buffer->al_source, AL_SOURCE_STATE, &state);
        uint32_t st = 0;
        if (state == AL_PLAYING) {
            st |= EAX_DSBSTATUS_PLAYING;
            if (buffer->is_looping.load(std::memory_order_relaxed)) st |= EAX_DSBSTATUS_LOOPING;
        }
        buffer->status.store(st, std::memory_order_relaxed);
    }
    *out_status = buffer->status.load(std::memory_order_acquire);
    return EAX_OK;
}

EAX_API eax_result_t eax_buffer_get_current_position(eax_buffer_h buffer, uint32_t* out_play_pos, uint32_t* out_write_pos) {
    if (!buffer) return EAX_ERR_POINTER;
    if (buffer->al_source != 0) {
        ALint offset = 0;
        alGetSourcei(buffer->al_source, AL_BYTE_OFFSET, &offset);
        if (out_play_pos) *out_play_pos = static_cast<uint32_t>(offset);
        if (out_write_pos) *out_write_pos = static_cast<uint32_t>(offset);
    } else {
        if (out_play_pos) *out_play_pos = static_cast<uint32_t>(buffer->sample_buffer.get_play_pos());
        if (out_write_pos) *out_write_pos = static_cast<uint32_t>(buffer->sample_buffer.get_write_pos());
    }
    return EAX_OK;
}

EAX_API eax_result_t eax_buffer_set_current_position(eax_buffer_h buffer, uint32_t new_pos) {
    if (!buffer) return EAX_ERR_POINTER;
    buffer->sample_buffer.set_play_pos(new_pos);
    if (buffer->format.nBlockAlign > 0) {
        buffer->playback_cursor.store(static_cast<double>(new_pos / buffer->format.nBlockAlign), std::memory_order_relaxed);
    }
    if (buffer->al_source != 0) {
        alSourcei(buffer->al_source, AL_BYTE_OFFSET, static_cast<ALint>(new_pos));
    }
    return EAX_OK;
}

EAX_API eax_result_t eax_buffer_get_volume(eax_buffer_h buffer, int32_t* out_volume_mb) {
    if (!buffer || !out_volume_mb) return EAX_ERR_POINTER;
    *out_volume_mb = buffer->volume_mb.load(std::memory_order_relaxed);
    return EAX_OK;
}

EAX_API eax_result_t eax_buffer_set_volume(eax_buffer_h buffer, int32_t volume_mb) {
    if (!buffer) return EAX_ERR_POINTER;
    buffer->volume_mb.store(volume_mb, std::memory_order_relaxed);
    if (buffer->al_source != 0) {
        alSourcef(buffer->al_source, AL_GAIN, eax::mb_to_gain(volume_mb));
    }
    return EAX_OK;
}

EAX_API eax_result_t eax_buffer_get_pan(eax_buffer_h buffer, int32_t* out_pan_mb) {
    if (!buffer || !out_pan_mb) return EAX_ERR_POINTER;
    *out_pan_mb = buffer->pan_mb.load(std::memory_order_relaxed);
    return EAX_OK;
}

EAX_API eax_result_t eax_buffer_set_pan(eax_buffer_h buffer, int32_t pan_mb) {
    if (!buffer) return EAX_ERR_POINTER;
    buffer->pan_mb.store(pan_mb, std::memory_order_relaxed);
    if (buffer->al_source != 0 && (buffer->caps.dwFlags & EAX_DSBCAPS_CTRL3D) == 0) {
        auto& ext = eax::get_al_ext();
        if (ext.has_source_panning) {
            alSourcei(buffer->al_source, AL_PANNING_ENABLED_SOFT, AL_TRUE);
            alSourcef(buffer->al_source, AL_PAN_SOFT, eax::calculate_panning_soft(pan_mb));
        }
    }
    return EAX_OK;
}

EAX_API eax_result_t eax_buffer_get_frequency(eax_buffer_h buffer, uint32_t* out_freq_hz) {
    if (!buffer || !out_freq_hz) return EAX_ERR_POINTER;
    *out_freq_hz = buffer->frequency_hz.load(std::memory_order_relaxed);
    return EAX_OK;
}

EAX_API eax_result_t eax_buffer_set_frequency(eax_buffer_h buffer, uint32_t freq_hz) {
    if (!buffer || freq_hz == 0) return EAX_ERR_INVALID_PARAM;
    buffer->frequency_hz.store(freq_hz, std::memory_order_relaxed);
    if (buffer->al_source != 0) {
        const float pitch = static_cast<float>(freq_hz) / static_cast<float>(buffer->format.nSamplesPerSec ? buffer->format.nSamplesPerSec : 44100);
        alSourcef(buffer->al_source, AL_PITCH, pitch);
    }
    return EAX_OK;
}

/* ---------------- 3D Buffer Parameters ---------------- */

EAX_API eax_result_t eax_buffer_get_3d_all_parameters(eax_buffer_h buffer, eax_ds3dbuffer_t* out_params) {
    if (!buffer || !out_params) return EAX_ERR_POINTER;
    *out_params = buffer->get_ds3d_snapshot();
    return EAX_OK;
}

EAX_API eax_result_t eax_buffer_set_3d_all_parameters(eax_buffer_h buffer, const eax_ds3dbuffer_t* params, uint32_t apply_mode) {
    if (!buffer || !params) return EAX_ERR_POINTER;
    if (apply_mode == EAX_DS3D_DEFERRED) {
        buffer->ds3d_deferred = *params;
        buffer->ds3d_has_deferred = true;
    } else {
        buffer->ds3d_seqlock.write_begin();
        buffer->ds3d = *params;
        buffer->ds3d_seqlock.write_end();
        buffer->sync_to_openal();
    }
    return EAX_OK;
}

EAX_API eax_result_t eax_buffer_get_3d_position(eax_buffer_h buffer, eax_vector_t* out_pos) {
    if (!buffer || !out_pos) return EAX_ERR_POINTER;
    *out_pos = buffer->get_ds3d_snapshot().vPosition;
    return EAX_OK;
}

EAX_API eax_result_t eax_buffer_set_3d_position(eax_buffer_h buffer, float x, float y, float z, uint32_t apply_mode) {
    if (!buffer) return EAX_ERR_POINTER;
    if (apply_mode == EAX_DS3D_DEFERRED) {
        buffer->ds3d_deferred.vPosition = { x, y, z };
        buffer->ds3d_has_deferred = true;
    } else {
        buffer->ds3d_seqlock.write_begin();
        buffer->ds3d.vPosition = { x, y, z };
        buffer->ds3d_seqlock.write_end();
        buffer->sync_to_openal();
    }
    return EAX_OK;
}

EAX_API eax_result_t eax_buffer_get_3d_velocity(eax_buffer_h buffer, eax_vector_t* out_vel) {
    if (!buffer || !out_vel) return EAX_ERR_POINTER;
    *out_vel = buffer->get_ds3d_snapshot().vVelocity;
    return EAX_OK;
}

EAX_API eax_result_t eax_buffer_set_3d_velocity(eax_buffer_h buffer, float x, float y, float z, uint32_t apply_mode) {
    if (!buffer) return EAX_ERR_POINTER;
    if (apply_mode == EAX_DS3D_DEFERRED) {
        buffer->ds3d_deferred.vVelocity = { x, y, z };
        buffer->ds3d_has_deferred = true;
    } else {
        buffer->ds3d_seqlock.write_begin();
        buffer->ds3d.vVelocity = { x, y, z };
        buffer->ds3d_seqlock.write_end();
        buffer->sync_to_openal();
    }
    return EAX_OK;
}

EAX_API eax_result_t eax_buffer_get_3d_cone_angles(eax_buffer_h buffer, uint32_t* out_inside, uint32_t* out_outside) {
    if (!buffer) return EAX_ERR_POINTER;
    const auto snap = buffer->get_ds3d_snapshot();
    if (out_inside) *out_inside = snap.dwInsideConeAngle;
    if (out_outside) *out_outside = snap.dwOutsideConeAngle;
    return EAX_OK;
}

EAX_API eax_result_t eax_buffer_set_3d_cone_angles(eax_buffer_h buffer, uint32_t inside, uint32_t outside, uint32_t apply_mode) {
    if (!buffer) return EAX_ERR_POINTER;
    if (apply_mode == EAX_DS3D_DEFERRED) {
        buffer->ds3d_deferred.dwInsideConeAngle = inside;
        buffer->ds3d_deferred.dwOutsideConeAngle = outside;
        buffer->ds3d_has_deferred = true;
    } else {
        buffer->ds3d_seqlock.write_begin();
        buffer->ds3d.dwInsideConeAngle = inside;
        buffer->ds3d.dwOutsideConeAngle = outside;
        buffer->ds3d_seqlock.write_end();
        buffer->sync_to_openal();
    }
    return EAX_OK;
}

EAX_API eax_result_t eax_buffer_get_3d_cone_orientation(eax_buffer_h buffer, eax_vector_t* out_orientation) {
    if (!buffer || !out_orientation) return EAX_ERR_POINTER;
    *out_orientation = buffer->get_ds3d_snapshot().vConeOrientation;
    return EAX_OK;
}

EAX_API eax_result_t eax_buffer_set_3d_cone_orientation(eax_buffer_h buffer, float x, float y, float z, uint32_t apply_mode) {
    if (!buffer) return EAX_ERR_POINTER;
    if (apply_mode == EAX_DS3D_DEFERRED) {
        buffer->ds3d_deferred.vConeOrientation = { x, y, z };
        buffer->ds3d_has_deferred = true;
    } else {
        buffer->ds3d_seqlock.write_begin();
        buffer->ds3d.vConeOrientation = { x, y, z };
        buffer->ds3d_seqlock.write_end();
        buffer->sync_to_openal();
    }
    return EAX_OK;
}

EAX_API eax_result_t eax_buffer_get_3d_cone_outside_volume(eax_buffer_h buffer, int32_t* out_volume_mb) {
    if (!buffer || !out_volume_mb) return EAX_ERR_POINTER;
    *out_volume_mb = buffer->get_ds3d_snapshot().lConeOutsideVolume;
    return EAX_OK;
}

EAX_API eax_result_t eax_buffer_set_3d_cone_outside_volume(eax_buffer_h buffer, int32_t volume_mb, uint32_t apply_mode) {
    if (!buffer) return EAX_ERR_POINTER;
    if (apply_mode == EAX_DS3D_DEFERRED) {
        buffer->ds3d_deferred.lConeOutsideVolume = volume_mb;
        buffer->ds3d_has_deferred = true;
    } else {
        buffer->ds3d_seqlock.write_begin();
        buffer->ds3d.lConeOutsideVolume = volume_mb;
        buffer->ds3d_seqlock.write_end();
        buffer->sync_to_openal();
    }
    return EAX_OK;
}

EAX_API eax_result_t eax_buffer_get_3d_min_distance(eax_buffer_h buffer, float* out_dist) {
    if (!buffer || !out_dist) return EAX_ERR_POINTER;
    *out_dist = buffer->get_ds3d_snapshot().flMinDistance;
    return EAX_OK;
}

EAX_API eax_result_t eax_buffer_set_3d_min_distance(eax_buffer_h buffer, float dist, uint32_t apply_mode) {
    if (!buffer || dist <= 0.0f) return EAX_ERR_INVALID_PARAM;
    if (apply_mode == EAX_DS3D_DEFERRED) {
        buffer->ds3d_deferred.flMinDistance = dist;
        buffer->ds3d_has_deferred = true;
    } else {
        buffer->ds3d_seqlock.write_begin();
        buffer->ds3d.flMinDistance = dist;
        buffer->ds3d_seqlock.write_end();
        buffer->sync_to_openal();
    }
    return EAX_OK;
}

EAX_API eax_result_t eax_buffer_get_3d_max_distance(eax_buffer_h buffer, float* out_dist) {
    if (!buffer || !out_dist) return EAX_ERR_POINTER;
    *out_dist = buffer->get_ds3d_snapshot().flMaxDistance;
    return EAX_OK;
}

EAX_API eax_result_t eax_buffer_set_3d_max_distance(eax_buffer_h buffer, float dist, uint32_t apply_mode) {
    if (!buffer || dist <= 0.0f) return EAX_ERR_INVALID_PARAM;
    if (apply_mode == EAX_DS3D_DEFERRED) {
        buffer->ds3d_deferred.flMaxDistance = dist;
        buffer->ds3d_has_deferred = true;
    } else {
        buffer->ds3d_seqlock.write_begin();
        buffer->ds3d.flMaxDistance = dist;
        buffer->ds3d_seqlock.write_end();
        buffer->sync_to_openal();
    }
    return EAX_OK;
}

EAX_API eax_result_t eax_buffer_get_3d_mode(eax_buffer_h buffer, uint32_t* out_mode) {
    if (!buffer || !out_mode) return EAX_ERR_POINTER;
    *out_mode = buffer->get_ds3d_snapshot().dwMode;
    return EAX_OK;
}

EAX_API eax_result_t eax_buffer_set_3d_mode(eax_buffer_h buffer, uint32_t mode, uint32_t apply_mode) {
    if (!buffer) return EAX_ERR_POINTER;
    if (apply_mode == EAX_DS3D_DEFERRED) {
        buffer->ds3d_deferred.dwMode = mode;
        buffer->ds3d_has_deferred = true;
    } else {
        buffer->ds3d_seqlock.write_begin();
        buffer->ds3d.dwMode = mode;
        buffer->ds3d_seqlock.write_end();
        buffer->sync_to_openal();
    }
    return EAX_OK;
}

/* ---------------- EAX Properties (IKsPropertySet) ---------------- */

static bool eax20_listener_prop_get(const eax20_listener_properties_t& p, uint32_t base_id, void* prop_data, uint32_t prop_len, uint32_t* out_len) {
    if (!prop_data || prop_len < 4) return false;
    int32_t as_int = 0;
    float as_float = 0.0f;
    switch (base_id) {
        case 2:  as_int = p.lRoom; goto copy;
        case 3:  as_int = p.lRoomHF; goto copy;
        case 4:  as_float = p.flRoomRolloffFactor; goto copy;
        case 5:  as_float = p.flDecayTime; goto copy;
        case 6:  as_float = p.flDecayHFRatio; goto copy;
        case 7:  as_int = p.lReflections; goto copy;
        case 8:  as_float = p.flReflectionsDelay; goto copy;
        case 9:  as_int = p.lReverb; goto copy;
        case 10: as_float = p.flReverbDelay; goto copy;
        case 11: as_int = static_cast<int32_t>(p.dwEnvironment); goto copy;
        case 12: as_float = p.flEnvironmentSize; goto copy;
        case 13: as_float = p.flEnvironmentDiffusion; goto copy;
        case 14: as_float = p.flAirAbsorptionHF; goto copy;
        case 15: as_int = static_cast<int32_t>(p.dwFlags); goto copy;
        default: return false;
    }
copy:
    std::memcpy(prop_data, (base_id == 4 || base_id == 5 || base_id == 6 || base_id == 8 ||
                            base_id == 10 || base_id == 12 || base_id == 13 || base_id == 14)
                           ? static_cast<const void*>(&as_float) : static_cast<const void*>(&as_int), 4);
    if (out_len) *out_len = 4;
    return true;
}

static bool apply_eax20_listener_prop(eax20_listener_properties_t& p, uint32_t base_id, const void* prop_data, uint32_t prop_len) {
    if (!prop_data || prop_len < 4) return false;
    const int32_t as_int = *static_cast<const int32_t*>(prop_data);
    const uint32_t as_uint = *static_cast<const uint32_t*>(prop_data);
    const float as_float = *static_cast<const float*>(prop_data);
    switch (base_id) {
        case 2:  p.lRoom = as_int; return true;
        case 3:  p.lRoomHF = as_int; return true;
        case 4:  p.flRoomRolloffFactor = as_float; return true;
        case 5:  p.flDecayTime = as_float; return true;
        case 6:  p.flDecayHFRatio = as_float; return true;
        case 7:  p.lReflections = as_int; return true;
        case 8:  p.flReflectionsDelay = as_float; return true;
        case 9:  p.lReverb = as_int; return true;
        case 10: p.flReverbDelay = as_float; return true;
        case 11:
            p.dwEnvironment = as_uint;
            if (as_uint == EAX_ENVIRONMENT_GENERIC) p = eax::PRESET_GENERIC;
            else if (as_uint == EAX_ENVIRONMENT_CAVE) p = eax::PRESET_CAVE;
            return true;
        case 12: p.flEnvironmentSize = as_float; return true;
        case 13: p.flEnvironmentDiffusion = as_float; return true;
        case 14: p.flAirAbsorptionHF = as_float; return true;
        case 15: p.dwFlags = as_uint; return true;
        default: return false;
    }
}

static bool eax20_buffer_prop_get(const eax20_buffer_properties_t& p, uint32_t base_id, void* prop_data, uint32_t prop_len, uint32_t* out_len) {
    if (!prop_data || prop_len < 4) return false;
    int32_t as_int = 0;
    float as_float = 0.0f;
    switch (base_id) {
        case 2:  as_int = p.lDirect; goto copy_int;
        case 3:  as_int = p.lDirectHF; goto copy_int;
        case 4:  as_int = p.lRoom; goto copy_int;
        case 5:  as_int = p.lRoomHF; goto copy_int;
        case 6:  as_float = p.flRoomRolloffFactor; goto copy_float;
        case 7:  as_int = p.lObstruction; goto copy_int;
        case 8:  as_float = p.flObstructionLFRatio; goto copy_float;
        case 9:  as_int = p.lOcclusion; goto copy_int;
        case 10: as_float = p.flOcclusionLFRatio; goto copy_float;
        case 11: as_float = p.flOcclusionRoomRatio; goto copy_float;
        case 12: as_int = p.lOutsideVolumeHF; goto copy_int;
        case 13: as_float = p.flAirAbsorptionFactor; goto copy_float;
        case 14: as_int = static_cast<int32_t>(p.dwFlags); goto copy_int;
        default: return false;
    }
copy_int:
    std::memcpy(prop_data, &as_int, sizeof(as_int));
    if (out_len) *out_len = sizeof(as_int);
    return true;
copy_float:
    std::memcpy(prop_data, &as_float, sizeof(as_float));
    if (out_len) *out_len = sizeof(as_float);
    return true;
}

static bool apply_eax20_buffer_prop(eax20_buffer_properties_t& p, uint32_t base_id, const void* prop_data, uint32_t prop_len) {
    if (!prop_data || prop_len < 4) return false;
    const int32_t as_int = *static_cast<const int32_t*>(prop_data);
    const float as_float = *static_cast<const float*>(prop_data);
    switch (base_id) {
        case 2:  p.lDirect = as_int; return true;
        case 3:  p.lDirectHF = as_int; return true;
        case 4:  p.lRoom = as_int; return true;
        case 5:  p.lRoomHF = as_int; return true;
        case 6:  p.flRoomRolloffFactor = as_float; return true;
        case 7:  p.lObstruction = as_int; return true;
        case 8:  p.flObstructionLFRatio = as_float; return true;
        case 9:  p.lOcclusion = as_int; return true;
        case 10: p.flOcclusionLFRatio = as_float; return true;
        case 11: p.flOcclusionRoomRatio = as_float; return true;
        case 12: p.lOutsideVolumeHF = as_int; return true;
        case 13: p.flAirAbsorptionFactor = as_float; return true;
        case 14: p.dwFlags = static_cast<uint32_t>(as_int); return true;
        default: return false;
    }
}

EAX_API eax_result_t eax_prop_query_support(eax_context_h context, eax_buffer_h /*buffer*/, const eax_guid_t* prop_set, uint32_t /*prop_id*/, uint32_t* out_support) {
    if (!context || !prop_set || !out_support) return EAX_ERR_POINTER;

    if (eax::are_guids_equal(*prop_set, eax::GUID_EAX10_Listener) ||
        eax::are_guids_equal(*prop_set, eax::GUID_EAX10_Buffer) ||
        eax::are_guids_equal(*prop_set, eax::GUID_EAX20_Listener) ||
        eax::are_guids_equal(*prop_set, eax::GUID_EAX20_Buffer)) {
        *out_support = EAX_KSPROPERTY_SUPPORT_GET | EAX_KSPROPERTY_SUPPORT_SET;
        return EAX_OK;
    }
    return EAX_ERR_PROP_UNSUPPORTED;
}

EAX_API eax_result_t eax_prop_get(eax_context_h context, eax_buffer_h buffer, const eax_guid_t* prop_set, uint32_t prop_id, void* /*instance_data*/, uint32_t /*instance_len*/, void* prop_data, uint32_t prop_len, uint32_t* bytes_returned) {
    if (!context || !prop_set || !prop_data) return EAX_ERR_POINTER;

    const uint32_t base_id = prop_id & 0x7FFFFFFFu;

    if (eax::are_guids_equal(*prop_set, eax::GUID_EAX20_Listener)) {
        if (base_id == 0) return EAX_ERR_PROP_UNSUPPORTED;
        uint32_t returned = 0;
        if (base_id == 1) {
            if (prop_len < sizeof(eax20_listener_properties_t)) return EAX_ERR_INVALID_PARAM;
            std::memcpy(prop_data, &context->eax_listener, sizeof(eax20_listener_properties_t));
            returned = sizeof(eax20_listener_properties_t);
        } else if (!eax20_listener_prop_get(context->eax_listener, base_id, prop_data, prop_len, &returned)) {
            return EAX_ERR_PROP_UNSUPPORTED;
        }
        if (bytes_returned) *bytes_returned = returned;
        return EAX_OK;
    }
    if (eax::are_guids_equal(*prop_set, eax::GUID_EAX20_Buffer)) {
        if (!buffer) return EAX_ERR_INVALID_PARAM;
        if (base_id == 0 || base_id == 1) {
            if (prop_len < sizeof(eax20_buffer_properties_t)) return EAX_ERR_INVALID_PARAM;
            std::memcpy(prop_data, &buffer->eax20, sizeof(eax20_buffer_properties_t));
            if (bytes_returned) *bytes_returned = sizeof(eax20_buffer_properties_t);
            return EAX_OK;
        }
        uint32_t returned = 0;
        if (!eax20_buffer_prop_get(buffer->eax20, base_id, prop_data, prop_len, &returned)) {
            return EAX_ERR_PROP_UNSUPPORTED;
        }
        if (bytes_returned) *bytes_returned = returned;
        return EAX_OK;
    }
    if (eax::are_guids_equal(*prop_set, eax::GUID_EAX10_Listener)) {
        if (prop_len < sizeof(eax10_reverb_properties_t)) return EAX_ERR_INVALID_PARAM;
        eax10_reverb_properties_t p10{};
        p10.dwEnvironment = context->eax_listener.dwEnvironment;
        p10.fVolume = eax::mb_to_gain(context->eax_listener.lRoom);
        p10.fDecayTime_sec = context->eax_listener.flDecayTime;
        p10.fDamping = 0.5f;
        std::memcpy(prop_data, &p10, sizeof(p10));
        if (bytes_returned) *bytes_returned = sizeof(p10);
        return EAX_OK;
    }
    if (eax::are_guids_equal(*prop_set, eax::GUID_EAX10_Buffer)) {
        if (!buffer || prop_len < sizeof(float)) return EAX_ERR_INVALID_PARAM;
        const float mix = buffer->eax10_mix.load(std::memory_order_relaxed);
        std::memcpy(prop_data, &mix, sizeof(float));
        if (bytes_returned) *bytes_returned = sizeof(float);
        return EAX_OK;
    }
    return EAX_ERR_PROP_UNSUPPORTED;
}

EAX_API eax_result_t eax_prop_set(eax_context_h context, eax_buffer_h buffer, const eax_guid_t* prop_set, uint32_t prop_id, void* /*instance_data*/, uint32_t /*instance_len*/, const void* prop_data, uint32_t prop_len) {
    if (!context || !prop_set || (!prop_data && prop_len > 0)) return EAX_ERR_POINTER;

    const uint32_t base_id = prop_id & 0x7FFFFFFFu;

    if (eax::are_guids_equal(*prop_set, eax::GUID_EAX20_Listener)) {
        if (base_id == 0) {
            context->commit_deferred_settings();
            return EAX_OK;
        }
        eax20_listener_properties_t p = (base_id == 1)
            ? [&]() {
                if (prop_len < sizeof(eax20_listener_properties_t)) return eax20_listener_properties_t{};
                return *static_cast<const eax20_listener_properties_t*>(prop_data);
            }()
            : context->eax_listener;
        if (base_id == 1 && prop_len < sizeof(eax20_listener_properties_t)) return EAX_ERR_INVALID_PARAM;
        if (base_id != 1 && !apply_eax20_listener_prop(p, base_id, prop_data, prop_len)) {
            return EAX_ERR_PROP_UNSUPPORTED;
        }
        if (prop_id & 0x80000000u) {
            context->eax_listener_deferred = p;
            context->eax_listener_has_deferred = true;
        } else {
            context->eax_listener = p;
            context->apply_eax_reverb_to_openal();
        }
        return EAX_OK;
    }

    if (eax::are_guids_equal(*prop_set, eax::GUID_EAX20_Buffer)) {
        if (!buffer) return EAX_ERR_INVALID_PARAM;
        if (base_id == 0) {
            buffer->commit_deferred_eax20();
            return EAX_OK;
        }
        eax20_buffer_properties_t p = (base_id == 1)
            ? [&]() {
                if (prop_len < sizeof(eax20_buffer_properties_t)) return eax20_buffer_properties_t{};
                return *static_cast<const eax20_buffer_properties_t*>(prop_data);
            }()
            : buffer->eax20;
        if (base_id == 1 && prop_len < sizeof(eax20_buffer_properties_t)) return EAX_ERR_INVALID_PARAM;
        if (base_id != 1 && !apply_eax20_buffer_prop(p, base_id, prop_data, prop_len)) {
            return EAX_ERR_PROP_UNSUPPORTED;
        }
        if (prop_id & 0x80000000u) {
            buffer->eax20_deferred = p;
            buffer->eax20_has_deferred = true;
        } else {
            buffer->eax20 = p;
            buffer->sync_to_openal();
        }
        return EAX_OK;
    }

    if (eax::are_guids_equal(*prop_set, eax::GUID_EAX10_Listener)) {
        if (base_id == 0) {
            context->commit_deferred_settings();
            return EAX_OK;
        }
        if (prop_len < sizeof(eax10_reverb_properties_t)) return EAX_ERR_INVALID_PARAM;
        const auto* p = static_cast<const eax10_reverb_properties_t*>(prop_data);
        eax20_listener_properties_t p20 = context->eax_listener;
        p20.dwEnvironment = p->dwEnvironment;
        p20.flDecayTime = p->fDecayTime_sec;
        if (prop_id & 0x80000000u) {
            context->eax_listener_deferred = p20;
            context->eax_listener_has_deferred = true;
        } else {
            context->eax_listener = p20;
            context->apply_eax_reverb_to_openal();
        }
        return EAX_OK;
    }

    if (eax::are_guids_equal(*prop_set, eax::GUID_EAX10_Buffer)) {
        if (!buffer || prop_len < sizeof(float)) return EAX_ERR_INVALID_PARAM;
        float mix = 0.0f;
        std::memcpy(&mix, prop_data, sizeof(float));
        buffer->eax10_mix.store(mix, std::memory_order_relaxed);
        return EAX_OK;
    }

    return EAX_ERR_PROP_UNSUPPORTED;
}

} // extern "C"
