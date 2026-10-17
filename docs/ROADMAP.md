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

### Exit Criteria (Phase 1)
- `eax-native/tests/` pass fully: offline-generated 1 kHz sine; assert 3D pan follows position via RMS L/R ratio; assert reverb tail present when EAX environment = REVERB_CAVE. Framework: catch2.
- A standalone Linux test binary runs WITHOUT Wine in CI (fresh container).
- `eax-native/` builds with clang, `-Wall -Wextra -Werror`, no exceptions in hot paths; arena allocation for buffers (zero per-frame malloc).
- Demo binary plays a 3D fly-by sound with reverb through the local audio backend; documented in `eax-native/README.md`.

---

## Phase 2: Wine DirectSound Shim & eaxctl CLI

### Scope
- Implement the Windows PE DLL shim in `wine-integration/`.
- COM vtable generation for `IDirectSound8`, `IDirectSoundBuffer8`, `IDirectSound3DListener`, `IDirectSound3DBuffer`, and `IKsPropertySet`.
- Unix call bridge linking the PE shim to `libeax-native.so`.
- Command-line utility `eaxctl` in `tools/eaxctl/` to query active audio streams, inspect EAX property states, and toggle HRTF parameters in real time.
- Compatibility test harness with DirectSound test binaries running under Wine.

### Exit Criteria (Phase 2)
- `eaxctl install --appid <steam_appid>` performs: shim install into Proton prefix, `dsound=n,b` override, winedbg verification that the game resolves the shim.
- Test game (F.E.A.R. or Halo:CE installed via protontricks) queries `DSPROPSETID_EAX20` successfully where stock Wine returns `E_PROP_ID_UNSUPPORTED`.
- EAX options are enabled (not grayed out) in the game's audio menu; full before/after log in `docs/PHASE2_RESULTS.md`.

---

## Phase 3: PipeWire DSP Module & Latency Benchmark

### Scope
- Direct PipeWire SPA (Simple Plugin Architecture) integration node for `libeax-native.so`.
- Real-time audio processing benchmark measuring round-trip buffer latency and CPU overhead comparing:
  - Standard Wine builtin `dsound.dll` (via MMDevAPI / ALSA / PulseAudio).
  - Native EAX stack outputting to PipeWire.
- Verification of lock-free behavior, absence of priority inversions, and zero dynamic memory allocations on the audio thread.

### Exit Criteria (Phase 3)
- `eaxctl demo --backend pipewire` runs the Phase 1 fly-by demo with HRTF and reverb.
- Measured round-trip latency < 40 ms, report printed by a timestamped loopback probe; methodology reproducible.
- Benchmark doc `docs/PHASE3_BENCH.md`: latency + CPU% for openal vs pipewire backends on the same machine.

---

## Phase 4: Arch Linux Packaging (PKGBUILDs & AUR)

### Scope
- Packaging scripts in `aur/` for Arch Linux and Arch-based gaming distributions:
  - `PKGBUILD` for `libeax-native` (native C++20 core library).
  - `PKGBUILD` for `wine-eax-dsound` (PE shim and Wine integration scripts).
  - `PKGBUILD` for `eaxctl` (CLI management utility).
- Continuous Integration pipelines in `ci/` validating MinGW-w64 cross-compilation, GCC/Clang native builds, packaging lint checks (`namcap`), and automated test runs.

### Exit Criteria (Phase 4)
- For all three packages: `docker run archlinux` → `makepkg -si` succeeds on x86_64; `namcap` reports no errors; correct `.SRCINFO`.
- README install section ≤ 3 commands end-to-end.
- `docs/RELEASE.md` with semver policy, changelog, and upstream-update procedure (openal-soft/dsoal changes).
