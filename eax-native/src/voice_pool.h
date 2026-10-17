#ifndef EAX_NATIVE_VOICE_POOL_H
#define EAX_NATIVE_VOICE_POOL_H

#include <cstdint>
#include <atomic>
#include <array>
#include "eax_native_abi.h"
#include "arena.h"
#include "openal_bridge.h"
#include "seqlock.h"

struct eax_context_s;

struct eax_buffer_s {
    std::atomic<bool> in_use{false};
    uint32_t id{0};
    eax_context_s* context{nullptr};

    eax_dsbcaps_t caps{};
    eax_waveformat_ex_t format{};

    // OpenAL objects
    ALuint al_source{0};
    ALuint al_buffer{0};
    ALuint al_direct_filter{0};
    ALuint al_send_filter{0};

    // Volume, Pan, Frequency
    std::atomic<int32_t> volume_mb{0};
    std::atomic<int32_t> pan_mb{0};
    std::atomic<uint32_t> frequency_hz{44100};

    // Playback state
    std::atomic<uint32_t> status{0};
    std::atomic<bool> is_looping{false};
    std::atomic<double> playback_cursor{0.0};

    // Sample data buffer (backed by pre-allocated arena)
    eax::CircularSampleBuffer sample_buffer{};

    // DirectSound3D parameters with lock-free SeqLock for RT tearing-free snapshot reads
    eax::SeqLock ds3d_seqlock{};
    eax_ds3dbuffer_t ds3d{};
    eax_ds3dbuffer_t ds3d_deferred{};
    bool ds3d_has_deferred{false};

    [[nodiscard]] eax_ds3dbuffer_t get_ds3d_snapshot() const noexcept {
        eax_ds3dbuffer_t snap;
        uint32_t s;
        do {
            s = ds3d_seqlock.read_begin();
            snap = ds3d;
        } while (ds3d_seqlock.read_retry(s));
        return snap;
    }

    // EAX 2.0 buffer properties
    eax20_buffer_properties_t eax20{};
    eax20_buffer_properties_t eax20_deferred{};
    bool eax20_has_deferred{false};

    void init_openal(eax_context_s* ctx) noexcept;
    void destroy_openal() noexcept;
    void reset() noexcept;
    void sync_to_openal() noexcept;
    void upload_sample_data() noexcept;
    void read_samples(float* out_left, float* out_right, size_t frame_count, uint32_t output_sample_rate) noexcept;
};

namespace eax {

class VoicePool {
public:
    static constexpr size_t MAX_VOICES = 64;

    VoicePool() noexcept;
    ~VoicePool() noexcept;

    void init(eax_context_s* ctx) noexcept;
    [[nodiscard]] eax_buffer_s* allocate_voice() noexcept;
    void release_voice(eax_buffer_s* voice) noexcept;

    [[nodiscard]] size_t active_voice_count() const noexcept;
    [[nodiscard]] std::array<eax_buffer_s, MAX_VOICES>& get_all_voices() noexcept { return m_voices; }

private:
    std::array<eax_buffer_s, MAX_VOICES> m_voices;
    eax_context_s* m_context{nullptr};
};

} // namespace eax

#endif // EAX_NATIVE_VOICE_POOL_H
