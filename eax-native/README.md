# EAX Native: Linux DirectSound3D & EAX OpenAL Soft Translation Layer

`eax-native` is a native Linux C++20 translation library that routes DirectSound3D spatialization, distance attenuation, and EAX environmental reverberation (EAX 1.0–2.0 with 3.0/4.0 property set handling) directly to OpenAL Soft.

It exposes a zero-overhead flat C ABI boundary (`include/eax_native_abi.h`) designed for integration with Wine/Proton shims, PipeWire DSP nodes, and standalone test harnesses.

---

## Architecture & Features

- **Flat C ABI (`include/eax_native_abi.h`)**:
  - Pure C interface (`extern "C"`) with opaque handles (`eax_context_h`, `eax_buffer_h`).
  - Strict Windows-compatible packing rules (`#pragma pack(push, 2)` for `eax_waveformat_ex_t` = 18 bytes; `#pragma pack(push, 4)` for `eax_ds3dlistener_t` = 64 bytes, `eax_ds3dbuffer_t` = 64 bytes, `eax_dscaps_t` = 96 bytes, `eax_dsbcaps_t` = 20 bytes, `eax20_listener_properties_t` = 56 bytes, `eax20_buffer_properties_t` = 52 bytes).
  - Compile-time `static_assert(sizeof(T) == N)` assertions embedded in header for immediate ABI validation.

- **OpenAL Soft Direct Routing (`src/context.cpp`, `src/voice_pool.cpp`)**:
  - Context initializes OpenAL Soft via `alcOpenDevice` and `alcCreateContext`.
  - PCM sample updates from `eax_buffer_lock`/`unlock` upload directly into OpenAL buffers (`alBufferData`).
  - Voice management manages up to 64 pre-allocated voice slots with atomic in-use tracking.
  - Streaming audio hot paths avoid per-frame dynamic heap allocations (`malloc`/`free`).

- **DirectSound3D Coordinate Transformation (`src/spatializer.h`)**:
  - Converts DirectSound left-handed (+X right, +Y up, +Z forward) to OpenAL right-handed (+X right, +Y up, -Z forward) space by negating the Z coordinate on position, velocity, and orientation vectors.
  - Mapped via `alListener3f`, `alListenerfv`, `alSource3f(AL_POSITION)`, `alSource3f(AL_VELOCITY)`, `alSource3f(AL_DIRECTION)`.
  - Attenuation parameters mapped to `AL_REFERENCE_DISTANCE`, `AL_MAX_DISTANCE`, `AL_ROLLOFF_FACTOR`, `AL_CONE_INNER_ANGLE`, `AL_CONE_OUTER_ANGLE`, `AL_CONE_OUTER_GAIN`.
  - 2D panning via `AL_PAN_SOFT` (using formula `panf = (pan <= 0) ? (mB_to_gain(pan) - 1.0f) : (1.0f - mB_to_gain(-pan))`) without clobbering 3D positions.

- **EAX Reverb & EFX Filter Translation (`src/eax_reverb.cpp`)**:
  - Creates and manages an OpenAL Soft Auxiliary Effect Slot (`AL_EFFECT_EAXREVERB`).
  - Translates EAX listener properties (`lRoom`, `lRoomHF`, `flDecayTime`, `flDecayHFRatio`, `lReflections`, `lReverb`, `flEnvironmentDiffusion`, `flRoomRolloffFactor`) into OpenAL Soft EFX `AL_EAXREVERB_*` properties.
  - Translates buffer obstruction and occlusion into OpenAL low-pass filters (`AL_FILTER_LOWPASS`) bound to `AL_DIRECT_FILTER` and `AL_AUXILIARY_SEND_FILTER`.
  - Implements named constants and presets (`eax::PRESET_CAVE`, `eax::GUID_EAX20_Listener`, etc.) without magic literals.

- **Offline / Loopback Diagnostics**:
  - `eax_context_render_offline` leverages `ALC_SOFT_loopback` (`alcRenderSamplesSOFT`) when available to render directly from OpenAL Soft into test buffers.

---

## Building and Running

### Prerequisites (Arch Linux)
```bash
sudo pacman -S --needed base-devel cmake clang gcc openal catch2
```

### Build with CMake & Clang
```bash
cmake -B build -S eax-native -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```

### Running Test Suite
```bash
ctest --test-dir build --output-on-failure
# Or directly run Catch2 executable:
./build/eax_tests
```

The test suite covers:
- `test_abi_layout.cpp`: `sizeof` and `offsetof` verification for all boundary POD structures.
- `test_coordinates.cpp`: Left-to-right handed vector mapping and distance attenuation calculations.
- `test_spatial_pan_rms.cpp`: Direct OpenAL Soft source positioning and verification that left placement yields $RMS_L \gg RMS_R$ and right yields $RMS_R \gg RMS_L$.
- `test_eax_reverb.cpp`: Verifies OpenAL Soft EFX auxiliary slot allocation and asserts extended reverb tail decay under `eax::PRESET_CAVE`.
- `test_properties.cpp`: EAX 1.0, 2.0, 3.0, 4.0 `QuerySupport`, `Get`, `Set`, and deferred batch commits.

---

## Realtime 3D Audio Demo (`eax_demo`)

The demo binary initializes OpenAL Soft, sets the environment to `REVERB_CAVE`, and plays a moving 3D sound source live through the default local audio backend (headphones or speakers):

```bash
./build/eax_demo [optional_output.wav]
```

### What the Demo Does:
1. Plays a 3D tone in real time through the user's OpenAL audio device.
2. Traverses from left ($X = -20\text{ m}$) across center to right ($X = +20\text{ m}$) at $Z = 5\text{ m}$.
3. Stops the dry source and lets the OpenAL Soft cave reverb tail naturally decay for 2.5 seconds.
4. Optionally outputs an offline diagnostic WAV file if a filepath is provided.
