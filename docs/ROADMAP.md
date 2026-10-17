# Project Roadmap: Linux Gaming Audio Stack (EAX Native)

This roadmap outlines the development and verification phases for the native Linux EAX / DirectSound3D spatialization stack.

---

## Phase 1: EAX & DirectSound3D Native Core (Standalone Linux, No Wine)

### Scope
- Implement the core C++20 engine in `eax-native/src/`.
- Flat C ABI boundary definition in `eax-native/include/eax_native_abi.h`.
- Pre-allocated memory arena and lock-free voice pool management.
- DirectSound3D coordinate transformation pipeline (left-handed to right-handed).
- Distance attenuation curves (min/max distance, rolloff factor).
- EAX 1.0, 2.0, 3.0, and 4.0 property translation into OpenAL Soft EFX calls (`AL_EFFECT_EAXREVERB`, `AL_DIRECT_FILTER`, `AL_AUXILIARY_SEND_FILTER`).
- Unit tests in `eax-native/tests/` exercising buffer creation, coordinate conversion, and EAX property queries without requiring Wine.

### Exit Criteria
- `PLACEHOLDER: Exact Phase 1 exit criteria to be defined by user.`

---

## Phase 2: Wine DirectSound Shim & eaxctl CLI

### Scope
- Implement the Windows PE DLL shim in `wine-integration/`.
- COM vtable generation for `IDirectSound8`, `IDirectSoundBuffer8`, `IDirectSound3DListener`, `IDirectSound3DBuffer`, and `IKsPropertySet`.
- Unix call bridge linking the PE shim to `libeax-native.so`.
- Command-line utility `eaxctl` in `tools/eaxctl/` to query active audio streams, inspect EAX property states, and toggle HRTF parameters in real time.
- Compatibility test harness with DirectSound test binaries running under Wine.

### Exit Criteria
- `PLACEHOLDER: Exact Phase 2 exit criteria to be defined by user.`

---

## Phase 3: PipeWire DSP Module & Latency Benchmark

### Scope
- Direct PipeWire SPA (Simple Plugin Architecture) integration node for `libeax-native.so`.
- Real-time audio processing benchmark measuring round-trip buffer latency and CPU overhead comparing:
  - Standard Wine builtin `dsound.dll` (via MMDevAPI / ALSA / PulseAudio).
  - Native EAX stack outputting to PipeWire.
- Verification of lock-free behavior, absence of priority inversions, and zero dynamic memory allocations on the audio thread.

### Exit Criteria
- `PLACEHOLDER: Exact Phase 3 exit criteria to be defined by user.`

---

## Phase 4: Arch Linux Packaging (PKGBUILDs & AUR)

### Scope
- Packaging scripts in `aur/` for Arch Linux and Arch-based gaming distributions:
  - `PKGBUILD` for `libeax-native` (native C++20 core library).
  - `PKGBUILD` for `wine-eax-dsound` (PE shim and Wine integration scripts).
  - `PKGBUILD` for `eaxctl` (CLI management utility).
- Continuous Integration pipelines in `ci/` validating MinGW-w64 cross-compilation, GCC/Clang native builds, packaging lint checks (`namcap`), and automated test runs.

### Exit Criteria
- `PLACEHOLDER: Exact Phase 4 exit criteria to be defined by user.`
