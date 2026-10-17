#!/usr/bin/env bash
# ==============================================================================
# Linux Gaming Stack - Phase 3 Latency Benchmark Runner
# ==============================================================================
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

echo "========================================================================"
echo "          Running Phase 3 Real-time Latency Benchmark Harness           "
echo "========================================================================"

BUILD_DIR="${REPO_ROOT}/build"
BENCH_BIN=""

# Check possible build locations
CANDIDATE_PATHS=(
    "${BUILD_DIR}/tools/eaxctl/eax_latency_bench"
    "${BUILD_DIR}/eax_latency_bench"
)

for cand in "${CANDIDATE_PATHS[@]}"; do
    if [[ -x "${cand}" ]]; then
        BENCH_BIN="${cand}"
        break
    fi
done

if [[ -z "${BENCH_BIN}" ]]; then
    echo "[*] eax_latency_bench binary not found. Compiling target..."
    cmake -B "${BUILD_DIR}" -S "${REPO_ROOT}" -DCMAKE_BUILD_TYPE=Release
    cmake --build "${BUILD_DIR}" --target eax_latency_bench -j2
    BENCH_BIN="${BUILD_DIR}/tools/eaxctl/eax_latency_bench"
fi

if [[ ! -x "${BENCH_BIN}" ]]; then
    echo "[-] Error: Failed to locate or compile eax_latency_bench at ${BENCH_BIN}!"
    exit 1
fi

echo "[*] Executing latency benchmark probe: ${BENCH_BIN}"
if ! "${BENCH_BIN}"; then
    echo ""
    echo "[-] FAILURE: Phase 3 latency benchmark execution failed."
    exit 1
fi

echo ""
echo "[+] SUCCESS: Phase 3 latency benchmark passed all assertions (< 40ms round-trip)."
