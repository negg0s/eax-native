#include <iostream>
#include <vector>
#include <cmath>
#include <fstream>
#include <chrono>
#include <thread>
#include <cstdint>
#include <algorithm>
#include "eax_native_abi.h"
#include "eax_reverb.h"

// Optional WAV file writer for headless verification
static bool write_wav_file(const char* filepath, const float* stereo_pcm, size_t frame_count, uint32_t sample_rate) {
    std::ofstream out(filepath, std::ios::binary);
    if (!out) return false;

    const uint32_t num_channels = 2;
    const uint32_t bits_per_sample = 16;
    const uint32_t block_align = num_channels * (bits_per_sample / 8);
    const uint32_t byte_rate = sample_rate * block_align;
    const uint32_t data_bytes = static_cast<uint32_t>(frame_count * block_align);
    const uint32_t chunk_size = 36 + data_bytes;

    out.write("RIFF", 4);
    out.write(reinterpret_cast<const char*>(&chunk_size), 4);
    out.write("WAVE", 4);
    out.write("fmt ", 4);
    uint32_t subchunk1_size = 16;
    uint16_t audio_format = 1;
    uint16_t channels_u16 = static_cast<uint16_t>(num_channels);
    out.write(reinterpret_cast<const char*>(&subchunk1_size), 4);
    out.write(reinterpret_cast<const char*>(&audio_format), 2);
    out.write(reinterpret_cast<const char*>(&channels_u16), 2);
    out.write(reinterpret_cast<const char*>(&sample_rate), 4);
    out.write(reinterpret_cast<const char*>(&byte_rate), 4);
    uint16_t block_align_u16 = static_cast<uint16_t>(block_align);
    out.write(reinterpret_cast<const char*>(&block_align_u16), 2);
    uint16_t bits_u16 = static_cast<uint16_t>(bits_per_sample);
    out.write(reinterpret_cast<const char*>(&bits_u16), 2);
    out.write("data", 4);
    out.write(reinterpret_cast<const char*>(&data_bytes), 4);

    for (size_t i = 0; i < frame_count * 2; ++i) {
        float s = std::clamp(stereo_pcm[i], -1.0f, 1.0f);
        int16_t val = static_cast<int16_t>(s * 32767.0f);
        out.write(reinterpret_cast<const char*>(&val), 2);
    }
    return true;
}

int main(int argc, char** argv) {
    std::cout << "====================================================\n";
    std::cout << "   EAX Native Realtime 3D Audio Demo (Phase 1)     \n";
    std::cout << "====================================================\n";

    eax_context_h ctx = nullptr;
    if (eax_context_create(&ctx) != EAX_OK) {
        std::cerr << "[-] Failed to initialize OpenAL audio backend context!\n";
        return 1;
    }

    // Configure EAX Environment to REVERB_CAVE using named constants from eax_reverb.h
    std::cout << "[*] Configuring OpenAL Soft EAX 2.0 preset: REVERB_CAVE (decay = 2.91s)...\n";
    eax_prop_set(ctx, nullptr, &eax::GUID_EAX20_Listener, 0, nullptr, 0, &eax::PRESET_CAVE, sizeof(eax::PRESET_CAVE));

    // Create 3D Sound Buffer
    const uint32_t sample_rate = 44100;
    const float duration_sec = 0.5f;
    const size_t tone_frames = static_cast<size_t>(sample_rate * duration_sec);
    const size_t buffer_bytes = tone_frames * sizeof(int16_t);

    eax_dsbcaps_t caps{};
    caps.dwSize = sizeof(eax_dsbcaps_t);
    caps.dwFlags = EAX_DSBCAPS_CTRL3D | EAX_DSBCAPS_CTRLVOLUME | EAX_DSBCAPS_CTRLFREQUENCY;
    caps.dwBufferBytes = static_cast<uint32_t>(buffer_bytes);

    eax_waveformat_ex_t format{};
    format.wFormatTag = 1;
    format.nChannels = 1; // mono for 3D spatialization
    format.nSamplesPerSec = sample_rate;
    format.wBitsPerSample = 16;
    format.nBlockAlign = 2;
    format.nAvgBytesPerSec = sample_rate * 2;
    format.cbSize = 0;

    eax_buffer_h buf = nullptr;
    if (eax_buffer_create(ctx, &caps, &format, &buf) != EAX_OK) {
        std::cerr << "[-] Failed to create 3D buffer!\n";
        eax_context_destroy(ctx);
        return 1;
    }

    // Fill buffer with 440 Hz tone pulse
    void *p1 = nullptr, *p2 = nullptr;
    uint32_t b1 = 0, b2 = 0;
    eax_buffer_lock(buf, 0, static_cast<uint32_t>(buffer_bytes), &p1, &b1, &p2, &b2, 0);
    if (p1) {
        auto* s16 = static_cast<int16_t*>(p1);
        const size_t samples = b1 / sizeof(int16_t);
        for (size_t i = 0; i < samples; ++i) {
            float phase = std::fmod(static_cast<float>(i) * 440.0f / static_cast<float>(sample_rate), 1.0f);
            float env = std::sin(3.14159f * static_cast<float>(i) / static_cast<float>(samples));
            s16[i] = static_cast<int16_t>((phase < 0.5f ? 0.8f : -0.8f) * env * 24000.0f);
        }
    }
    eax_buffer_unlock(buf, p1, b1, p2, b2);

    std::cout << "[*] Playing 3D fly-by sound through OpenAL audio device in real time...\n";
    std::cout << "[*] Trajectory: (-20m, 0m, 5m) -> (+20m, 0m, 5m). Listen for stereo panning and cave reverb!\n";

    // Start playback through OpenAL device
    eax_buffer_play(buf, 0, EAX_DSBPLAY_LOOPING);

    const auto start_time = std::chrono::steady_clock::now();
    const float flyby_duration = 3.5f;

    while (true) {
        const auto now = std::chrono::steady_clock::now();
        const float elapsed = std::chrono::duration<float>(now - start_time).count();
        if (elapsed > flyby_duration) {
            break;
        }

        const float progress = elapsed / flyby_duration;
        const float x = -20.0f + progress * 40.0f;
        const float y = 0.0f;
        const float z = 5.0f;

        eax_buffer_set_3d_position(buf, x, y, z, EAX_DS3D_IMMEDIATE);

        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    // Stop source sound to let OpenAL Soft EAX reverb tail ring out
    std::cout << "[*] Sound source stopped; listening to EAX reverb tail decay...\n";
    eax_buffer_stop(buf);

    // Sleep 2.5 seconds to hear the cave reverb tail decaying through the speakers/headphones
    std::this_thread::sleep_for(std::chrono::milliseconds(2500));

    // Optional: write offline rendered copy if user explicitly passed a filepath
    if (argc > 1) {
        std::cout << "[*] Writing offline diagnostic WAV render to " << argv[1] << "...\n";
        const size_t total_frames = sample_rate * 5;
        std::vector<float> pcm(total_frames * 2, 0.0f);
        eax_context_render_offline(ctx, pcm.data(), total_frames, sample_rate);
        write_wav_file(argv[1], pcm.data(), total_frames, sample_rate);
    }

    std::cout << "[+] Realtime OpenAL playback complete.\n";

    eax_buffer_destroy(buf);
    eax_context_destroy(ctx);
    return 0;
}
