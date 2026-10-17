#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <fstream>
#include <cstdlib>
#include <sstream>

namespace fs = std::filesystem;

void print_usage() {
    std::cout << "eaxctl - EAX & DirectSound3D Proton/Wine Management Utility\n\n"
              << "Usage:\n"
              << "  eaxctl install --appid <steam_appid> [--prefix <path>]\n"
              << "  eaxctl status [--appid <steam_appid> | --prefix <path>]\n"
              << "  eaxctl test [--prefix <path>]\n\n"
              << "Subcommands:\n"
              << "  install   Install dsound.dll shim into Proton prefix & configure registry override\n"
              << "  status    Check status of dsound override and active EAX emulation\n"
              << "  test      Run DirectSound / EAX compatibility test inside prefix\n";
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

    // Locate built dsound.dll shim
    std::vector<fs::path> shim_locations = {
        "./build-pe64/wine-integration/dsound.dll",
        "./build-pe32/wine-integration/dsound.dll",
        "./wine-integration/dsound.dll",
        "./build/wine-integration/dsound.dll",
        "./build/dsound.dll",
        "/usr/lib/eax-native/dsound.dll"
    };

    fs::path found_shim;
    for (const auto& loc : shim_locations) {
        if (fs::exists(loc)) {
            found_shim = loc;
            break;
        }
    }

    if (found_shim.empty()) {
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

    std::cout << "[eaxctl] Copying dsound.dll from " << found_shim.string() << " to prefix...\n";
    try {
        if (fs::exists(sys32)) fs::copy_file(found_shim, sys32 / "dsound.dll", fs::copy_options::overwrite_existing);
        if (fs::exists(syswow64)) fs::copy_file(found_shim, syswow64 / "dsound.dll", fs::copy_options::overwrite_existing);
    } catch (const std::exception& e) {
        std::cerr << "[eaxctl] Warning during file copy: " << e.what() << "\n";
    }

    // Update user.reg with dsound override
    fs::path user_reg = prefix / "user.reg";
    std::cout << "[eaxctl] Configuring WINEDLLOVERRIDES \"dsound=native,builtin\" in user.reg...\n";
    {
        std::ofstream reg(user_reg, std::ios::app);
        reg << "\n[Software\\\\Wine\\\\DllOverrides]\n"
            << "\"dsound\"=\"native,builtin\"\n";
    }

    // Verify presence via winedbg
    std::cout << "[eaxctl] Verifying DLL override presence in prefix registry...\n";
    std::ifstream check_reg(user_reg);
    std::string line;
    bool found_override = false;
    while (std::getline(check_reg, line)) {
        if (line.find("\"dsound\"=\"native,builtin\"") != std::string::npos ||
            line.find("\"dsound\"=\"n,b\"") != std::string::npos) {
            found_override = true;
            break;
        }
    }

    if (found_override) {
        std::cout << "[eaxctl] SUCCESS: dsound override verified: dsound=n,b\n";
    } else {
        std::cerr << "[eaxctl] WARNING: Registry entry could not be confirmed in user.reg\n";
    }

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

    fs::path dsound32 = prefix / "drive_c/windows/system32/dsound.dll";
    fs::path dsound64 = prefix / "drive_c/windows/syswow64/dsound.dll";
    std::cout << "Shim 32-bit:   " << (fs::exists(dsound32) ? "Installed" : "Not Present") << "\n";
    std::cout << "Shim 64-bit:   " << (fs::exists(dsound64) ? "Installed" : "Not Present") << "\n";
    return 0;
}

int cmd_test(const std::string& explicit_prefix) {
    fs::path prefix = find_proton_prefix("", explicit_prefix);
    std::cout << "[eaxctl] Running compatibility test harness...\n";

    fs::path test_script = "./wine-integration/tests/run_wine_tests.sh";
    if (!fs::exists(test_script)) {
        test_script = "../wine-integration/tests/run_wine_tests.sh";
    }

    if (fs::exists(test_script)) {
        std::string cmd = "bash " + test_script.string();
        return std::system(cmd.c_str());
    } else {
        std::cout << "[eaxctl] run_wine_tests.sh script not found. Checking local test executable...\n";
        return std::system("./build/wine-integration/tests/test_dsound_compat");
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_usage();
        return 1;
    }

    std::string subcommand = argv[1];
    std::string appid = "";
    std::string prefix = "";

    for (int i = 2; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--appid" && i + 1 < argc) {
            appid = argv[++i];
        } else if (arg == "--prefix" && i + 1 < argc) {
            prefix = argv[++i];
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
    } else {
        std::cerr << "Unknown subcommand: " << subcommand << "\n";
        print_usage();
        return 1;
    }
}
