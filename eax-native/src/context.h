#ifndef EAX_NATIVE_CONTEXT_H
#define EAX_NATIVE_CONTEXT_H

#include <vector>
#include <mutex>
#include "eax_native_abi.h"
#include "arena.h"
#include "voice_pool.h"
#include "spatializer.h"
#include "eax_reverb.h"
#include "openal_bridge.h"
#include "seqlock.h"

struct eax_context_s {
    eax::MemoryArena arena{eax::MemoryArena::DEFAULT_ARENA_SIZE};
    eax::VoicePool voice_pool{};

    // OpenAL Device & Context
    ALCdevice* m_al_device{nullptr};
    ALCcontext* m_al_context{nullptr};
    ALuint m_eax_slot{0};
    ALuint m_eax_effect{0};
    bool m_is_loopback{false};

    // Listener State with lock-free SeqLock for RT tearing-free snapshot reads
    eax::SeqLock listener_seqlock{};
    eax_ds3dlistener_t listener{};
    eax_ds3dlistener_t listener_deferred{};
    bool listener_has_deferred{false};

    [[nodiscard]] eax_ds3dlistener_t get_listener_snapshot() const noexcept {
        eax_ds3dlistener_t snap;
        uint32_t s;
        do {
            s = listener_seqlock.read_begin();
            snap = listener;
        } while (listener_seqlock.read_retry(s));
        return snap;
    }

    // EAX Reverb State
    eax20_listener_properties_t eax_listener{};
    eax20_listener_properties_t eax_listener_deferred{};
    bool eax_listener_has_deferred{false};

    eax::ReverbDSP fallback_reverb{};
    uint32_t fallback_sample_rate{44100};
    uint32_t speaker_config{EAX_SPEAKER_CONFIG_HEADPHONE};
    std::mutex config_mutex{};

    static constexpr size_t SCRATCH_MAX_FRAMES = 4096;
    float m_scratch_l[SCRATCH_MAX_FRAMES]{};
    float m_scratch_r[SCRATCH_MAX_FRAMES]{};
    float m_scratch_reverb[SCRATCH_MAX_FRAMES]{};

    explicit eax_context_s(bool loopback = false, uint32_t loopback_sample_rate = 44100) noexcept;
    ~eax_context_s() noexcept;

    void init_openal(bool loopback, uint32_t loopback_sample_rate) noexcept;
    void shutdown_openal() noexcept;

    void sync_listener_to_openal() noexcept;
    void apply_eax_reverb_to_openal() noexcept;
    void commit_deferred_settings() noexcept;
    void render_offline(float* out_stereo_pcm, size_t frame_count, uint32_t sample_rate) noexcept;
};

#endif // EAX_NATIVE_CONTEXT_H
