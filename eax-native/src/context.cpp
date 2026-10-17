#include "context.h"
#include <algorithm>
#include <vector>

eax_context_s::eax_context_s(bool loopback, uint32_t loopback_sample_rate) noexcept {
    listener_seqlock.write_begin();
    listener.dwSize = sizeof(eax_ds3dlistener_t);
    listener.vPosition = { 0.0f, 0.0f, 0.0f };
    listener.vVelocity = { 0.0f, 0.0f, 0.0f };
    listener.vOrientFront = { 0.0f, 0.0f, 1.0f }; // DS3D +Z forward
    listener.vOrientTop = { 0.0f, 1.0f, 0.0f };
    listener.flDistanceFactor = 1.0f;
    listener.flRolloffFactor = 1.0f;
    listener.flDopplerFactor = 1.0f;
    listener_seqlock.write_end();
    listener_deferred = listener;
    listener_has_deferred = false;

    eax_listener = eax::PRESET_GENERIC;
    eax_listener_deferred = eax_listener;
    eax_listener_has_deferred = false;

    init_openal(loopback, loopback_sample_rate);
}

eax_context_s::~eax_context_s() noexcept {
    shutdown_openal();
}

void eax_context_s::init_openal(bool loopback, uint32_t loopback_sample_rate) noexcept {
    auto& ext = eax::get_al_ext();
    m_is_loopback = false;
    fallback_sample_rate = loopback_sample_rate ? loopback_sample_rate : 44100;

    if (loopback) {
        // Try loopback device
        if (alcIsExtensionPresent(nullptr, "ALC_SOFT_loopback")) {
            ext.alcLoopbackOpenDeviceSOFT = reinterpret_cast<LPALCLOOPBACKOPENDEVICESOFT>(
                alcGetProcAddress(nullptr, "alcLoopbackOpenDeviceSOFT"));
            ext.alcRenderSamplesSOFT = reinterpret_cast<LPALCRENDERSAMPLESSOFT>(
                alcGetProcAddress(nullptr, "alcRenderSamplesSOFT"));

            if (ext.alcLoopbackOpenDeviceSOFT) {
                const ALCint attrs[] = {
                    ALC_FORMAT_CHANNELS_SOFT, ALC_STEREO_SOFT,
                    ALC_FORMAT_TYPE_SOFT, ALC_FLOAT_SOFT,
                    ALC_FREQUENCY, static_cast<ALCint>(loopback_sample_rate ? loopback_sample_rate : 44100),
                    0
                };
                m_al_device = ext.alcLoopbackOpenDeviceSOFT(nullptr);
                if (m_al_device) {
                    m_al_context = alcCreateContext(m_al_device, attrs);
                    m_is_loopback = true;
                }
            }
        }
    }

    if (!m_al_device) {
        // Standard interactive audio playback device
        m_al_device = alcOpenDevice(nullptr);
        if (m_al_device) {
            m_al_context = alcCreateContext(m_al_device, nullptr);
        }
    }

    if (m_al_context) {
        alcMakeContextCurrent(m_al_context);
        ext.init(m_al_device);

        // Configure EFX EAX Reverb Auxiliary Slot
        if (ext.has_efx && ext.alGenAuxiliaryEffectSlots && ext.alGenEffects) {
            ext.alGenAuxiliaryEffectSlots(1, &m_eax_slot);
            ext.alGenEffects(1, &m_eax_effect);
            apply_eax_reverb_to_openal();
        }

        // Initialize voice pool with OpenAL source handles
        voice_pool.init(this);
        sync_listener_to_openal();
    }
}

void eax_context_s::shutdown_openal() noexcept {
    auto& ext = eax::get_al_ext();
    if (m_al_context) {
        alcMakeContextCurrent(nullptr);

        if (m_eax_effect && ext.alDeleteEffects) {
            ext.alDeleteEffects(1, &m_eax_effect);
            m_eax_effect = 0;
        }
        if (m_eax_slot && ext.alDeleteAuxiliaryEffectSlots) {
            ext.alDeleteAuxiliaryEffectSlots(1, &m_eax_slot);
            m_eax_slot = 0;
        }

        alcDestroyContext(m_al_context);
        m_al_context = nullptr;
    }

    if (m_al_device) {
        alcCloseDevice(m_al_device);
        m_al_device = nullptr;
    }
}

void eax_context_s::sync_listener_to_openal() noexcept {
    if (!m_al_context) return;
    const auto snap = get_listener_snapshot();

    // Left-handed (+X right, +Y up, +Z forward) to OpenAL right-handed (+X right, +Y up, -Z forward)
    alListener3f(AL_POSITION, snap.vPosition.x, snap.vPosition.y, -snap.vPosition.z);
    alListener3f(AL_VELOCITY, snap.vVelocity.x, snap.vVelocity.y, -snap.vVelocity.z);

    const float ori[6] = {
        snap.vOrientFront.x, snap.vOrientFront.y, -snap.vOrientFront.z,
        snap.vOrientTop.x, snap.vOrientTop.y, -snap.vOrientTop.z
    };
    alListenerfv(AL_ORIENTATION, ori);
    alListenerf(AL_METERS_PER_UNIT, snap.flDistanceFactor);
    alDopplerFactor(snap.flDopplerFactor);
}

void eax_context_s::apply_eax_reverb_to_openal() noexcept {
    fallback_reverb.configure(eax_listener, fallback_sample_rate);

    auto& ext = eax::get_al_ext();
    if (!ext.has_efx || !m_eax_effect || !m_eax_slot || !ext.alEffecti || !ext.alEffectf || !ext.alAuxiliaryEffectSloti) {
        return;
    }

    ext.alEffecti(m_eax_effect, AL_EFFECT_TYPE, AL_EFFECT_EAXREVERB);

    // Translate EAX properties to AL_EAXREVERB_*
    ext.alEffectf(m_eax_effect, AL_EAXREVERB_GAIN, eax::mb_to_gain(eax_listener.lRoom));
    ext.alEffectf(m_eax_effect, AL_EAXREVERB_GAINHF, eax::mb_to_gain(eax_listener.lRoomHF));
    ext.alEffectf(m_eax_effect, AL_EAXREVERB_ROOM_ROLLOFF_FACTOR, eax_listener.flRoomRolloffFactor);
    ext.alEffectf(m_eax_effect, AL_EAXREVERB_DECAY_TIME, std::clamp(eax_listener.flDecayTime, 0.1f, 20.0f));
    ext.alEffectf(m_eax_effect, AL_EAXREVERB_DECAY_HFRATIO, std::clamp(eax_listener.flDecayHFRatio, 0.1f, 2.0f));
    ext.alEffectf(m_eax_effect, AL_EAXREVERB_REFLECTIONS_GAIN, eax::mb_to_gain(eax_listener.lReflections));
    ext.alEffectf(m_eax_effect, AL_EAXREVERB_REFLECTIONS_DELAY, std::clamp(eax_listener.flReflectionsDelay, 0.0f, 0.3f));
    ext.alEffectf(m_eax_effect, AL_EAXREVERB_LATE_REVERB_GAIN, eax::mb_to_gain(eax_listener.lReverb));
    ext.alEffectf(m_eax_effect, AL_EAXREVERB_LATE_REVERB_DELAY, std::clamp(eax_listener.flReverbDelay, 0.0f, 0.1f));
    ext.alEffectf(m_eax_effect, AL_EAXREVERB_DIFFUSION, std::clamp(eax_listener.flEnvironmentDiffusion, 0.0f, 1.0f));

    // Commit effect to auxiliary slot
    ext.alAuxiliaryEffectSloti(m_eax_slot, AL_EFFECTSLOT_EFFECT, static_cast<ALint>(m_eax_effect));
}

void eax_context_s::commit_deferred_settings() noexcept {
    std::lock_guard<std::mutex> lock(config_mutex);
    if (listener_has_deferred) {
        listener_seqlock.write_begin();
        listener = listener_deferred;
        listener_seqlock.write_end();
        listener_has_deferred = false;
        sync_listener_to_openal();
    }
    if (eax_listener_has_deferred) {
        eax_listener = eax_listener_deferred;
        eax_listener_has_deferred = false;
        apply_eax_reverb_to_openal();
    }

    // Apply deferred buffer settings across all voices
    for (auto& voice : voice_pool.get_all_voices()) {
        if (!voice.in_use.load(std::memory_order_relaxed)) continue;
        if (voice.ds3d_has_deferred) {
            voice.ds3d_seqlock.write_begin();
            voice.ds3d = voice.ds3d_deferred;
            voice.ds3d_seqlock.write_end();
            voice.ds3d_has_deferred = false;
            voice.sync_to_openal();
        }
        if (voice.eax20_has_deferred) {
            voice.eax20 = voice.eax20_deferred;
            voice.eax20_has_deferred = false;
            voice.sync_to_openal();
        }
    }
}

void eax_context_s::render_offline(float* out_stereo_pcm, size_t frame_count, uint32_t sample_rate) noexcept {
    if (!out_stereo_pcm || frame_count == 0) return;

    auto& ext = eax::get_al_ext();
    if (m_is_loopback && ext.alcRenderSamplesSOFT && m_al_device) {
        // Direct native OpenAL Soft rendering via ALC_SOFT_loopback
        ext.alcRenderSamplesSOFT(m_al_device, out_stereo_pcm, static_cast<ALCsizei>(frame_count));
        return;
    }

    // Diagnostic fallback software mixing if loopback is unavailable
    // Process in chunks of SCRATCH_MAX_FRAMES using preallocated member buffers (RT safe: zero malloc)
    if (fallback_sample_rate != sample_rate) {
        fallback_sample_rate = sample_rate;
        fallback_reverb.configure(eax_listener, sample_rate);
    }
    size_t frames_rendered = 0;
    while (frames_rendered < frame_count) {
        const size_t chunk = std::min(frame_count - frames_rendered, SCRATCH_MAX_FRAMES);
        float* chunk_out = out_stereo_pcm + 2 * frames_rendered;
        std::fill_n(chunk_out, chunk * 2, 0.0f);
        std::fill_n(m_scratch_reverb, chunk, 0.0f);

        const auto listener_snap = get_listener_snapshot();

        for (auto& voice : voice_pool.get_all_voices()) {
            if (!voice.in_use.load(std::memory_order_relaxed)) continue;
            if ((voice.status.load(std::memory_order_relaxed) & EAX_DSBSTATUS_PLAYING) == 0) continue;

            voice.read_samples(m_scratch_l, m_scratch_r, chunk, sample_rate);

            const bool is_3d = (voice.caps.dwFlags & EAX_DSBCAPS_CTRL3D) != 0;
            eax::SpatialGains gains;
            float p_left = 1.0f;
            float p_right = 1.0f;

            if (is_3d) {
                const auto voice_ds3d = voice.get_ds3d_snapshot();
                gains = eax::Spatializer::compute_gains(listener_snap, voice_ds3d, voice.volume_mb.load(std::memory_order_relaxed));
                p_left = gains.left_gain;
                p_right = gains.right_gain;
            } else {
                const float vol = eax::mb_to_gain(voice.volume_mb.load(std::memory_order_relaxed));
                const float pan = eax::calculate_panning_soft(voice.pan_mb.load(std::memory_order_relaxed));
                p_left = vol * std::sqrt(0.5f * (1.0f - pan));
                p_right = vol * std::sqrt(0.5f * (1.0f + pan));
            }

            for (size_t i = 0; i < chunk; ++i) {
                const float sample_mono = 0.5f * (m_scratch_l[i] + m_scratch_r[i]);
                chunk_out[2 * i] += m_scratch_l[i] * p_left;
                chunk_out[2 * i + 1] += m_scratch_r[i] * p_right;
                const float send_gain = is_3d ? gains.distance_gain : 1.0f;
                m_scratch_reverb[i] += sample_mono * send_gain;
            }
        }

        for (size_t i = 0; i < chunk; ++i) {
            float rev_l = 0.0f;
            float rev_r = 0.0f;
            fallback_reverb.process(m_scratch_reverb[i], rev_l, rev_r);
            chunk_out[2 * i] += rev_l;
            chunk_out[2 * i + 1] += rev_r;
        }

        frames_rendered += chunk;
    }
}
