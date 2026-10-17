#include <iostream>
#include <vector>
#include <cmath>
#include <fstream>
#include <cstdint>
#include "eax_native_abi.h"

// Minimal 16-bit stereo WAV file writer
static bool write_wav_file(const char* filepath, const float* stereo_pcm, size_t frame_count, uint32_t sample_rate) {
    std::ofstream out(filepath, std::ios::binary);
    if (!out) return false;

    const uint32_t num_channels = 2;
    const uint32_t bits_per_sample = 16;
    const uint32_t block_align = num_channels * (bits_per_sample / 8);
    const uint32_t byte_rate = sample_rate * block_align;
    const uint32_t data_bytes = static_cast<uint32_t>(frame_count * block_align);
    const uint32_t chunk_size = 36 + data_bytes;

    // RIFF header
    out.write("RIFF", 4);
    out.write(reinterpret_cast<const char*>(&chunk_size), 4);
    out.write("WAVE", 4);

    // fmt subchunk
    out.write("fmt ", 4);
    uint32_t subchunk1_size = 16;
    uint16_t audio_format = 1; // PCM
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

    // data subchunk
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
    std::cout << "   EAX Native 3D Audio & Reverb Demo (Phase 1)     \n";
    std::cout << "====================================================\n";

    eax_context_h ctx = nullptr;
    if (eax_context_create(&ctx) != EAX_OK) {
        std::cerr << "Failed to create EAX context!\n";
        return 1;
    }

    // Configure EAX Environment to REVERB_CAVE
    std::cout << "[*] Configuring EAX 2.0 preset: REVERB_CAVE (decay = 2.91s)...\n";
    eax20_listener_properties_t cave_props = {
        -1000, 0, 0.0f, 2.91f, 1.30f, -602, 0.015f, -302, 0.022f,
        EAX_ENVIRONMENT_CAVE, 14.6f, 1.0f, -5.0f, 0x3F
    };
    eax_guid_t guid_eax20_listener = {
        0x0306A6A8, 0xB224, 0x11D2, {0x99, 0xE5, 0x00, 0x00, 0xE8, 0xD8, 0xC7, 0x22}
    };
    eax_prop_set(ctx, nullptr, &guid_eax20_listener, 0, nullptr, 0, &cave_props, sizeof(cave_props));

    // Create 3D Sound Buffer
    const uint32_t sample_rate = 44100;
    const float duration_sec = 1.0f;
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
        std::cerr << "Failed to create 3D buffer!\n";
        eax_context_destroy(ctx);
        return 1;
    }

    // Fill buffer with 440 Hz pulse wave
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

    // Simulate 3D Fly-by from (-20, 0, 5) to (+20, 0, 5) over 4 seconds
    std::cout << "[*] Simulating 3D fly-by trajectory (left -> right) with cave reverb tail...\n";
    const size_t total_render_seconds = 6; // 4s sound + 2s reverb tail
    const size_t total_frames = total_render_seconds * sample_rate;
    std::vector<float> output_pcm(total_frames * 2, 0.0f);

    // Start playing
    eax_buffer_play(buf, 0, EAX_DSBPLAY_LOOPING);

    const size_t chunk_size = 512;
    size_t rendered_frames = 0;

    while (rendered_frames < total_frames) {
        const size_t current_chunk = std::min(chunk_size, total_frames - rendered_frames);
        const float time_sec = static_cast<float>(rendered_frames) / static_cast<float>(sample_rate);

        // Update 3D position
        if (time_sec <= 4.0f) {
            const float progress = time_sec / 4.0f; // [0, 1]
            const float x = -20.0f + progress * 40.0f;
            const float y = 0.0f;
            const float z = 5.0f;
            eax_buffer_set_3d_position(buf, x, y, z, EAX_DS3D_IMMEDIATE);
        } else {
            // Stop buffer to let reverb tail decay naturally
            eax_buffer_stop(buf);
        }

        eax_context_render_offline(ctx, output_pcm.data() + rendered_frames * 2, current_chunk, sample_rate);
        rendered_frames += current_chunk;
    }

    const char* output_file = (argc > 1) ? argv[1] : "eax_flyby_cave_demo.wav";
    std::cout << "[*] Writing rendered output to " << output_file << "...\n";
    if (write_wav_file(output_file, output_pcm.data(), total_frames, sample_rate)) {
        std::cout << "[+] Demo audio generated successfully (" << total_render_seconds << "s, 44.1kHz 16-bit stereo)!\n";
    } else {
        std::cerr << "[-] Failed to write output WAV file!\n";
    }

    eax_buffer_destroy(buf);
    eax_context_destroy(ctx);
    std::cout << "[+] Done.\n";
    return 0;
}
