#!/usr/bin/env bash
# Wine execution runner script for DirectSound compatibility test
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/../.." && pwd)"

echo "=== Running DirectSound & EAX Wine Compatibility Runner ==="

# Check for host ELF test executable
HOST_TEST=""
for candidate in \
    "${ROOT_DIR}/build/wine-integration/tests/test_dsound_compat" \
    "/root/build-p2/wine-integration/tests/test_dsound_compat" \
    "${SCRIPT_DIR}/test_dsound_compat"; do
    if [ -f "$candidate" ]; then
        HOST_TEST="$candidate"
        break
    fi
done

# Check for Windows PE test executable and shim
TEST_EXE=""
for candidate in \
    "${ROOT_DIR}/build-pe64/wine-integration/tests/wine_dsound_test.exe" \
    "${ROOT_DIR}/build-pe32/wine-integration/tests/wine_dsound_test.exe" \
    "${ROOT_DIR}/build/wine-integration/tests/wine_dsound_test.exe" \
    "${SCRIPT_DIR}/wine_dsound_test.exe"; do
    if [ -f "$candidate" ]; then
        TEST_EXE="$candidate"
        break
    fi
done

SHIM_DLL=""
for candidate in \
    "${ROOT_DIR}/build-pe64/wine-integration/dsound.dll" \
    "${ROOT_DIR}/build-pe32/wine-integration/dsound.dll" \
    "${ROOT_DIR}/build/wine-integration/dsound.dll" \
    "${SCRIPT_DIR}/../dsound.dll"; do
    if [ -f "$candidate" ]; then
        SHIM_DLL="$candidate"
        break
    fi
done

if ! command -v wine &>/dev/null || [ -z "$TEST_EXE" ]; then
    echo "[wine_runner] Wine not found or PE test executable not built. Running host test binary mode..."
    if [ -n "$HOST_TEST" ]; then
        "$HOST_TEST"
        exit 0
    else
        echo "[wine_runner] Test binary not found. Build targets first."
        exit 1
    fi
fi

WINEPREFIX="${WINEPREFIX:-$HOME/.wine_eax_test}"
mkdir -p "$WINEPREFIX"

# If shim DLL is found, copy next to test executable or into prefix
if [ -n "$SHIM_DLL" ]; then
    cp -f "$SHIM_DLL" "$(dirname "$TEST_EXE")/"
fi

WINEDLLOVERRIDES="dsound=n,b" wine "${TEST_EXE}"
echo "=== DirectSound Wine Compatibility Test Completed Successfully ==="
