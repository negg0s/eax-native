#!/usr/bin/env bash
# Wine execution runner script for DirectSound compatibility test
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BIN_DIR="${SCRIPT_DIR}/../../build"
TEST_EXE="${BIN_DIR}/wine_dsound_test.exe"
SHIM_DLL="${BIN_DIR}/dsound.dll"

echo "=== Running DirectSound & EAX Wine Compatibility Runner ==="

if ! command -v wine &>/dev/null; then
    echo "[wine_runner] 'wine' not found in PATH."
    echo "[wine_runner] Direct host test binary execution mode:"
    if [ -f "${BIN_DIR}/wine-integration/tests/test_dsound_compat" ]; then
        "${BIN_DIR}/wine-integration/tests/test_dsound_compat"
        exit 0
    else
        echo "[wine_runner] Test binary not compiled yet. Build targets first."
        exit 1
    fi
fi

WINEPREFIX="${WINEPREFIX:-$HOME/.wine_eax_test}"
mkdir -p "$WINEPREFIX"

# Set dsound override to native
WINEDLLOVERRIDES="dsound=n,b" wine "${TEST_EXE}"
echo "=== DirectSound Wine Compatibility Test Completed Successfully ==="
