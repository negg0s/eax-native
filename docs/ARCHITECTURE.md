# System Architecture & C ABI Specification

## 1. Overview & Architectural Stack

The project provides high-performance, low-latency 3D spatial audio and environmental reverberation (EAX 1.0–4.0) for Windows games running under Wine/Proton on Arch Linux. 

Rather than relying on Wine's legacy software downmixing pipeline (`wine/dlls/dsound/mixer.c`) or standard Windows-only wrapper DLLs, the system employs a decoupled split architecture:
1. **Windows PE Shim (`wine-integration/`)**: A lightweight DirectSound COM implementation compiled as a Windows PE DLL (`dsound.dll`), exposing standard COM interfaces (`IDirectSound8`, `IDirectSoundBuffer8`, `IDirectSound3DListener`, `IDirectSound3DBuffer`, `IKsPropertySet`).
2. **Flat C ABI Boundary**: A zero-overhead C-compatible interface transferring calls from the Windows PE environment to native Linux without COM object leaks or struct packing mismatches.
3. **Native Core (`eax-native/`)**: High-performance C++20 engine running as a native Linux shared library (`libeax-native.so`), featuring an internal memory arena, real-time safe state management, and direct backend translation to OpenAL Soft and PipeWire.

```mermaid
graph TD
    subgraph "Wine / Windows PE Process Context"
        Game["Game Executable (.exe)"] -->|DirectSound / COM API| Shim["dsound.dll (PE Shim)"]
        Shim -->|IDirectSound8 / IDirectSoundBuffer8| COM_DS["COM Interface Wrapper"]
        Shim -->|IDirectSound3DListener / 3DBuffer| COM_3D["3D Spatializer Wrapper"]
        Shim -->|IKsPropertySet (EAX 1-4)| COM_EAX["EAX Property Set Wrapper"]
    end

    subgraph "C ABI Boundary (Unix Call / Flat C Dynamic Link)"
        COM_DS -->|eax_ds_*| CABI["Flat C ABI (extern 'C', POD structs, opaque handles)"]
        COM_3D -->|eax_3d_*| CABI
        COM_EAX -->|eax_prop_*| CABI
    end

    subgraph "Native Linux Process Context (libeax-native.so)"
        CABI --> Core["eax-native Core (C++20, noexcept)"]
        Core --> Arena["Realtime Memory Arena (Zero malloc in hot paths)"]
        Core --> State["Voice & Listener State Graph"]
        Core --> Trans["EAX Parameter Translator"]
    end

    subgraph "Linux Audio Backends"
        Trans -->|ALC_SOFT_HRTF / AL_EFFECT_EAXREVERB| OAL["OpenAL Soft Engine"]
        Core -->|Direct Low-Latency Stream| PW["PipeWire SPA / DSP Client"]
        OAL --> PW
        PW --> AudioHW["Audio Hardware / Headphones (Binaural HRTF)"]
    end
```

---

## 2. C API Boundary & Method Mapping Table

Every DirectSound and EAX method called by games maps to a corresponding function at the C ABI boundary, which translates to OpenAL Soft / EFX operations.

### 2.1 IDirectSound & IDirectSound8 Methods

| DirectSound Interface Method | Flat C ABI Function | Backend Translation / OpenAL Soft Mapping | Notes |
| :--- | :--- | :--- | :--- |
| `QueryInterface` | `eax_ds_query_interface` | Local handle validation & GUID dispatch | Evaluates `IID_IUnknown`, `IID_IDirectSound`, `IID_IDirectSound8`. |
| `AddRef` | `eax_ds_add_ref` | Atomic increment (`std::atomic_ref`) | Thread-safe, lock-free. |
| `Release` | `eax_ds_release` | Atomic decrement; deallocates on zero | Lock-free; returns residual refcount. |
| `CreateSoundBuffer` | `eax_ds_create_buffer` | `alGenSourcesDirect`, arena slot reservation | Allocates secondary voice from pre-reserved pool. |
| `GetCaps` | `eax_ds_get_caps` | Returns static `DSCAPS` with 3D/EAX flags | Advertises hardware acceleration and EAX capability. |
| `DuplicateSoundBuffer` | `eax_ds_duplicate_buffer` | `alGenSourcesDirect`, clones buffer ref | Shares underlying sample data buffer. |
| `SetCooperativeLevel` | `eax_ds_set_cooperative_level` | NOP / state record | DirectSound cooperative levels are ignored in Linux audio. |
| `Compact` | `eax_ds_compact` | `DS_OK` (NOP) | Memory compaction not applicable. |
| `GetSpeakerConfig` | `eax_ds_get_speaker_config` | Returns `DSSPEAKER_STEREO` or `DSSPEAKER_HEADPHONE` | Determined by active OpenAL HRTF status. |
| `SetSpeakerConfig` | `eax_ds_set_speaker_config` | `alcResetDeviceSOFT` with `ALC_HRTF_SOFT` | Enables HRTF when headphone config requested. |
| `Initialize` | `eax_ds_initialize` | `alcCreateContext`, `alcMakeContextCurrent` | Initializes OpenAL device and context. |
| `VerifyCertification` | `eax_ds_verify_certification` | Returns `DS_OK` (`_CERTIFIED`) | DirectSound 8 WHQL certification verification stub. |

### 2.2 IDirectSoundBuffer & IDirectSoundBuffer8 Methods

| DirectSoundBuffer Method | Flat C ABI Function | Backend Translation / OpenAL Soft Mapping | Notes |
| :--- | :--- | :--- | :--- |
| `QueryInterface` | `eax_buf_query_interface` | Local handle validation & GUID dispatch | Matches `IID_IDirectSound3DBuffer`, `IID_IKsPropertySet`. |
| `AddRef` | `eax_buf_add_ref` | Atomic increment | Thread-safe. |
| `Release` | `eax_buf_release` | Atomic decrement; releases to voice arena | Reclaims voice slot into arena free-list. |
| `GetCaps` | `eax_buf_get_caps` | Copies cached `DSBCAPS` | Zero syscalls/locks. |
| `GetCurrentPosition` | `eax_buf_get_current_position` | `alGetSourceiDirect(..., AL_BYTE_OFFSET, ...)` | Calculates play and write cursors. |
| `SetCurrentPosition` | `eax_buf_set_current_position` | `alSourceiDirect(..., AL_BYTE_OFFSET, ...)` | Resets playback position within PCM buffer. |
| `GetFormat` | `eax_buf_get_format` | Copies stored `WAVEFORMATEX` | Cached in buffer instance. |
| `SetFormat` | `eax_buf_set_format` | Validates PCM and reconfigures voice | Only valid on primary buffer in DirectSound. |
| `GetVolume` | `eax_buf_get_volume` | Returns stored millibel attenuation | Cached attenuation ($0\text{ to }-10000\text{ mB}$). |
| `SetVolume` | `eax_buf_set_volume` | `alSourcefDirect(..., AL_GAIN, gain)` | Millibels converted to linear gain ($10^{mB/2000}$). |
| `GetPan` | `eax_buf_get_pan` | Returns stored stereo pan ($mPan$, range $-10000\text{ to }+10000\text{ mB}$) | Cached DirectSound millibel pan. |
| `SetPan` | `eax_buf_set_pan` | `alSourceiDirect(..., AL_PANNING_ENABLED_SOFT, AL_TRUE)`, `alSourcefDirect(..., AL_PAN_SOFT, panf)` | Non-3D buffers only (`dsoal/src/buffer.cpp:672–675`): `panf = (pan <= 0) ? (mB_to_gain(pan) - 1.0f) : (1.0f - mB_to_gain(-pan))`, where $mB\_to\_gain(x) = 10^{x/2000}$. Does NOT touch `AL_POSITION`. Volume remains mapped via `AL_GAIN` (`buffer.cpp:623`). |
| `GetFrequency` | `eax_buf_get_frequency` | Returns sample rate in Hz | Cached value. |
| `SetFrequency` | `eax_buf_set_frequency` | `alSourcefDirect(..., AL_PITCH, freq / base_freq)` | Pitch shifting calculated relative to original rate. |
| `GetStatus` | `eax_buf_get_status` | `alGetSourceiDirect(..., AL_SOURCE_STATE, ...)` | Returns `DSBSTATUS_PLAYING`, `DSBSTATUS_LOOPING`. |
| `Initialize` | `eax_buf_initialize` | Configures secondary buffer | Standard DirectSound initialization. |
| `Lock` | `eax_buf_lock` | Maps arena circular buffer pointers | **Hot path**: Zero malloc; provides direct pointer slices. |
| `Unlock` | `eax_buf_unlock` | `alBufferDataDirect` or queue unrolled chunks | **Hot path**: Updates OpenAL buffer objects. |
| `Play` | `eax_buf_play` | `alSourcePlayDirect(..., source)` | Starts audio stream playback. |
| `Stop` | `eax_buf_stop` | `alSourceStopDirect(..., source)` | Stops audio stream playback. |
| `SetFX` | `eax_buf_set_fx` | Binds EFX effect slots to voice | DirectSound8 DMO effect stub/mapping. |
| `AcquireResources` | `eax_buf_acquire_resources` | Returns `DS_OK` | DirectSound8 resource allocation stub. |
| `GetObjectInPath` | `eax_buf_get_object_in_path` | `DSERR_CONTROLUNAVAIL` or internal interface | DirectSound8 effect query. |

### 2.3 IDirectSound3DListener Methods

| Listener Method | Flat C ABI Function | Backend Translation / OpenAL Soft Mapping | Coordinate Transform |
| :--- | :--- | :--- | :--- |
| `GetAllParameters` | `eax_3dl_get_all_parameters` | Copies cached `DS3DLISTENER` | Cached in listener object. |
| `SetAllParameters` | `eax_3dl_set_all_parameters` | Batched `alListener3f` / `alListenerfv` calls | Negates Z coordinates. |
| `GetDistanceFactor` | `eax_3dl_get_distance_factor` | Returns `flDistanceFactor` | Cached value. |
| `SetDistanceFactor` | `eax_3dl_set_distance_factor` | `alListenerfDirect(..., AL_METERS_PER_UNIT, factor)` | Direct mapping. |
| `GetDopplerFactor` | `eax_3dl_get_doppler_factor` | Returns `flDopplerFactor` | Cached value. |
| `SetDopplerFactor` | `eax_3dl_set_doppler_factor` | `alDopplerFactorDirect(factor)` | Direct mapping. |
| `GetOrientation` | `eax_3dl_get_orientation` | Returns front and top vectors | Cached orientation. |
| `SetOrientation` | `eax_3dl_set_orientation` | `alListenerfvDirect(..., AL_ORIENTATION, ori)` | $(x_f, y_f, -z_f, x_t, y_t, -z_t)$ |
| `GetPosition` | `eax_3dl_get_position` | Returns cached position | Cached coordinates. |
| `SetPosition` | `eax_3dl_set_position` | `alListener3fDirect(..., AL_POSITION, x, y, -z)` | Negates Z coordinate. |
| `GetRolloffFactor` | `eax_3dl_get_rolloff_factor` | Returns `flRolloffFactor` | Global distance attenuation curve. |
| `SetRolloffFactor` | `eax_3dl_set_rolloff_factor` | Updates default source rolloff factors | Applied to active sources. |
| `GetVelocity` | `eax_3dl_get_velocity` | Returns cached velocity | Cached velocity. |
| `SetVelocity` | `eax_3dl_set_velocity` | `alListener3fDirect(..., AL_VELOCITY, x, y, -z)` | Negates Z coordinate. |
| `CommitDeferredSettings` | `eax_3dl_commit_deferred` | `alcSuspendContext` $\to$ flush $\to$ `alcProcessContext` | Atomic batch application of staged updates. |

### 2.4 IDirectSound3DBuffer Methods

| 3D Buffer Method | Flat C ABI Function | Backend Translation / OpenAL Soft Mapping | Coordinate Transform |
| :--- | :--- | :--- | :--- |
| `GetAllParameters` | `eax_3db_get_all_parameters` | Copies cached `DS3DBUFFER` | Cached parameters. |
| `SetAllParameters` | `eax_3db_set_all_parameters` | Batched `alSource3f` / `alSourcef` calls | Negates Z coordinates. |
| `GetConeAngles` | `eax_3db_get_cone_angles` | Returns inside/outside cone angles | Cached angles ($^\circ$). |
| `SetConeAngles` | `eax_3db_set_cone_angles` | `alSourcef(..., AL_CONE_INNER_ANGLE / AL_CONE_OUTER_ANGLE)` | Direct degree mapping. |
| `GetConeOrientation` | `eax_3db_get_cone_orientation` | Returns cone direction vector | Cached direction. |
| `SetConeOrientation` | `eax_3db_set_cone_orientation` | `alSource3fDirect(..., AL_DIRECTION, x, y, -z)` | Negates Z coordinate. |
| `GetConeOutsideVolume` | `eax_3db_get_cone_outside_volume` | Returns outside cone volume (mB) | Cached attenuation. |
| `SetConeOutsideVolume` | `eax_3db_set_cone_outside_volume` | `alSourcefDirect(..., AL_CONE_OUTER_GAIN, gain)` | Millibels converted to linear gain. |
| `GetMaxDistance` | `eax_3db_get_max_distance` | Returns `flMaxDistance` | Clamping cutoff distance. |
| `SetMaxDistance` | `eax_3db_set_max_distance` | `alSourcefDirect(..., AL_MAX_DISTANCE, dist)` | Direct mapping. |
| `GetMinDistance` | `eax_3db_get_min_distance` | Returns `flMinDistance` | Reference threshold distance. |
| `SetMinDistance` | `eax_3db_set_min_distance` | `alSourcefDirect(..., AL_REFERENCE_DISTANCE, dist)` | Direct mapping. |
| `GetMode` | `eax_3db_get_mode` | Returns `DS3DMODE_NORMAL / HEADRELATIVE / DISABLE` | Cached mode flag. |
| `SetMode` | `eax_3db_set_mode` | Sets `AL_SOURCE_RELATIVE` and spatialization enable | `DS3DMODE_DISABLE` disables spatialization. |
| `GetPosition` | `eax_3db_get_position` | Returns cached coordinates | Cached position. |
| `SetPosition` | `eax_3db_set_position` | `alSource3fDirect(..., AL_POSITION, x, y, -z)` | Negates Z coordinate. |
| `GetVelocity` | `eax_3db_get_velocity` | Returns cached velocity | Cached velocity. |
| `SetVelocity` | `eax_3db_set_velocity` | `alSource3fDirect(..., AL_VELOCITY, x, y, -z)` | Negates Z coordinate. |

### 2.5 EAX Property Sets (IKsPropertySet)

| Property Set & Scope | Flat C ABI Function | Backend Translation / OpenAL Soft Mapping | Notes |
| :--- | :--- | :--- | :--- |
| `QuerySupport` | `eax_prop_query_support` | Returns `KSPROPERTY_SUPPORT_GET \| KSPROPERTY_SUPPORT_SET` | Checks supported EAX GUIDs (1.0, 2.0, 3.0, 4.0). |
| `Get` (EAX 1.0–3.0 Listener) | `eax_prop_get_listener` | `EAXGetDirect` or extracts from `EFXEAXREVERBPROPERTIES` | Retrieves active room acoustics preset. |
| `Set` (EAX 1.0–3.0 Listener) | `eax_prop_set_listener` | `EAXSetDirect` / binds to `AL_EFFECT_EAXREVERB` slot | Translates room, reflections, decay, and diffusion. |
| `Get` (EAX 1.0–3.0 Buffer) | `eax_prop_get_buffer` | `EAXGetDirect` / reads source obstruction parameters | Reads obstruction, occlusion, and direct HF filters. |
| `Set` (EAX 1.0–3.0 Buffer) | `eax_prop_set_buffer` | `EAXSetDirect` / configures `AL_DIRECT_FILTER` / sends | Applies biquad low-pass occlusion filters. |
| `Get` / `Set` (EAX 4.0 Context) | `eax_prop_context_eax4` | Configures global context HF reference and primary FX slots | Manages multiple simultaneous EAX 4.0 effect slots. |
| `Get` / `Set` (EAX 4.0 FX Slot) | `eax_prop_fx_slot_eax4` | Binds aux effect slots `0..3` to dedicated reverb instances | Allows independent environment sends. |
| `Get` / `Set` (EAX 4.0 Source) | `eax_prop_source_eax4` | Configures multi-slot send levels and occlusion per slot | Dispatches obstruction parameters across FX slots. |

---

## 3. Realtime Engine & Memory Arena Rules

The core audio engine operates under strict real-time safety constraints required for audio processing pipelines:

### 3.1 Zero Per-Stream Dynamic Allocation (`malloc` / `free`)
1. **Voice Arenas**: All stream voice states (`eax_voice_t`), ring buffers, and sample queues are allocated from a fixed-size, pre-faulted memory arena during system initialization.
2. **Buffer Lock / Unlock Protocol**: The methods `IDirectSoundBuffer::Lock` and `Unlock` execute millions of times per session during streaming audio playback. Dynamic heap allocations (`malloc`, `calloc`, `new`, `std::vector::resize`) are **strictly prohibited** in these code paths.
3. **Circular Ring Buffers**: Buffer data pointers returned to applications reference pre-allocated contiguous circular memory maps. Mirrored virtual memory mapping (double-mapping the same physical page consecutively via `mmap`) ensures wrap-around reads and writes require zero manual boundary splitting or copying.

### 3.2 No Exceptions in Hot Execution Paths
1. All C ABI functions and internal core routines on audio execution paths are declared `noexcept`.
2. Error reporting uses explicit integer return codes (`HRESULT` / `eax_status_t`) and fixed-storage error structures.
3. No C++ exceptions may propagate across the C ABI boundary under any circumstances.

### 3.3 Flat C ABI Boundary Specification
1. All functions exposed across the boundary use standard C linkage (`extern "C"`).
2. Data structures are strictly Plain Old Data (POD) with explicit scalar widths (`uint32_t`, `int32_t`, `float`) and standard packing alignment (`#pragma pack(push, 8)`), avoiding platform-dependent padding.
3. Handles passed across the boundary are opaque pointers (`eax_context_h`, `eax_buffer_h`).
4. Designed for zero-overhead linkage with future Wine PE shims via standard `cdecl` / `stdcall` or Wine's `__wine_unix_call` interface.
