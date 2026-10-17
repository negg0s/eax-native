# EAX Native: High-Performance Linux DirectSound3D & EAX Audio Engine

`eax-native` is a native Linux C++20 implementation of DirectSound3D spatialization, distance attenuation, and EAX environmental reverberation (EAX 1.0–4.0). It provides a zero-overhead flat C ABI boundary (`include/eax_native_abi.h`) designed for direct integration with Wine/Proton shims, PipeWire DSP, and native Linux games.

---

## Key Features

- **Flat C ABI (`eax_native_abi.h`)**: Pure C interface (`extern "C"`), opaque handles, strict per-struct packing (`#pragma pack(push, 2)` for `WAVEFORMATEX`, `#pragma pack(push, 4)` for DS3D/EAX structs), and compile-time `static_assert(sizeof(T) == N)` binary layout validation.
- **Pre-Allocated Memory Arena (`src/arena.h`)**: Linear pre-faulted memory arena ensuring zero dynamic `malloc` / `free` allocations in audio streaming hot paths (`eax_buffer_lock`, `eax_buffer_unlock`, mixing callbacks).
- **Lock-Free Voice Pool (`src/voice_pool.h`)**: Manages up to 64 concurrent hardware-accelerated audio voices with atomic state tracking and wait-free buffer recycling.
- **DirectSound3D Spatializer (`src/spatializer.h`)**:
  - DirectSound left-handed (+X right, +Y up, +Z forward) to OpenAL right-handed (+X right, +Y up, -Z forward) coordinate mapping.
  - Clamped inverse distance attenuation with configurable minimum/maximum distance and rolloff factor curves.
  - Directional sound cones with inside/outside angles and outside volume attenuation.
  - Normalized panning via `AL_PAN_SOFT` with constant-power panning law.
- **EAX Environmental Reverb (`src/eax_reverb.h`)**:
  - Full translation of EAX 1.0, 2.0, 3.0, and 4.0 listener and buffer properties (`IKsPropertySet`).
  - High-fidelity reverberation DSP engine implementing authentic early reflections, diffuse comb/allpass delay networks, and room HF absorption matching `REVERB_CAVE` and standard EAX environment presets.
  - Deferred parameter batching via `eax_listener_commit_deferred`.

---

## Building and Running

### Prerequisites (Arch Linux)
```bash
sudo pacman -S base-devel cmake clang openal catch2
```

### Build with CMake & Clang
```bash
cmake -B build -S eax-native -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```

### Running Test Suite
The comprehensive unit test suite exercises ABI layouts, coordinate transformations, RMS Left/Right pan ratios, and extended EAX reverb decay tails:
```bash
ctest --test-dir build --output-on-failure
# Or directly run the test binary:
./build/eax_tests
```

---

## 3D Fly-By Demo (`eax_demo`)

The demo binary simulates a 3D sound source traversing past the listener while enclosed in a cavernous environment (`REVERB_CAVE` preset with 2.91s decay time).

### Running the Demo
```bash
./build/eax_demo [output_filename.wav]
```

### What the Demo Demonstrates:
1. **Dynamic 3D Trajectory**: The sound source moves continuously from full left ($X = -20\text{ m}$) across the center to full right ($X = +20\text{ m}$) at a distance of $Z = 5\text{ m}$ ahead of the listener.
2. **Smooth Spatial Panning**: The left and right channel energies adjust dynamically according to the sound position in DirectSound coordinate space.
3. **EAX Cave Reverb Tail**: When the sound pulse stops at $t = 4.0\text{s}$, the reverberant field continues decaying naturally for over 2 seconds in the simulated cave environment.
4. **WAV Output**: Writes uncompressed 16-bit 44.1 kHz stereo audio to `eax_flyby_cave_demo.wav`.
