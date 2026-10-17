#include "catch_amalgamated.hpp"
#include "eax_native_abi.h"
#include "context.h"
#include "voice_pool.h"
#include "eax_reverb.h"
#include <vector>
#include <cmath>
#include <AL/al.h>

static float calculate_rms(const float* samples, size_t count, size_t stride) {
    if (count == 0) return 0.0f;
    double sum_sq = 0.0;
    for (size_t i = 0; i < count; ++i) {
        double val = static_cast<double>(samples[i * stride]);
        sum_sq += val * val;
    }
    return static_cast<float>(std::sqrt(sum_sq / static_cast<double>(count)));
}

TEST_CASE("3D Spatial Panning RMS: 1 kHz Sine Left/Right Ratio via OpenAL Soft", "[pan_rms]") {
    const uint32_t sample_rate = 44100;
    eax_context_h ctx = nullptr;
    REQUIRE(eax_context_create_offline(&ctx, sample_rate) == EAX_OK);
    REQUIRE(ctx != nullptr);
    REQUIRE(ctx->m_al_context != nullptr);
    REQUIRE(ctx->m_is_loopback);
    REQUIRE(eax::get_al_ext().alcRenderSamplesSOFT != nullptr);
    const float duration_sec = 0.5f;
    const size_t frame_count = static_cast<size_t>(sample_rate * duration_sec);
    const size_t buffer_bytes = frame_count * sizeof(int16_t);

    eax_dsbcaps_t caps{};
    caps.dwSize = sizeof(eax_dsbcaps_t);
    caps.dwFlags = EAX_DSBCAPS_CTRL3D | EAX_DSBCAPS_CTRLVOLUME | EAX_DSBCAPS_CTRLFREQUENCY;
    caps.dwBufferBytes = static_cast<uint32_t>(buffer_bytes);

    eax_waveformat_ex_t format{};
    format.wFormatTag = 1;
    format.nChannels = 1; // mono
    format.nSamplesPerSec = sample_rate;
    format.wBitsPerSample = 16;
    format.nBlockAlign = 2;
    format.nAvgBytesPerSec = sample_rate * 2;
    format.cbSize = 0;

    eax_buffer_h buf = nullptr;
    REQUIRE(eax_buffer_create(ctx, &caps, &format, &buf) == EAX_OK);
    REQUIRE(buf->al_source != 0);
    REQUIRE(buf->al_buffer != 0);

    // Fill buffer with pure 1 kHz sine wave
    void *p1 = nullptr, *p2 = nullptr;
    uint32_t b1 = 0, b2 = 0;
    REQUIRE(eax_buffer_lock(buf, 0, static_cast<uint32_t>(buffer_bytes), &p1, &b1, &p2, &b2, 0) == EAX_OK);
    auto* s16 = static_cast<int16_t*>(p1);
    const size_t total_samples = b1 / sizeof(int16_t);
    for (size_t i = 0; i < total_samples; ++i) {
        float phase = 2.0f * 3.1415926535f * 1000.0f * static_cast<float>(i) / static_cast<float>(sample_rate);
        s16[i] = static_cast<int16_t>(std::sin(phase) * 20000.0f);
    }
    REQUIRE(eax_buffer_unlock(buf, p1, b1, p2, b2) == EAX_OK);

    // Set dry environment so spatial panning ratios are measured without diffuse reverb interference
    eax20_listener_properties_t dry_env = eax::PRESET_GENERIC;
    dry_env.lRoom = -10000;
    dry_env.lRoomHF = -10000;
    REQUIRE(eax_prop_set(ctx, nullptr, &eax::GUID_EAX20_Listener, 0, nullptr, 0, &dry_env, sizeof(dry_env)) == EAX_OK);

    std::vector<float> output_pcm(frame_count * 2, 0.0f);

    SECTION("Position Full Left (-10, 0, 0): OpenAL source pos and RMS L >> RMS R") {
        REQUIRE(eax_buffer_set_3d_position(buf, -10.0f, 0.0f, 0.0f, EAX_DS3D_IMMEDIATE) == EAX_OK);

        // Verify OpenAL source position matches
        float al_pos[3] = { 0.0f };
        alGetSource3f(buf->al_source, AL_POSITION, &al_pos[0], &al_pos[1], &al_pos[2]);
        REQUIRE(al_pos[0] == Catch::Approx(-10.0f));

        REQUIRE(eax_buffer_set_current_position(buf, 0) == EAX_OK);
        REQUIRE(eax_buffer_play(buf, 0, EAX_DSBPLAY_LOOPING) == EAX_OK);

        ALint al_state = 0;
        alGetSourcei(buf->al_source, AL_SOURCE_STATE, &al_state);
        REQUIRE(al_state == AL_PLAYING);

        REQUIRE(eax_context_render_offline(ctx, output_pcm.data(), frame_count, sample_rate) == EAX_OK);
        REQUIRE(eax_buffer_stop(buf) == EAX_OK);

        const float rms_left = calculate_rms(output_pcm.data(), frame_count, 2);
        const float rms_right = calculate_rms(output_pcm.data() + 1, frame_count, 2);

        REQUIRE(rms_left > 0.01f);
        REQUIRE(rms_left / (rms_right + 0.00001f) > 5.0f);
    }

    SECTION("Position Full Right (+10, 0, 0): OpenAL source pos and RMS R >> RMS L") {
        REQUIRE(eax_buffer_set_3d_position(buf, 10.0f, 0.0f, 0.0f, EAX_DS3D_IMMEDIATE) == EAX_OK);

        float al_pos[3] = { 0.0f };
        alGetSource3f(buf->al_source, AL_POSITION, &al_pos[0], &al_pos[1], &al_pos[2]);
        REQUIRE(al_pos[0] == Catch::Approx(10.0f));

        REQUIRE(eax_buffer_set_current_position(buf, 0) == EAX_OK);
        REQUIRE(eax_buffer_play(buf, 0, EAX_DSBPLAY_LOOPING) == EAX_OK);

        REQUIRE(eax_context_render_offline(ctx, output_pcm.data(), frame_count, sample_rate) == EAX_OK);
        REQUIRE(eax_buffer_stop(buf) == EAX_OK);

        const float rms_left = calculate_rms(output_pcm.data(), frame_count, 2);
        const float rms_right = calculate_rms(output_pcm.data() + 1, frame_count, 2);

        REQUIRE(rms_right > 0.01f);
        REQUIRE(rms_right / (rms_left + 0.00001f) > 5.0f);
    }

    SECTION("Position Center Forward (0, 0, 10): OpenAL source pos and RMS L ~= RMS R") {
        REQUIRE(eax_buffer_set_3d_position(buf, 0.0f, 0.0f, 10.0f, EAX_DS3D_IMMEDIATE) == EAX_OK);

        // DS +Z forward maps to OpenAL -Z forward
        float al_pos[3] = { 0.0f };
        alGetSource3f(buf->al_source, AL_POSITION, &al_pos[0], &al_pos[1], &al_pos[2]);
        REQUIRE(al_pos[2] == Catch::Approx(-10.0f));

        REQUIRE(eax_buffer_set_current_position(buf, 0) == EAX_OK);
        REQUIRE(eax_buffer_play(buf, 0, EAX_DSBPLAY_LOOPING) == EAX_OK);

        REQUIRE(eax_context_render_offline(ctx, output_pcm.data(), frame_count, sample_rate) == EAX_OK);
        REQUIRE(eax_buffer_stop(buf) == EAX_OK);

        const float rms_left = calculate_rms(output_pcm.data(), frame_count, 2);
        const float rms_right = calculate_rms(output_pcm.data() + 1, frame_count, 2);

        REQUIRE(rms_left > 0.01f);
        REQUIRE(std::abs(rms_left - rms_right) / rms_left < 0.05f);
    }

    REQUIRE(eax_buffer_destroy(buf) == EAX_OK);
    REQUIRE(eax_context_destroy(ctx) == EAX_OK);
}
