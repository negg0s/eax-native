#!/usr/bin/env bash
# Two-stage cross-compilation script for Windows PE 64-bit and 32-bit dsound.dll stack
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"

echo "=== Building Windows PE x86_64 Stack ==="
cmake -B "${ROOT_DIR}/build-pe64" \
    -S "${ROOT_DIR}" \
    -DCMAKE_TOOLCHAIN_FILE="${ROOT_DIR}/cmake/mingw64.cmake" \
    -DCMAKE_BUILD_TYPE=Release
cmake --build "${ROOT_DIR}/build-pe64" --target wine_dsound -j"$(nproc)"

echo "=== Building Windows PE i686 (32-bit) Stack ==="
cmake -B "${ROOT_DIR}/build-pe32" \
    -S "${ROOT_DIR}" \
    -DCMAKE_TOOLCHAIN_FILE="${ROOT_DIR}/cmake/mingw32.cmake" \
    -DCMAKE_BUILD_TYPE=Release
cmake --build "${ROOT_DIR}/build-pe32" --target wine_dsound -j"$(nproc)"

echo "=== Inspecting Export Tables ==="
if command -v x86_64-w64-mingw32-objdump &>/dev/null; then
    echo "--- 64-bit dsound.dll Exports ---"
    x86_64-w64-mingw32-objdump -p "${ROOT_DIR}/build-pe64/wine-integration/dsound.dll" | grep -A 22 '\[Ordinal/Name Pointer\] Table' || true
fi

if command -v i686-w64-mingw32-objdump &>/dev/null; then
    echo "--- 32-bit dsound.dll Exports ---"
    i686-w64-mingw32-objdump -p "${ROOT_DIR}/build-pe32/wine-integration/dsound.dll" | grep -A 22 '\[Ordinal/Name Pointer\] Table' || true
fi

echo "=== Windows PE Cross-Build Finished Successfully ==="
