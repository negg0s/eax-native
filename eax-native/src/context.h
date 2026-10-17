#ifndef EAX_NATIVE_CONTEXT_H
#define EAX_NATIVE_CONTEXT_H

#include <vector>
#include <mutex>
#include "eax_native_abi.h"
#include "arena.h"
#include "voice_pool.h"
#include "spatializer.h"
#include "eax_reverb.h"

struct eax_context_s {
    eax::MemoryArena arena{eax::MemoryArena::DEFAULT_ARENA_SIZE};
    eax::VoicePool voice_pool{};

    // Listener State
    eax_ds3dlistener_t listener{};
    eax_ds3dlistener_t listener_deferred{};
    bool listener_has_deferred{false};

    // EAX Reverb State
    eax20_listener_properties_t eax_listener{};
    eax20_listener_properties_t eax_listener_deferred{};
    bool eax_listener_has_deferred{false};

    eax::ReverbDSP reverb_dsp{};
    uint32_t speaker_config{EAX_SPEAKER_CONFIG_HEADPHONE};

    // Synchronization for non-hot-path configuration
    std::mutex config_mutex{};

    eax_context_s() noexcept;
    void commit_deferred_settings() noexcept;
    void render_offline(float* out_stereo_pcm, size_t frame_count, uint32_t sample_rate) noexcept;
};

#endif // EAX_NATIVE_CONTEXT_H
