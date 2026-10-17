#include "eax_native_abi.h"
#include "context.h"
#include <cstring>
#include <new>

extern "C" {

EAX_API eax_result_t eax_context_create(eax_context_h* out_context) {
    if (!out_context) return EAX_ERR_POINTER;
    auto* ctx = new (std::nothrow) eax_context_s();
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
    *out_params = context->listener;
    return EAX_OK;
}

EAX_API eax_result_t eax_listener_set_all_parameters(eax_context_h context, const eax_ds3dlistener_t* params, uint32_t apply_mode) {
    if (!context || !params) return EAX_ERR_POINTER;
    if (apply_mode == EAX_DS3D_DEFERRED) {
        context->listener_deferred = *params;
        context->listener_has_deferred = true;
    } else {
        context->listener = *params;
    }
    return EAX_OK;
}

EAX_API eax_result_t eax_listener_get_position(eax_context_h context, eax_vector_t* out_pos) {
    if (!context || !out_pos) return EAX_ERR_POINTER;
    *out_pos = context->listener.vPosition;
    return EAX_OK;
}

EAX_API eax_result_t eax_listener_set_position(eax_context_h context, float x, float y, float z, uint32_t apply_mode) {
    if (!context) return EAX_ERR_POINTER;
    if (apply_mode == EAX_DS3D_DEFERRED) {
        context->listener_deferred.vPosition = { x, y, z };
        context->listener_has_deferred = true;
    } else {
        context->listener.vPosition = { x, y, z };
    }
    return EAX_OK;
}

EAX_API eax_result_t eax_listener_get_velocity(eax_context_h context, eax_vector_t* out_vel) {
    if (!context || !out_vel) return EAX_ERR_POINTER;
    *out_vel = context->listener.vVelocity;
    return EAX_OK;
}

EAX_API eax_result_t eax_listener_set_velocity(eax_context_h context, float x, float y, float z, uint32_t apply_mode) {
    if (!context) return EAX_ERR_POINTER;
    if (apply_mode == EAX_DS3D_DEFERRED) {
        context->listener_deferred.vVelocity = { x, y, z };
        context->listener_has_deferred = true;
    } else {
        context->listener.vVelocity = { x, y, z };
    }
    return EAX_OK;
}

EAX_API eax_result_t eax_listener_get_orientation(eax_context_h context, eax_vector_t* out_front, eax_vector_t* out_top) {
    if (!context) return EAX_ERR_POINTER;
    if (out_front) *out_front = context->listener.vOrientFront;
    if (out_top) *out_top = context->listener.vOrientTop;
    return EAX_OK;
}

EAX_API eax_result_t eax_listener_set_orientation(eax_context_h context, float fx, float fy, float fz, float tx, float ty, float tz, uint32_t apply_mode) {
    if (!context) return EAX_ERR_POINTER;
    if (apply_mode == EAX_DS3D_DEFERRED) {
        context->listener_deferred.vOrientFront = { fx, fy, fz };
        context->listener_deferred.vOrientTop = { tx, ty, tz };
        context->listener_has_deferred = true;
    } else {
        context->listener.vOrientFront = { fx, fy, fz };
        context->listener.vOrientTop = { tx, ty, tz };
    }
    return EAX_OK;
}

EAX_API eax_result_t eax_listener_get_distance_factor(eax_context_h context, float* out_factor) {
    if (!context || !out_factor) return EAX_ERR_POINTER;
    *out_factor = context->listener.flDistanceFactor;
    return EAX_OK;
}

EAX_API eax_result_t eax_listener_set_distance_factor(eax_context_h context, float factor, uint32_t apply_mode) {
    if (!context || factor <= 0.0f) return EAX_ERR_INVALID_PARAM;
    if (apply_mode == EAX_DS3D_DEFERRED) {
        context->listener_deferred.flDistanceFactor = factor;
        context->listener_has_deferred = true;
    } else {
        context->listener.flDistanceFactor = factor;
    }
    return EAX_OK;
}

EAX_API eax_result_t eax_listener_get_rolloff_factor(eax_context_h context, float* out_factor) {
    if (!context || !out_factor) return EAX_ERR_POINTER;
    *out_factor = context->listener.flRolloffFactor;
    return EAX_OK;
}

EAX_API eax_result_t eax_listener_set_rolloff_factor(eax_context_h context, float factor, uint32_t apply_mode) {
    if (!context || factor < 0.0f) return EAX_ERR_INVALID_PARAM;
    if (apply_mode == EAX_DS3D_DEFERRED) {
        context->listener_deferred.flRolloffFactor = factor;
        context->listener_has_deferred = true;
    } else {
        context->listener.flRolloffFactor = factor;
    }
    return EAX_OK;
}

EAX_API eax_result_t eax_listener_get_doppler_factor(eax_context_h context, float* out_factor) {
    if (!context || !out_factor) return EAX_ERR_POINTER;
    *out_factor = context->listener.flDopplerFactor;
    return EAX_OK;
}

EAX_API eax_result_t eax_listener_set_doppler_factor(eax_context_h context, float factor, uint32_t apply_mode) {
    if (!context || factor < 0.0f) return EAX_ERR_INVALID_PARAM;
    if (apply_mode == EAX_DS3D_DEFERRED) {
        context->listener_deferred.flDopplerFactor = factor;
        context->listener_has_deferred = true;
    } else {
        context->listener.flDopplerFactor = factor;
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

    // Allocate PCM sample memory from pre-allocated arena
    void* mem = context->arena.allocate(caps->dwBufferBytes);
    if (!mem && caps->dwBufferBytes > 0) {
        context->voice_pool.release_voice(voice);
        return EAX_ERR_OUT_OF_MEMORY;
    }
    voice->sample_buffer.initialize(mem, caps->dwBufferBytes);

    *out_buffer = voice;
    return EAX_OK;
}

EAX_API eax_result_t eax_buffer_destroy(eax_buffer_h buffer) {
    if (!buffer) return EAX_ERR_POINTER;
    buffer->status.store(0, std::memory_order_release);
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
    voice->ds3d = source_buffer->ds3d;

    // Duplicate shares underlying sample memory
    voice->sample_buffer.initialize(source_buffer->sample_buffer.data(), source_buffer->sample_buffer.capacity());

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
    buffer->sample_buffer.lock_region(offset, bytes, ptr1, bytes1, ptr2, bytes2);
    return EAX_OK;
}

EAX_API eax_result_t eax_buffer_unlock(eax_buffer_h buffer, void* /*ptr1*/, uint32_t /*bytes1*/, void* /*ptr2*/, uint32_t /*bytes2*/) {
    if (!buffer) return EAX_ERR_POINTER;
    return EAX_OK;
}

EAX_API eax_result_t eax_buffer_play(eax_buffer_h buffer, uint32_t /*priority*/, uint32_t flags) {
    if (!buffer) return EAX_ERR_POINTER;
    const bool looping = (flags & EAX_DSBPLAY_LOOPING) != 0;
    buffer->is_looping.store(looping, std::memory_order_relaxed);

    uint32_t st = EAX_DSBSTATUS_PLAYING;
    if (looping) st |= EAX_DSBSTATUS_LOOPING;
    buffer->status.store(st, std::memory_order_release);
    return EAX_OK;
}

EAX_API eax_result_t eax_buffer_stop(eax_buffer_h buffer) {
    if (!buffer) return EAX_ERR_POINTER;
    buffer->status.store(0, std::memory_order_release);
    return EAX_OK;
}

EAX_API eax_result_t eax_buffer_get_status(eax_buffer_h buffer, uint32_t* out_status) {
    if (!buffer || !out_status) return EAX_ERR_POINTER;
    *out_status = buffer->status.load(std::memory_order_acquire);
    return EAX_OK;
}

EAX_API eax_result_t eax_buffer_get_current_position(eax_buffer_h buffer, uint32_t* out_play_pos, uint32_t* out_write_pos) {
    if (!buffer) return EAX_ERR_POINTER;
    if (out_play_pos) *out_play_pos = static_cast<uint32_t>(buffer->sample_buffer.get_play_pos());
    if (out_write_pos) *out_write_pos = static_cast<uint32_t>(buffer->sample_buffer.get_write_pos());
    return EAX_OK;
}

EAX_API eax_result_t eax_buffer_set_current_position(eax_buffer_h buffer, uint32_t new_pos) {
    if (!buffer) return EAX_ERR_POINTER;
    buffer->sample_buffer.set_play_pos(new_pos);
    if (buffer->format.nBlockAlign > 0) {
        buffer->playback_cursor.store(static_cast<double>(new_pos / buffer->format.nBlockAlign), std::memory_order_relaxed);
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
    return EAX_OK;
}

/* ---------------- 3D Buffer Parameters ---------------- */

EAX_API eax_result_t eax_buffer_get_3d_all_parameters(eax_buffer_h buffer, eax_ds3dbuffer_t* out_params) {
    if (!buffer || !out_params) return EAX_ERR_POINTER;
    *out_params = buffer->ds3d;
    return EAX_OK;
}

EAX_API eax_result_t eax_buffer_set_3d_all_parameters(eax_buffer_h buffer, const eax_ds3dbuffer_t* params, uint32_t apply_mode) {
    if (!buffer || !params) return EAX_ERR_POINTER;
    if (apply_mode == EAX_DS3D_DEFERRED) {
        buffer->ds3d_deferred = *params;
        buffer->ds3d_has_deferred = true;
    } else {
        buffer->ds3d = *params;
    }
    return EAX_OK;
}

EAX_API eax_result_t eax_buffer_get_3d_position(eax_buffer_h buffer, eax_vector_t* out_pos) {
    if (!buffer || !out_pos) return EAX_ERR_POINTER;
    *out_pos = buffer->ds3d.vPosition;
    return EAX_OK;
}

EAX_API eax_result_t eax_buffer_set_3d_position(eax_buffer_h buffer, float x, float y, float z, uint32_t apply_mode) {
    if (!buffer) return EAX_ERR_POINTER;
    if (apply_mode == EAX_DS3D_DEFERRED) {
        buffer->ds3d_deferred.vPosition = { x, y, z };
        buffer->ds3d_has_deferred = true;
    } else {
        buffer->ds3d.vPosition = { x, y, z };
    }
    return EAX_OK;
}

EAX_API eax_result_t eax_buffer_get_3d_velocity(eax_buffer_h buffer, eax_vector_t* out_vel) {
    if (!buffer || !out_vel) return EAX_ERR_POINTER;
    *out_vel = buffer->ds3d.vVelocity;
    return EAX_OK;
}

EAX_API eax_result_t eax_buffer_set_3d_velocity(eax_buffer_h buffer, float x, float y, float z, uint32_t apply_mode) {
    if (!buffer) return EAX_ERR_POINTER;
    if (apply_mode == EAX_DS3D_DEFERRED) {
        buffer->ds3d_deferred.vVelocity = { x, y, z };
        buffer->ds3d_has_deferred = true;
    } else {
        buffer->ds3d.vVelocity = { x, y, z };
    }
    return EAX_OK;
}

EAX_API eax_result_t eax_buffer_get_3d_cone_angles(eax_buffer_h buffer, uint32_t* out_inside, uint32_t* out_outside) {
    if (!buffer) return EAX_ERR_POINTER;
    if (out_inside) *out_inside = buffer->ds3d.dwInsideConeAngle;
    if (out_outside) *out_outside = buffer->ds3d.dwOutsideConeAngle;
    return EAX_OK;
}

EAX_API eax_result_t eax_buffer_set_3d_cone_angles(eax_buffer_h buffer, uint32_t inside, uint32_t outside, uint32_t apply_mode) {
    if (!buffer) return EAX_ERR_POINTER;
    if (apply_mode == EAX_DS3D_DEFERRED) {
        buffer->ds3d_deferred.dwInsideConeAngle = inside;
        buffer->ds3d_deferred.dwOutsideConeAngle = outside;
        buffer->ds3d_has_deferred = true;
    } else {
        buffer->ds3d.dwInsideConeAngle = inside;
        buffer->ds3d.dwOutsideConeAngle = outside;
    }
    return EAX_OK;
}

EAX_API eax_result_t eax_buffer_get_3d_cone_orientation(eax_buffer_h buffer, eax_vector_t* out_orientation) {
    if (!buffer || !out_orientation) return EAX_ERR_POINTER;
    *out_orientation = buffer->ds3d.vConeOrientation;
    return EAX_OK;
}

EAX_API eax_result_t eax_buffer_set_3d_cone_orientation(eax_buffer_h buffer, float x, float y, float z, uint32_t apply_mode) {
    if (!buffer) return EAX_ERR_POINTER;
    if (apply_mode == EAX_DS3D_DEFERRED) {
        buffer->ds3d_deferred.vConeOrientation = { x, y, z };
        buffer->ds3d_has_deferred = true;
    } else {
        buffer->ds3d.vConeOrientation = { x, y, z };
    }
    return EAX_OK;
}

EAX_API eax_result_t eax_buffer_get_3d_cone_outside_volume(eax_buffer_h buffer, int32_t* out_volume_mb) {
    if (!buffer || !out_volume_mb) return EAX_ERR_POINTER;
    *out_volume_mb = buffer->ds3d.lConeOutsideVolume;
    return EAX_OK;
}

EAX_API eax_result_t eax_buffer_set_3d_cone_outside_volume(eax_buffer_h buffer, int32_t volume_mb, uint32_t apply_mode) {
    if (!buffer) return EAX_ERR_POINTER;
    if (apply_mode == EAX_DS3D_DEFERRED) {
        buffer->ds3d_deferred.lConeOutsideVolume = volume_mb;
        buffer->ds3d_has_deferred = true;
    } else {
        buffer->ds3d.lConeOutsideVolume = volume_mb;
    }
    return EAX_OK;
}

EAX_API eax_result_t eax_buffer_get_3d_min_distance(eax_buffer_h buffer, float* out_dist) {
    if (!buffer || !out_dist) return EAX_ERR_POINTER;
    *out_dist = buffer->ds3d.flMinDistance;
    return EAX_OK;
}

EAX_API eax_result_t eax_buffer_set_3d_min_distance(eax_buffer_h buffer, float dist, uint32_t apply_mode) {
    if (!buffer || dist <= 0.0f) return EAX_ERR_INVALID_PARAM;
    if (apply_mode == EAX_DS3D_DEFERRED) {
        buffer->ds3d_deferred.flMinDistance = dist;
        buffer->ds3d_has_deferred = true;
    } else {
        buffer->ds3d.flMinDistance = dist;
    }
    return EAX_OK;
}

EAX_API eax_result_t eax_buffer_get_3d_max_distance(eax_buffer_h buffer, float* out_dist) {
    if (!buffer || !out_dist) return EAX_ERR_POINTER;
    *out_dist = buffer->ds3d.flMaxDistance;
    return EAX_OK;
}

EAX_API eax_result_t eax_buffer_set_3d_max_distance(eax_buffer_h buffer, float dist, uint32_t apply_mode) {
    if (!buffer || dist <= 0.0f) return EAX_ERR_INVALID_PARAM;
    if (apply_mode == EAX_DS3D_DEFERRED) {
        buffer->ds3d_deferred.flMaxDistance = dist;
        buffer->ds3d_has_deferred = true;
    } else {
        buffer->ds3d.flMaxDistance = dist;
    }
    return EAX_OK;
}

EAX_API eax_result_t eax_buffer_get_3d_mode(eax_buffer_h buffer, uint32_t* out_mode) {
    if (!buffer || !out_mode) return EAX_ERR_POINTER;
    *out_mode = buffer->ds3d.dwMode;
    return EAX_OK;
}

EAX_API eax_result_t eax_buffer_set_3d_mode(eax_buffer_h buffer, uint32_t mode, uint32_t apply_mode) {
    if (!buffer) return EAX_ERR_POINTER;
    if (apply_mode == EAX_DS3D_DEFERRED) {
        buffer->ds3d_deferred.dwMode = mode;
        buffer->ds3d_has_deferred = true;
    } else {
        buffer->ds3d.dwMode = mode;
    }
    return EAX_OK;
}

/* ---------------- EAX Properties (IKsPropertySet) ---------------- */

EAX_API eax_result_t eax_prop_query_support(eax_context_h context, eax_buffer_h /*buffer*/, const eax_guid_t* prop_set, uint32_t /*prop_id*/, uint32_t* out_support) {
    if (!context || !prop_set || !out_support) return EAX_ERR_POINTER;

    if (eax::are_guids_equal(*prop_set, eax::GUID_EAX10_Listener) ||
        eax::are_guids_equal(*prop_set, eax::GUID_EAX10_Buffer) ||
        eax::are_guids_equal(*prop_set, eax::GUID_EAX20_Listener) ||
        eax::are_guids_equal(*prop_set, eax::GUID_EAX20_Buffer) ||
        eax::are_guids_equal(*prop_set, eax::GUID_EAX30_Listener) ||
        eax::are_guids_equal(*prop_set, eax::GUID_EAX30_Buffer) ||
        eax::are_guids_equal(*prop_set, eax::GUID_EAX40_Context) ||
        eax::are_guids_equal(*prop_set, eax::GUID_EAX40_Source)) {
        *out_support = EAX_KSPROPERTY_SUPPORT_GET | EAX_KSPROPERTY_SUPPORT_SET;
        return EAX_OK;
    }
    return EAX_ERR_PROP_UNSUPPORTED;
}

EAX_API eax_result_t eax_prop_get(eax_context_h context, eax_buffer_h buffer, const eax_guid_t* prop_set, uint32_t /*prop_id*/, void* /*instance_data*/, uint32_t /*instance_len*/, void* prop_data, uint32_t prop_len, uint32_t* bytes_returned) {
    if (!context || !prop_set || !prop_data) return EAX_ERR_POINTER;

    if (eax::are_guids_equal(*prop_set, eax::GUID_EAX20_Listener)) {
        if (prop_len < sizeof(eax20_listener_properties_t)) return EAX_ERR_INVALID_PARAM;
        std::memcpy(prop_data, &context->eax_listener, sizeof(eax20_listener_properties_t));
        if (bytes_returned) *bytes_returned = sizeof(eax20_listener_properties_t);
        return EAX_OK;
    }
    if (eax::are_guids_equal(*prop_set, eax::GUID_EAX20_Buffer)) {
        if (!buffer || prop_len < sizeof(eax20_buffer_properties_t)) return EAX_ERR_INVALID_PARAM;
        std::memcpy(prop_data, &buffer->eax20, sizeof(eax20_buffer_properties_t));
        if (bytes_returned) *bytes_returned = sizeof(eax20_buffer_properties_t);
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
    return EAX_ERR_PROP_UNSUPPORTED;
}

EAX_API eax_result_t eax_prop_set(eax_context_h context, eax_buffer_h buffer, const eax_guid_t* prop_set, uint32_t prop_id, void* /*instance_data*/, uint32_t /*instance_len*/, const void* prop_data, uint32_t prop_len) {
    if (!context || !prop_set || !prop_data) return EAX_ERR_POINTER;

    const bool deferred = (prop_id & 0x80000000u) != 0;

    if (eax::are_guids_equal(*prop_set, eax::GUID_EAX20_Listener)) {
        if (prop_len < sizeof(eax20_listener_properties_t)) return EAX_ERR_INVALID_PARAM;
        const auto* p = static_cast<const eax20_listener_properties_t*>(prop_data);
        if (deferred) {
            context->eax_listener_deferred = *p;
            context->eax_listener_has_deferred = true;
        } else {
            context->eax_listener = *p;
            context->reverb_dsp.configure(context->eax_listener, 44100);
        }
        return EAX_OK;
    }

    if (eax::are_guids_equal(*prop_set, eax::GUID_EAX20_Buffer)) {
        if (!buffer || prop_len < sizeof(eax20_buffer_properties_t)) return EAX_ERR_INVALID_PARAM;
        const auto* p = static_cast<const eax20_buffer_properties_t*>(prop_data);
        if (deferred) {
            buffer->eax20_deferred = *p;
            buffer->eax20_has_deferred = true;
        } else {
            buffer->eax20 = *p;
        }
        return EAX_OK;
    }

    if (eax::are_guids_equal(*prop_set, eax::GUID_EAX10_Listener)) {
        if (prop_len < sizeof(eax10_reverb_properties_t)) return EAX_ERR_INVALID_PARAM;
        const auto* p = static_cast<const eax10_reverb_properties_t*>(prop_data);
        eax20_listener_properties_t p20 = context->eax_listener;
        p20.dwEnvironment = p->dwEnvironment;
        p20.flDecayTime = p->fDecayTime_sec;
        if (deferred) {
            context->eax_listener_deferred = p20;
            context->eax_listener_has_deferred = true;
        } else {
            context->eax_listener = p20;
            context->reverb_dsp.configure(context->eax_listener, 44100);
        }
        return EAX_OK;
    }

    return EAX_ERR_PROP_UNSUPPORTED;
}

} // extern "C"
