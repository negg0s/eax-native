#include <iostream>
#include <cassert>
#include <vector>
#include <cmath>
#include <spa/support/plugin.h>
#include "eax_pipewire.h"
#include "eax_reverb.h"

extern "C" SPA_EXPORT int spa_handle_factory_enum(const struct spa_handle_factory** factory, uint32_t* index);

int main() {
    std::cout << "[pipewire-test] Starting PipeWire filter integration test...\n";

    eax_context_h ctx = nullptr;
    if (eax_context_create_offline(&ctx, 48000) != EAX_OK) {
        std::cerr << "[-] Failed to create offline eax context!\n";
        return 1;
    }

    // Set CAVE preset
    eax_prop_set(ctx, nullptr, &eax::GUID_EAX20_Listener, 0, nullptr, 0, &eax::PRESET_CAVE, sizeof(eax::PRESET_CAVE));

    // Create 3D buffer
    eax_dsbcaps_t caps{};
    caps.dwSize = sizeof(eax_dsbcaps_t);
    caps.dwFlags = EAX_DSBCAPS_CTRL3D | EAX_DSBCAPS_CTRLVOLUME;
    caps.dwBufferBytes = 48000 * 2; // 1 sec mono 16-bit

    eax_waveformat_ex_t format{};
    format.wFormatTag = 1;
    format.nChannels = 1;
    format.nSamplesPerSec = 48000;
    format.wBitsPerSample = 16;
    format.nBlockAlign = 2;
    format.nAvgBytesPerSec = 48000 * 2;

    eax_buffer_h buf = nullptr;
    if (eax_buffer_create(ctx, &caps, &format, &buf) != EAX_OK) {
        std::cerr << "[-] Failed to create buffer!\n";
        eax_context_destroy(ctx);
        return 1;
    }

    // Fill sine wave
    void *p1 = nullptr, *p2 = nullptr;
    uint32_t b1 = 0, b2 = 0;
    eax_buffer_lock(buf, 0, caps.dwBufferBytes, &p1, &b1, &p2, &b2, 0);
    if (p1) {
        auto* s16 = static_cast<int16_t*>(p1);
        for (size_t i = 0; i < b1 / 2; ++i) {
            s16[i] = static_cast<int16_t>(std::sin(2.0 * 3.14159 * 1000.0 * i / 48000.0) * 20000.0);
        }
    }
    eax_buffer_unlock(buf, p1, b1, p2, b2);

    eax_buffer_set_3d_position(buf, -5.0f, 0.0f, 1.0f, EAX_DS3D_IMMEDIATE);
    eax_buffer_play(buf, 0, EAX_DSBPLAY_LOOPING);

    // Create PipeWire filter node
    eax_pipewire_filter_t* pw_filter = eax_pipewire_filter_create(ctx, "test-eax-filter", 48000, 2);
    if (!pw_filter) {
        std::cerr << "[-] Failed to create PipeWire filter!\n";
        eax_buffer_destroy(buf);
        eax_context_destroy(ctx);
        return 1;
    }

    // Test real-time process block
    const size_t frames = 512;
    std::vector<float> out_stereo(frames * 2, 0.0f);
    int res = eax_pipewire_filter_process_block(pw_filter, nullptr, out_stereo.data(), frames);
    if (res != EAX_PW_OK) {
        std::cerr << "[-] eax_pipewire_filter_process_block failed with " << res << "\n";
        return 1;
    }

    // Check non-zero output rendered
    float sum_abs = 0.0f;
    for (float sample : out_stereo) {
        sum_abs += std::abs(sample);
    }
    std::cout << "[pipewire-test] Rendered 512 frames, sum_abs = " << sum_abs << "\n";
    assert(sum_abs > 0.001f);

    // Test start & stop
    eax_pipewire_filter_start(pw_filter);
    std::cout << "[pipewire-test] Filter running (connected = " << eax_pipewire_filter_is_connected(pw_filter) << ")\n";
    eax_pipewire_filter_stop(pw_filter);

    // Test SPA factory enum
    const struct spa_handle_factory* factory = nullptr;
    uint32_t index = 0;
    int enum_res = spa_handle_factory_enum(&factory, &index);
    if (enum_res <= 0 || !factory) {
        std::cerr << "[-] Failed to enumerate SPA handle factory!\n";
        return 1;
    }
    std::cout << "[pipewire-test] SPA Factory enumerated: " << factory->name << "\n";
    assert(std::string(factory->name) == "audio.filter.eax");

    eax_pipewire_filter_destroy(pw_filter);
    eax_buffer_destroy(buf);
    eax_context_destroy(ctx);

    std::cout << "[pipewire-test] All PipeWire integration tests PASSED successfully!\n";
    return 0;
}
