#include "catch_amalgamated.hpp"
#include "eax_native_abi.h"
#include "context.h"
#include "eax_reverb.h"
#include <vector>
#include <cmath>

static float calculate_rms_slice(const float* stereo_pcm, size_t start_frame, size_t count) {
    if (count == 0) return 0.0f;
    double sum_sq = 0.0;
    for (size_t i = 0; i < count; ++i) {
        float l = stereo_pcm[(start_frame + i) * 2];
        float r = stereo_pcm[(start_frame + i) * 2 + 1];
        sum_sq += static_cast<double>(l * l + r * r) * 0.5;
    }
    return static_cast<float>(std::sqrt(sum_sq / static_cast<double>(count)));
}

TEST_CASE("EAX Reverb Tail: REVERB_CAVE Preset Extended Decay via OpenAL Soft", "[reverb]") {
    const uint32_t sample_rate = 44100;
    eax_context_h ctx = nullptr;
    REQUIRE(eax_context_create_offline(&ctx, sample_rate) == EAX_OK);
    REQUIRE(ctx != nullptr);
    REQUIRE(ctx->m_al_context != nullptr);
    REQUIRE(ctx->m_is_loopback);
    REQUIRE(eax::get_al_ext().alcRenderSamplesSOFT != nullptr);

    auto& ext = eax::get_al_ext();
    if (ext.has_efx) {
        REQUIRE(ctx->m_eax_slot != 0);
        REQUIRE(ctx->m_eax_effect != 0);
    }

    const float burst_duration = 0.03f; // 30 ms impulse tone
    const size_t burst_frames = static_cast<size_t>(sample_rate * burst_duration);
    const size_t buffer_bytes = burst_frames * sizeof(int16_t);

    eax_dsbcaps_t caps{};
    caps.dwSize = sizeof(eax_dsbcaps_t);
    caps.dwFlags = EAX_DSBCAPS_CTRL3D | EAX_DSBCAPS_CTRLVOLUME;
    caps.dwBufferBytes = static_cast<uint32_t>(buffer_bytes);

    eax_waveformat_ex_t format{};
    format.wFormatTag = 1;
    format.nChannels = 1;
    format.nSamplesPerSec = sample_rate;
    format.wBitsPerSample = 16;
    format.nBlockAlign = 2;
    format.nAvgBytesPerSec = sample_rate * 2;
    format.cbSize = 0;

    eax_buffer_h buf = nullptr;
    REQUIRE(eax_buffer_create(ctx, &caps, &format, &buf) == EAX_OK);

    void *p1 = nullptr, *p2 = nullptr;
    uint32_t b1 = 0, b2 = 0;
    REQUIRE(eax_buffer_lock(buf, 0, static_cast<uint32_t>(buffer_bytes), &p1, &b1, &p2, &b2, 0) == EAX_OK);
    auto* s16 = static_cast<int16_t*>(p1);
    for (size_t i = 0; i < burst_frames; ++i) {
        float phase = 2.0f * 3.1415926535f * 1000.0f * static_cast<float>(i) / static_cast<float>(sample_rate);
        s16[i] = static_cast<int16_t>(std::sin(phase) * 30000.0f);
    }
    REQUIRE(eax_buffer_unlock(buf, p1, b1, p2, b2) == EAX_OK);

    REQUIRE(eax_prop_set(ctx, nullptr, &eax::GUID_EAX20_Listener, 1, nullptr, 0, &eax::PRESET_CAVE, sizeof(eax::PRESET_CAVE)) == EAX_OK);

    // Play once (non-looping), so dry sound terminates after 30 ms
    REQUIRE(eax_buffer_play(buf, 0, 0) == EAX_OK);

    const size_t total_render_frames = sample_rate * 1; // 1.0 second
    std::vector<float> pcm_with_reverb(total_render_frames * 2, 0.0f);
    REQUIRE(eax_context_render_offline(ctx, pcm_with_reverb.data(), total_render_frames, sample_rate) == EAX_OK);

    // Measure dry burst RMS [0..30ms] and reverb tail RMS [100ms..600ms]
    const size_t tail_start_frame = static_cast<size_t>(sample_rate * 0.10f); // 100 ms
    const size_t tail_frame_count = static_cast<size_t>(sample_rate * 0.50f); // 500 ms window

    const float burst_rms = calculate_rms_slice(pcm_with_reverb.data(), 0, burst_frames);
    const float cave_tail_rms = calculate_rms_slice(pcm_with_reverb.data(), tail_start_frame, tail_frame_count);

    REQUIRE(burst_rms > 0.1f);
    REQUIRE(cave_tail_rms > 0.001f);

    // Contrast with disabled reverb (dry room: lRoom = -10000 mB)
    eax20_listener_properties_t dry_props = eax::PRESET_CAVE;
    dry_props.lRoom = -10000;
    REQUIRE(eax_prop_set(ctx, nullptr, &eax::GUID_EAX20_Listener, 1, nullptr, 0, &dry_props, sizeof(dry_props)) == EAX_OK);

    REQUIRE(eax_buffer_set_current_position(buf, 0) == EAX_OK);
    REQUIRE(eax_buffer_play(buf, 0, 0) == EAX_OK);

    std::vector<float> pcm_dry(total_render_frames * 2, 0.0f);
    REQUIRE(eax_context_render_offline(ctx, pcm_dry.data(), total_render_frames, sample_rate) == EAX_OK);
    const float dry_tail_rms = calculate_rms_slice(pcm_dry.data(), tail_start_frame, tail_frame_count);

    // Reverb tail in cave must be significantly higher than dry (9.5x measured, window [100;600] ms, decay 2.91 s)
    REQUIRE(cave_tail_rms > dry_tail_rms * 5.0f);

    REQUIRE(eax_buffer_destroy(buf) == EAX_OK);
    REQUIRE(eax_context_destroy(ctx) == EAX_OK);
}
