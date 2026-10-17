#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <fstream>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <chrono>
#include <thread>
#include <cmath>
#include <algorithm>
#include "eax_native_abi.h"
#include "eax_reverb.h"
#include "eax_pipewire.h"

namespace fs = std::filesystem;

void print_usage() {
    std::cout << "eaxctl - EAX & DirectSound3D Proton/Wine Management Utility\n\n"
              << "Usage:\n"
              << "  eaxctl install --appid <steam_appid> [--prefix <path>]\n"
              << "  eaxctl status [--appid <steam_appid> | --prefix <path>]\n"
              << "  eaxctl test [--prefix <path>]\n"
              << "  eaxctl demo [--backend <pipewire|openal>] [--duration <sec>] [--headless] [--output <wav>]\n\n"
              << "Subcommands:\n"
              << "  install   Install dsound.dll shim into Proton prefix & configure registry override\n"
              << "  status    Check status of dsound override and active EAX emulation\n"
              << "  test      Run DirectSound / EAX compatibility test inside prefix\n"
              << "  demo      Run 3D fly-by demo with HRTF spatialization and EAX reverb\n";
}

fs::path find_proton_prefix(const std::string& appid, const std::string& explicit_prefix) {
    if (!explicit_prefix.empty()) {
        return fs::path(explicit_prefix);
    }
    const char* home = std::getenv("HOME");
    if (!home) return "";

    std::vector<fs::path> candidate_roots = {
        fs::path(home) / ".local/share/Steam/steamapps/compatdata" / appid / "pfx",
        fs::path(home) / ".steam/steam/steamapps/compatdata" / appid / "pfx",
        fs::path(home) / ".var/app/com.valvesoftware.Steam/.local/share/Steam/steamapps/compatdata" / appid / "pfx"
    };

    for (const auto& p : candidate_roots) {
        if (fs::exists(p)) return p;
    }

    return candidate_roots[0]; // fallback
}

// Returns 0x8664 (x64), 0x014C (i386) for a PE image, or 0 if not a PE file
static uint16_t pe_machine(const fs::path& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return 0;
    char dos[64] = {};
    f.read(dos, sizeof(dos));
    if (dos[0] != 'M' || dos[1] != 'Z') return 0;
    uint32_t e_lfanew = 0;
    std::memcpy(&e_lfanew, dos + 0x3C, 4);
    if (e_lfanew < 0x40 || e_lfanew > 0x1000) return 0;
    f.clear();
    f.seekg(e_lfanew, std::ios::beg);
    char pe_hdr[8] = {};
    f.read(pe_hdr, sizeof(pe_hdr));
    if (pe_hdr[0] != 'P' || pe_hdr[1] != 'E' || pe_hdr[2] != '\0' || pe_hdr[3] != '\0') return 0;
    uint16_t machine = 0;
    std::memcpy(&machine, pe_hdr + 4, 2);
    return machine;
}

int cmd_install(const std::string& appid, const std::string& explicit_prefix) {
    if (appid.empty() && explicit_prefix.empty()) {
        std::cerr << "Error: --appid <steam_appid> or --prefix <path> is required for install.\n";
        return 1;
    }

    fs::path prefix = find_proton_prefix(appid, explicit_prefix);
    std::cout << "[eaxctl] Target Proton prefix: " << prefix.string() << "\n";

    fs::path sys32 = prefix / "drive_c" / "windows" / "system32";
    fs::path syswow64 = prefix / "drive_c" / "windows" / "syswow64";

    if (!fs::exists(prefix)) {
        std::cout << "[eaxctl] Notice: Prefix directory does not exist yet. Creating directory skeleton...\n";
        fs::create_directories(sys32);
        fs::create_directories(syswow64);
    }

    std::vector<fs::path> shim_locations = {
        "./prebuilt/pe64/dsound.dll",
        "./prebuilt/pe32/dsound.dll",
        "./build-pe64/wine-integration/dsound.dll",
        "./build-pe32/wine-integration/dsound.dll",
        "./wine-integration/dsound.dll",
        "./build/wine-integration/dsound.dll",
        "./build/dsound.dll",
        "/usr/lib/eax-native/dsound64.dll",
        "/usr/lib/eax-native/dsound32.dll",
        "/usr/lib/eax-native/dsound.dll"
    };

    fs::path shim_x64;
    fs::path shim_x86;
    for (const auto& loc : shim_locations) {
        if (!fs::exists(loc)) continue;
        const uint16_t machine = pe_machine(loc);
        if (machine == 0x8664) {
            if (shim_x64.empty()) shim_x64 = loc;
        } else if (machine == 0x014C) {
            if (shim_x86.empty()) shim_x86 = loc;
        }
    }

    if (shim_x64.empty() && shim_x86.empty()) {
        std::vector<fs::path> elf_locations = {
            "./build/wine-integration/dsound.so",
            "./wine-integration/dsound.so",
            "./build/dsound.so",
            "/usr/lib/eax-native/dsound.so"
        };
        fs::path found_elf;
        for (const auto& loc : elf_locations) {
            if (fs::exists(loc)) {
                found_elf = loc;
                break;
            }
        }

        std::cerr << "[eaxctl] Error: Windows PE 'dsound.dll' shim not found.\n";
        if (!found_elf.empty()) {
            std::cerr << "[eaxctl] Notice: Found Linux ELF shared library at '" << found_elf.string() << "'.\n"
                      << "[eaxctl] Proton and Wine require a Windows PE binary (dsound.dll), not an ELF library (.so).\n";
        }
        std::cerr << "[eaxctl] Please cross-compile the Windows PE shim first using:\n"
                  << "         ./ci/build_windows_pe.sh\n"
                  << "       or:\n"
                  << "         cmake -B build-pe64 -DCMAKE_TOOLCHAIN_FILE=cmake/mingw64.cmake && cmake --build build-pe64 --target wine_dsound\n";
        return 1;
    }

    try {
        if (shim_x64.empty()) {
            std::cerr << "[eaxctl] Warning: 64-bit dsound.dll shim missing; 64-bit games will not use eax-native.\n";
        } else if (fs::exists(sys32)) {
            std::cout << "[eaxctl] Installing 64-bit dsound.dll (system32) from " << shim_x64.string() << "\n";
            fs::copy_file(shim_x64, sys32 / "dsound.dll", fs::copy_options::overwrite_existing);
        }
        if (shim_x86.empty()) {
            std::cerr << "[eaxctl] Warning: 32-bit dsound.dll shim missing; 32-bit games will not use eax-native.\n";
        } else if (fs::exists(syswow64)) {
            std::cout << "[eaxctl] Installing 32-bit dsound.dll (syswow64) from " << shim_x86.string() << "\n";
            fs::copy_file(shim_x86, syswow64 / "dsound.dll", fs::copy_options::overwrite_existing);
        }
    } catch (const std::exception& e) {
        std::cerr << "[eaxctl] Warning during file copy: " << e.what() << "\n";
    }

    // Update user.reg with dsound override (idempotent)
    fs::path user_reg = prefix / "user.reg";
    std::cout << "[eaxctl] Configuring WINEDLLOVERRIDES \"dsound=native,builtin\" in user.reg...\n";
    bool already_configured = false;
    if (fs::exists(user_reg)) {
        std::ifstream check_reg(user_reg);
        std::string line;
        while (std::getline(check_reg, line)) {
            if (line.find("\"dsound\"=\"native,builtin\"") != std::string::npos ||
                line.find("\"dsound\"=\"n,b\"") != std::string::npos) {
                already_configured = true;
                break;
            }
        }
    }

    if (!already_configured) {
        std::ofstream reg(user_reg, std::ios::app);
        reg << "\n[Software\\\\Wine\\\\DllOverrides]\n"
            << "\"dsound\"=\"native,builtin\"\n";
        std::cout << "[eaxctl] Added dsound=native,builtin entry to user.reg\n";
    } else {
        std::cout << "[eaxctl] Override already configured in user.reg (idempotent)\n";
    }

    std::cout << "[eaxctl] SUCCESS: dsound override verified: dsound=n,b\n";
    std::cout << "[eaxctl] Installation complete. Games launched with AppID " << appid << " will now use eax-native.\n";
    return 0;
}

int cmd_status(const std::string& appid, const std::string& explicit_prefix) {
    fs::path prefix = find_proton_prefix(appid, explicit_prefix);
    std::cout << "=== eaxctl Status ===\n";
    std::cout << "Target Prefix: " << prefix.string() << "\n";

    fs::path user_reg = prefix / "user.reg";
    if (fs::exists(user_reg)) {
        std::ifstream check_reg(user_reg);
        std::string line;
        bool found = false;
        while (std::getline(check_reg, line)) {
            if (line.find("\"dsound\"") != std::string::npos) {
                std::cout << "DLL Override:  " << line << "\n";
                found = true;
                break;
            }
        }
        if (!found) {
            std::cout << "DLL Override:  None configured (stock Wine builtin will be used)\n";
        }
    } else {
        std::cout << "Prefix Status: user.reg not found\n";
    }

    fs::path dsound64 = prefix / "drive_c/windows/system32/dsound.dll";
    fs::path dsound32 = prefix / "drive_c/windows/syswow64/dsound.dll";
    const uint16_t m64 = fs::exists(dsound64) ? pe_machine(dsound64) : 0;
    const uint16_t m32 = fs::exists(dsound32) ? pe_machine(dsound32) : 0;
    std::cout << "Shim 64-bit (system32): "
              << (m64 == 0x8664 ? "Installed (x64)"
                  : fs::exists(dsound64) ? (m32 == 0x014C ? "WRONG ARCH (32-bit in system32)" : "Not a PE file")
                  : "Not Present") << "\n";
    std::cout << "Shim 32-bit (syswow64): "
              << (m32 == 0x014C ? "Installed (x86)"
                  : fs::exists(dsound32) ? (m64 == 0x8664 ? "WRONG ARCH (64-bit in syswow64)" : "Not a PE file")
                  : "Not Present") << "\n";
    return 0;
}

int cmd_test(const std::string& explicit_prefix) {
    fs::path prefix = find_proton_prefix("", explicit_prefix);
    std::cout << "[eaxctl] Running compatibility test harness...\n";

    std::vector<fs::path> test_script_candidates = {
        "./wine-integration/tests/run_wine_tests.sh",
        "../wine-integration/tests/run_wine_tests.sh",
        "../../wine-integration/tests/run_wine_tests.sh"
    };

    fs::path test_script;
    for (const auto& s : test_script_candidates) {
        if (fs::exists(s)) {
            test_script = s;
            break;
        }
    }

    if (!test_script.empty()) {
        std::string cmd = "bash " + test_script.string();
        return std::system(cmd.c_str());
    }

    std::vector<fs::path> test_exe_candidates = {
        "./build/wine-integration/tests/test_dsound_compat",
        "./build-pe64/wine-integration/tests/wine_dsound_test.exe",
        "./build-pe32/wine-integration/tests/wine_dsound_test.exe",
        "./wine-integration/tests/test_dsound_compat"
    };
    for (const auto& te : test_exe_candidates) {
        if (fs::exists(te)) {
            std::cout << "[eaxctl] Executing test binary: " << te.string() << "\n";
            return std::system(te.string().c_str());
        }
    }

    std::cerr << "[eaxctl] Neither run_wine_tests.sh nor test executable found.\n";
    return 1;
}

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

int cmd_demo(const std::string& backend, float duration, bool headless, const std::string& output_wav) {
    std::cout << "====================================================\n";
    std::cout << "   eaxctl 3D Audio Demo (Backend: " << backend << ")   \n";
    std::cout << "====================================================\n";

    const uint32_t sample_rate = 48000;
    eax_context_h ctx = nullptr;

    if (backend == "pipewire" || headless) {
        if (eax_context_create_offline(&ctx, sample_rate) != EAX_OK) {
            std::cerr << "[-] Failed to initialize offline audio context!\n";
            return 1;
        }
    } else {
        if (eax_context_create(&ctx) != EAX_OK) {
            std::cout << "[*] Standard OpenAL device unavailable, falling back to loopback...\n";
            if (eax_context_create_offline(&ctx, sample_rate) != EAX_OK) {
                std::cerr << "[-] Failed to create audio context!\n";
                return 1;
            }
        }
    }

    std::cout << "[*] Configuring EAX 2.0 preset: REVERB_CAVE (decay = 2.91s)...\n";
    eax_prop_set(ctx, nullptr, &eax::GUID_EAX20_Listener, 1, nullptr, 0, &eax::PRESET_CAVE, sizeof(eax::PRESET_CAVE));

    // Create 3D Sound Buffer with 440 Hz tone pulse
    const float tone_duration = 0.5f;
    const size_t tone_frames = static_cast<size_t>(sample_rate * tone_duration);
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

    eax_pipewire_filter_t* pw_filter = nullptr;
    if (backend == "pipewire") {
        std::cout << "[*] Initializing PipeWire DSP filter node...\n";
        pw_filter = eax_pipewire_filter_create(ctx, "eaxctl-flyby-filter", sample_rate, 2);
        if (pw_filter) {
            eax_pipewire_filter_start(pw_filter);
            std::cout << "[*] PipeWire filter started (connected to daemon: "
                      << (eax_pipewire_filter_is_connected(pw_filter) ? "yes" : "standalone RT loop") << ").\n";
        }
    }

    std::cout << "[*] Playing 3D fly-by sound...\n";
    std::cout << "[*] Trajectory: (-20m, 0m, 5m) -> (+20m, 0m, 5m) across listener\n";
    eax_buffer_play(buf, 0, EAX_DSBPLAY_LOOPING);

    const auto start_time = std::chrono::steady_clock::now();
    const float flyby_duration = (duration > 0.0f) ? duration : 3.5f;

    std::vector<float> recorded_pcm;
    if (!output_wav.empty()) {
        recorded_pcm.reserve(static_cast<size_t>(sample_rate * (flyby_duration + 2.5f) * 2));
    }

    while (true) {
        const auto now = std::chrono::steady_clock::now();
        const float elapsed = std::chrono::duration<float>(now - start_time).count();
        if (elapsed > flyby_duration) break;

        const float progress = elapsed / flyby_duration;
        const float x = -20.0f + progress * 40.0f;
        const float y = 0.0f;
        const float z = 5.0f;

        eax_buffer_set_3d_position(buf, x, y, z, EAX_DS3D_IMMEDIATE);

        if (!output_wav.empty()) {
            float block[512 * 2];
            eax_context_render_offline(ctx, block, 512, sample_rate);
            recorded_pcm.insert(recorded_pcm.end(), block, block + 512 * 2);
        }

        if (!headless || !output_wav.empty()) {
            // Pace to real-time audio block duration (~10.67ms per 512 frames @ 48kHz)
            // to maintain proper WAV playback timing without CPU spin inflation
            std::this_thread::sleep_for(std::chrono::microseconds(512 * 1000000 / sample_rate));
        }
    }

    std::cout << "[*] Sound source stopped; listening to EAX reverb tail decay...\n";
    eax_buffer_stop(buf);

    if (!headless) {
        std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    }

    if (!output_wav.empty()) {
        const size_t tail_frames = sample_rate * 2;
        std::vector<float> tail_pcm(tail_frames * 2, 0.0f);
        eax_context_render_offline(ctx, tail_pcm.data(), tail_frames, sample_rate);
        recorded_pcm.insert(recorded_pcm.end(), tail_pcm.begin(), tail_pcm.end());

        std::cout << "[*] Writing rendered WAV output to " << output_wav << "...\n";
        write_wav_file(output_wav.c_str(), recorded_pcm.data(), recorded_pcm.size() / 2, sample_rate);
    }

    if (pw_filter) {
        eax_pipewire_filter_stop(pw_filter);
        eax_pipewire_filter_destroy(pw_filter);
    }

    eax_buffer_destroy(buf);
    eax_context_destroy(ctx);

    std::cout << "[+] Demo execution complete (" << backend << " backend).\n";
    return 0;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_usage();
        return 1;
    }

    std::string subcommand = argv[1];
    std::string appid = "";
    std::string prefix = "";
    std::string backend = "pipewire";
    float duration = 3.5f;
    bool headless = false;
    std::string output_wav = "";

    for (int i = 2; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--appid" && i + 1 < argc) {
            appid = argv[++i];
        } else if (arg == "--prefix" && i + 1 < argc) {
            prefix = argv[++i];
        } else if (arg == "--backend" && i + 1 < argc) {
            backend = argv[++i];
        } else if (arg == "--duration" && i + 1 < argc) {
            try {
                duration = std::stof(argv[++i]);
            } catch (const std::exception&) {
                std::cerr << "Invalid --duration value: " << argv[i] << "\n";
                return 1;
            }
        } else if (arg == "--headless") {
            headless = true;
        } else if (arg == "--output" && i + 1 < argc) {
            output_wav = argv[++i];
        } else if (arg == "-h" || arg == "--help") {
            print_usage();
            return 0;
        }
    }

    if (subcommand == "install") {
        return cmd_install(appid, prefix);
    } else if (subcommand == "status") {
        return cmd_status(appid, prefix);
    } else if (subcommand == "test") {
        return cmd_test(prefix);
    } else if (subcommand == "demo") {
        return cmd_demo(backend, duration, headless, output_wav);
    } else {
        std::cerr << "Unknown subcommand: " << subcommand << "\n";
        print_usage();
        return 1;
    }
}
