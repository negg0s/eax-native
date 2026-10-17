# Technical Research: DirectSound, DSOAL, OpenAL Soft, and Wine Audio Subsystem

## 1. Executive Summary

This document presents technical research into the mechanics of DirectSound interception, 3D spatialization, EAX environmental reverb translation, and the integration points within Wine on Arch Linux. All findings are verified against upstream reference source code in `eax-native/third_party/` (`kcat/dsoal`, `kcat/openal-soft`, and Wine `dlls/dsound`).

---

## 2. DirectSound Interception Mechanics (kcat/dsoal)

### 2.1 Binary Entrypoints and Vtable Redirection
DirectSound is based on Microsoft COM (Component Object Model). Applications interact with it either by directly calling exported DLL functions from `dsound.dll` or by invoking `CoCreateInstance` on registered DirectSound COM class identifiers.

In `kcat/dsoal` (verified in `eax-native/third_party/dsoal/mingw.def` lines 1–17 and `eax-native/third_party/dsoal/src/dsoal.cpp` lines 442–785), DSOAL replaces `dsound.dll` by exporting identical entrypoints:
- `DirectSoundCreate` (`DSOAL_DirectSoundCreate@12`)
- `DirectSoundCreate8` (`DSOAL_DirectSoundCreate8@12`)
- `DirectSoundEnumerateA` (`DSOAL_DirectSoundEnumerateA@8`)
- `DirectSoundEnumerateW` (`DSOAL_DirectSoundEnumerateW@8`)
- `DirectSoundCaptureCreate` (`DSOAL_DirectSoundCaptureCreate@12`)
- `DirectSoundCaptureCreate8` (`DSOAL_DirectSoundCaptureCreate8@12`)
- `DirectSoundCaptureEnumerateA` (`DSOAL_DirectSoundCaptureEnumerateA@8`)
- `DirectSoundCaptureEnumerateW` (`DSOAL_DirectSoundCaptureEnumerateW@8`)
- `DirectSoundFullDuplexCreate` (`DSOAL_DirectSoundFullDuplexCreate@40`)
- `GetDeviceID` (`DSOAL_GetDeviceID@8`)
- `DllGetClassObject` (`DSOAL_DllGetClassObject@12`)
- `DllCanUnloadNow` (`DSOAL_DllCanUnloadNow@0`)

When a game initiates COM activation via `CoCreateInstance`, the COM runtime dispatches to `DllGetClassObject` (`dsoal/src/dsoal.cpp:778`), matching the following class GUIDs:
- `CLSID_DirectSound`: `{47D4D946-62E8-11CF-93BC-444553540000}`
- `CLSID_DirectSound8`: `{3901CC3F-84B5-4FA4-BA35-AA8172B8A09B}`
- `CLSID_DirectSoundCapture`: `{B0210780-89CD-11D0-AF08-00A0C925CD16}`
- `CLSID_DirectSoundCapture8`: `{E4BCAC14-F620-4B72-9F4F-57F7A192B3BE}`
- `CLSID_DirectSoundFullDuplex`: `{FEA4300C-7959-4147-B26A-2377B9E7A916}`

Vtable interception occurs through C++ interface inheritance:
1. `DSound8OAL` (`dsoal/src/dsoundoal.h:113`) inherits from `IDirectSound8`. C++ compiler vtables match the COM binary standard (`stdcall` calling convention on 32-bit x86, Microsoft x64 ABI on 64-bit).
2. Querying interface capabilities via `QueryInterface` (`dsoal/src/dsoundoal.cpp:491–525`) handles:
   - `IID_IUnknown`: `{00000000-0000-0000-C000-000000000046}`
   - `IID_IDirectSound`: `{279AFA83-4981-11CE-A521-0020AF0BE560}`
   - `IID_IDirectSound8`: `{C50A7E93-F395-4834-9EF6-7FA9F2829664}`

### 2.2 Buffer Hierarchy and Secondary COM Interfaces
DirectSound organizes audio playback into Primary and Secondary buffers:
- **Primary Buffer (`PrimaryBuffer` in `dsoal/src/primarybuffer.h:37`)**: Represents the output mix and global listener. Exposes:
  - `IDirectSoundBuffer` (`IID_IDirectSoundBuffer`: `{279AFA85-4981-11CE-A521-0020AF0BE560}`)
  - `IDirectSound3DListener` (`IID_IDirectSound3DListener`: `{279AFA84-4981-11CE-A521-0020AF0BE560}`, `dsoal/src/primarybuffer.cpp:82–86`)
  - `IKsPropertySet` (`IID_IKsPropertySet`: `{31EFAC30-515C-11D0-A9AA-00AA0061BE93}`, `dsoal/src/primarybuffer.cpp:88–92`)
- **Secondary Buffer (`Buffer` in `dsoal/src/buffer.h:55`)**: Represents discrete sound streams/voices. Exposes:
  - `IDirectSoundBuffer` (`IID_IDirectSoundBuffer`)
  - `IDirectSoundBuffer8` (`IID_IDirectSoundBuffer8`: `{6825A449-7524-4D82-920F-50E36AB3AB1E}`, `dsoal/src/buffer.cpp:714–723`)
  - `IDirectSound3DBuffer` (`IID_IDirectSound3DBuffer`: `{279AFA86-4981-11CE-A521-0020AF0BE560}`, `dsoal/src/buffer.cpp:725–734`, active if `DSBCAPS_CTRL3D` flag is set)
  - `IDirectSoundNotify` (`IID_IDirectSoundNotify`: `{B0210783-89CD-11D0-AF08-00A0C925CD16}`, `dsoal/src/buffer.cpp:736–745`, active if `DSBCAPS_CTRLPOSITIONNOTIFY` flag is set)
  - `IKsPropertySet` (`IID_IKsPropertySet`, `dsoal/src/buffer.cpp:747–751`)

### 2.3 3D Coordinate System and Spatialization Model
DirectSound uses a **left-handed Cartesian coordinate system**:
- $+X$: right
- $+Y$: up
- $+Z$: forward (into the screen)

OpenAL uses a **right-handed Cartesian coordinate system**:
- $+X$: right
- $+Y$: up
- $-Z$: forward (into the screen)

In `dsoal/src/primarybuffer.cpp:681–685` and `dsoal/src/buffer.cpp:1499–1506`, coordinate transformation is performed by inverting the Z coordinate:
```cpp
// DirectSound to OpenAL listener coordinate mapping:
alListener3fDirect(mContext, AL_POSITION, params.vPosition.x, params.vPosition.y, -params.vPosition.z);
alListener3fDirect(mContext, AL_VELOCITY, params.vVelocity.x, params.vVelocity.y, -params.vVelocity.z);

// DirectSound to OpenAL buffer (source) coordinate mapping:
alSource3fDirect(mContext, mSource, AL_POSITION, params.vPosition.x, params.vPosition.y, -params.vPosition.z);
alSource3fDirect(mContext, mSource, AL_VELOCITY, params.vVelocity.x, params.vVelocity.y, -params.vVelocity.z);
alSource3fDirect(mContext, mSource, AL_DIRECTION, params.vConeOrientation.x, params.vConeOrientation.y, -params.vConeOrientation.z);
```

#### Attenuation and Distance Models
- `flMinDistance`: Mapped directly to `AL_REFERENCE_DISTANCE` (`buffer.cpp:1519`). Below this distance, gain remains at maximum ($1.0$).
- `flMaxDistance`: Mapped directly to `AL_MAX_DISTANCE` (`buffer.cpp:1521`). Beyond this distance, attenuation clamps or drops to silent depending on rolloff factor.
- `flRolloffFactor`: Mapped to `AL_ROLLOFF_FACTOR` (`buffer.cpp:1541`).
- `flDistanceFactor`: Mapped to `AL_METERS_PER_UNIT` (`primarybuffer.cpp:697`). DirectSound defaults to meters ($1.0$), but games using feet or arbitrary units scale this property.
- Mode Switching (`buffer.cpp:1522–1544`):
  - `DS3DMODE_NORMAL`: Full 3D spatialization with distance attenuation and Doppler effects.
  - `DS3DMODE_HEADRELATIVE`: Position is relative to listener (`AL_SOURCE_RELATIVE = AL_TRUE`).
  - `DS3DMODE_DISABLE`: 3D processing disabled. DSOAL pins position to $(0, 0, -1)$, sets rolloff to $0.0$, and disables spatialization (`AL_SOURCE_SPATIALIZE_SOFT = AL_FALSE`).

#### Immediate vs. Deferred Execution
DirectSound methods accept `DS3D_IMMEDIATE` or `DS3D_DEFERRED`.
- Under `DS3D_DEFERRED`, changes are queued into temporary parameter structures (`mDeferred` bitset in `primarybuffer.h:60` and `buffer.h:178`).
- Calling `IDirectSound3DListener::CommitDeferredSettings()` (`primarybuffer.cpp:673–720`) flushes all accumulated listener and source modifications in a single batched OpenAL context update using `alcSuspendContext` and `alcProcessContext`.

### 2.4 EAX Property Set Architecture via IKsPropertySet
EAX does not introduce new COM interfaces; rather, it attaches to DirectSound objects through the kernel streaming interface `IKsPropertySet`.

In `eax-native/third_party/dsoal/src/eax.h` (lines 6–75), property sets are distinguished by GUID:
- `DSPROPSETID_EAX10_ListenerProperties`: `{4A4E6FC1-C341-11D1-B73A-444553540000}`
- `DSPROPSETID_EAX10_BufferProperties`: `{4A4E6FC0-C341-11D1-B73A-444553540000}`
- `DSPROPSETID_EAX20_ListenerProperties`: `{0306A6A8-B224-11D2-99E5-0000E8D8C722}`
- `DSPROPSETID_EAX20_BufferProperties`: `{0306A6A7-B224-11D2-99E5-0000E8D8C722}`
- `DSPROPSETID_EAX30_ListenerProperties`: `{A8FA6882-B476-11D3-BDB9-00C0F02DDF87}`
- `DSPROPSETID_EAX30_BufferProperties`: `{A8FA6881-B476-11D3-BDB9-00C0F02DDF87}`
- `EAXPROPERTYID_EAX40_Context`: `{1D4870AD-0DEF-43C0-A40C-523632296342}`
- `EAXPROPERTYID_EAX40_FXSlot0`: `{C4D79F1E-F1AC-436B-A81D-A738E7045469}`
- `EAXPROPERTYID_EAX40_FXSlot1`: `{08C00E96-74BE-4491-93AA-E8AD35A49117}`
- `EAXPROPERTYID_EAX40_FXSlot2`: `{1D433B88-F0F6-4637-919F-60E7E06B5EDD}`
- `EAXPROPERTYID_EAX40_FXSlot3`: `{EFFF08EA-C7D8-44AB-93AD-6DBD5F910064}`
- `EAXPROPERTYID_EAX40_Source`: `{1B86B823-22DF-4EAE-8B3C-1278CE544227}`

In DSOAL, `Buffer::Prop::Set` (`dsoal/src/buffer.cpp:2230–2290`) and `PrimaryBuffer::Prop::Set` (`dsoal/src/primarybuffer.cpp:845–890`) intercept calls to `IKsPropertySet::Set` and `IKsPropertySet::Get`. Instead of emulating the DSP in user space, DSOAL checks for the OpenAL extension `AL_EXT_EAX` and forwards property payloads to OpenAL Soft using:
```cpp
EAXSetDirect(mContext, &guidPropSet, dwPropID, mSource, pPropData, cbPropData);
EAXGetDirect(mContext, &guidPropSet, dwPropID, mSource, pPropData, cbPropData);
```
(Defined in `dsoal/src/AL/alext.h:1115–1116`).

---

## 3. OpenAL Soft Reverb & HRTF Architecture (kcat/openal-soft)

### 3.1 HRTF Processing Model
OpenAL Soft implements true head-related transfer function filters for spatial binaural output over stereo headphones.

- **Interface & Configuration (`eax-native/third_party/openal-soft/include/AL/alext.h:471–488`)**:
  - `ALC_SOFT_HRTF` extension.
  - Attributes: `ALC_HRTF_SOFT` (`ALC_TRUE`, `ALC_FALSE`, `ALC_DONT_CARE_SOFT`).
  - Runtime status queried via `alcGetIntegerv(device, ALC_HRTF_STATUS_SOFT, 1, &status)`:
    - `ALC_HRTF_DISABLED_SOFT` ($0$)
    - `ALC_HRTF_ENABLED_SOFT` ($1$)
    - `ALC_HRTF_DENIED_SOFT` ($2$)
    - `ALC_HRTF_REQUIRED_SOFT` ($3$)
    - `ALC_HRTF_HEADPHONES_DETECTED_SOFT` ($4$)
    - `ALC_HRTF_UNSUPPORTED_FORMAT_SOFT` ($5$)
- **Core Signal Pipeline (`eax-native/third_party/openal-soft/core/hrtf.cpp` and `openal-soft/core/mixer/hrtfbase.h`)**:
  - Audio frames are spatialized using Head-Related Impulse Response (HRIR) tables loaded from dataset definition files (`openal-soft/hrtf/Default HRTF.mhr`).
  - Real-time convolution applies ear-specific delays (Interaural Time Difference, ITD) and frequency-shaping FIR filters (Interaural Level Difference, ILD) per audio chunk, avoiding phase discontinuities using history overlap-add.

### 3.2 EAX Reverb Engine in OpenAL Soft
OpenAL Soft contains a complete internal EAX implementation (`openal-soft/al/eax/api.h`, `openal-soft/al/eax/call.cpp`, `openal-soft/al/eax/effect.h`, `openal-soft/al/auxeffectslot.cpp`).

- **Auxiliary Effect Slot Mapping**:
  In `openal-soft/al/auxeffectslot.cpp:269, 1170`, EAX effects allocate and bind to an auxiliary slot of type `AL_EFFECT_EAXREVERB`.
- **EAX Parameter Translation**:
  EAX parameters are translated into `EFXEAXREVERBPROPERTIES` (`openal-soft/include/AL/efx.h:180–230`):
  - `flDensity` / `flDiffusion`: Early reflection scattering and modal density.
  - `flGain` / `flGainHF` / `flGainLF`: Master reverberation gain and shelving attenuation.
  - `flDecayTime` / `flDecayHFRatio` / `flDecayLFRatio`: Mid-frequency RT60 decay time and high/low ratio multipliers.
  - `flReflectionsGain` / `flReflectionsDelay` / `flReflectionsPan`: Early reflections amplitude, onset latency, and 3D vector pan.
  - `flLateReverbGain` / `flLateReverbDelay` / `flLateReverbPan`: Late diffuse field amplitude, onset latency, and 3D vector pan.
  - `flEchoTime` / `flEchoDepth`: Cyclic echo spacing and feedback loop depth.
  - `flModulationTime` / `flModulationDepth`: Pitch chorus modulation to prevent flutter echoes.
  - `flAirAbsorptionGainHF`: High-frequency air absorption ($dB/m$).
  - `flHFReference` ($20–20000\text{ Hz}$) / `flLFReference` ($20–1000\text{ Hz}$): Corner crossover frequencies for dual biquad shelf filters.
  - `flRoomRolloffFactor`: Distance attenuation curve scaling for the reverberant tail.

- **Obstruction and Occlusion (`openal-soft/core/filters/biquad.cpp`)**:
  - Direct sound path: Filtered using low-pass biquad filters (`AL_DIRECT_FILTER`).
  - Auxiliary reverb send path: Attenuated independently using auxiliary send low-pass filters (`AL_AUXILIARY_SEND_FILTER`).

---

## 4. Wine DirectSound Architecture & Interception Points

### 4.1 Upstream Wine Architecture (`eax-native/third_party/wine/dlls/dsound/`)
Analysis of Wine's implementation in `dlls/dsound` reveals:
1. **Audio Driver Backend (`dlls/dsound/primary.c:314–352`)**:
   Wine's `dsound.dll` does not output directly to ALSA, PulseAudio, or OSS. It activates the Windows Multimedia Device API (`IMMDevice` / `IAudioClient` from `mmdevapi.dll`). `mmdevapi.dll` then routes calls through Wine's platform audio drivers (`winealsa.drv`, `winepulse.drv`, or `wineoss.drv`).
2. **Software Mixer Bottleneck (`dlls/dsound/mixer.c`)**:
   Wine performs software downmixing of secondary buffers into a primary mixing buffer before pushing audio frames into `IAudioClient_GetCurrentPadding` and `IAudioRenderClient_GetBuffer`.
3. **Absence of EAX (`dlls/dsound/propset.c:524–585`)**:
   Wine's `IKsPropertySet` implementation explicitly rejects all property sets except `DSPROPSETID_DirectSoundDevice` (`{84624f82-25ec-11d1-a4d8-00c04fc28aca}`). Queries for EAX 1.0–5.0 GUIDs fail with `E_PROP_ID_UNSUPPORTED` or `E_PROP_SET_UNSUPPORTED`.
4. **Simplistic 3D Math (`dlls/dsound/sound3d.c`)**:
   Wine performs rudimentary volume and balance panning in software (`DSOUND_3D_Calculate`), without HRTF convolution, elevation filtering, or room acoustics modeling.

### 4.2 Interception Strategy Comparison

| Strategy | Interception Point | Pros | Cons |
| :--- | :--- | :--- | :--- |
| **Strategy A: PE Drop-in (`dsound.dll`)** | Application directory or `WINEDLLOVERRIDES="dsound=n,b"` | Zero Wine modifications; isolated per-game testing; easy deployment. | Requires Windows PE cross-compilation (MinGW-w64); cannot directly link host Linux libraries without Wine Unix call bridge. |
| **Strategy B: Wine builtin replacement** | Replace `dlls/dsound/` inside Wine tree | Seamless integration for all wine prefixes automatically. | Requires patching and rebuilding Wine or Proton; difficult to distribute via AUR. |
| **Strategy C: Split Architecture (PE Shim + Native Linux Shared Library)** | PE `dsound.dll` in Wine $\to$ flat C ABI / `__wine_unix_call` $\to$ `libeax-native.so` | Combines native PE COM compatibility with direct native Linux execution (OpenAL Soft / PipeWire direct access, low latency, no MMDevAPI overhead). | Requires maintaining a stable, decoupled C ABI boundary across the PE/Unix barrier. |

**Selected Strategy for Project:** Strategy C (Split Architecture).
The Windows PE component remains a thin translation shim forwarding COM invocations across a flat C ABI to `libeax-native.so`, which executes natively on Linux with OpenAL Soft and PipeWire backends.

---

## 5. Verification Status & Discrepancies

- [x] **DSOAL Vtable & COM interfaces**: Verified via `dsoal/mingw.def`, `dsoal/src/dsoundoal.h`, `dsoal/src/buffer.h`.
- [x] **DSOAL coordinate system mapping**: Verified via `dsoal/src/primarybuffer.cpp:681`, `dsoal/src/buffer.cpp:1499`.
- [x] **DSOAL EAX delegation**: Verified via `dsoal/src/buffer.cpp:2244`, `dsoal/src/eax.h:6–75`.
- [x] **OpenAL Soft HRTF extension**: Verified via `openal-soft/include/AL/alext.h:471`, `openal-soft/core/hrtf.cpp`.
- [x] **OpenAL Soft EAX Reverb**: Verified via `openal-soft/al/auxeffectslot.cpp:269, 1170`, `openal-soft/al/eax/api.h`.
- [x] **Wine dsound MMDevAPI path**: Verified via `wine/dlls/dsound/primary.c:314`.
- [x] **Wine dsound lack of EAX**: Verified via `wine/dlls/dsound/propset.c:524`.
- [!] **UNVERIFIED**: Exact EAX 5.0 128-voice hardware voice virtualization limits in vintage Creative hardware vs OpenAL Soft software mixing capacity. Marked for benchmarking in Phase 3.
