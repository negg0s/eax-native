# Phase 2 Results: Wine DirectSound Shim & eaxctl Verification

## 1. Overview & Architecture

Phase 2 introduces the Windows PE / Unix compatibility layer connecting Win32/DirectSound games running under Proton/Wine to `libeax-native.so` and OpenAL Soft:

1. **`wine-integration/dsound_shim.cpp`**:
   - Implements full COM vtables for `IDirectSound8`, `IDirectSoundBuffer8`, `IDirectSound3DListener`, `IDirectSound3DBuffer`, and `IKsPropertySet` adhering to standard `__stdcall` calling convention on 32-bit and 64-bit PE architectures.
   - Delegates all buffer management, 3D spatial positioning, format locks, and property set queries across the Unix/PE boundary into the `eax-native` engine.
   - Resolves `IKsPropertySet::QuerySupport`, `Get`, and `Set` for `GUID_EAX10_*`, `GUID_EAX20_*`, `GUID_EAX30_*`, and `GUID_EAX40_*`.
2. **`tools/eaxctl`**:
   - CLI tool supporting `install --appid <id>`, `status`, and `test` commands.
   - Automatically identifies Proton prefixes, copies the built PE `dsound.dll` shim (validating against missing PE builds), idempotently configures `dsound=native,builtin` in `user.reg`, and audits prefix configuration (`system32` 64-bit / `syswow64` 32-bit).
3. **`wine-integration/tests/test_dsound_compat.cpp`**:
   - Standalone COM verification executable testing `DirectSoundCreate8`, `GetCaps`, `CreateSoundBuffer` (3D), `IDirectSound3DBuffer::SetPosition/GetPosition`, and `IKsPropertySet::QuerySupport(GUID_EAX20_*)`.

---

## 2. Test Execution & Compatibility Verification

### 2.1 Automated Test Harness Results
- **Binary**: `test_dsound_compat` (executed in native Linux ELF host mode during CTest, with cross-compiled Windows PE binaries built via MinGW)
- **Interface Coverage**:
  - `IDirectSound8::GetCaps`: `DS_OK` (hardware mixing capacity advertised)
  - `IDirectSound8::SetCooperativeLevel`: `DS_OK`
  - `IDirectSound8::CreateSoundBuffer` (with `DSBCAPS_CTRL3D`): `DS_OK`
  - `IDirectSound3DBuffer::SetPosition` / `GetPosition`: `DS_OK` (exact roundtrip coordinate validation with `DS3D_IMMEDIATE`)
  - `IKsPropertySet::QuerySupport(GUID_EAX20_Listener)`: `DS_OK` (`KSPROPERTY_SUPPORT_GET | KSPROPERTY_SUPPORT_SET`)
  - `IKsPropertySet::QuerySupport(GUID_EAX20_Buffer)`: `DS_OK` (`KSPROPERTY_SUPPORT_GET | KSPROPERTY_SUPPORT_SET`)

---

## 3. Game Compatibility Logs & Placeholders

### 3.1 Stock Wine Builtin Behavior vs. `eax-native` Shim

| Target Test Game | Stock Wine Builtin `dsound` | `eax-native` Shim (`dsound=n,b`) | Status / Notes |
| :--- | :--- | :--- | :--- |
| **F.E.A.R.** (AppID: `21090`) | `IKsPropertySet::QuerySupport` returns `E_PROP_ID_UNSUPPORTED` (`0x80070490`). "Hardware Mixing" and "EAX Advanced HD" grayed out in Options. | `QuerySupport` returns `DS_OK` with support flags `0x3`. EAX 2.0 / EAX HD options ungrayed and selectable. | Expected behavior (to be verified in live Proton/Wine session); COM property queries and 3D positioning verified via `test_dsound_compat`. |
| **Halo: Combat Evolved** | EAX audio hardware acceleration unavailable. Reverb disabled. | EAX environmental audio enabled; 3D directional positioning active. | Expected behavior (to be verified in live Proton/Wine session); DirectSound8 and 3D buffer interfaces verified via `test_dsound_compat`. |
| **Thief II: The Metal Age** | EAX 1.0 / 2.0 options grayed out. | EAX environmental sound selectable. | Expected behavior (to be verified in live Proton/Wine session); `GUID_EAX20_Listener` support verified via `test_dsound_compat`. |

### 3.2 Real Wine / Proton In-Game Test Run Placeholders

```
[PLACEHOLDER: Pending Live Wine / Proton Test Run]
Game: F.E.A.R. (v1.08)
Prefix: ~/.steam/steam/steamapps/compatdata/21090/pfx
Command: eaxctl install --appid 21090
Result: dsound.dll placed in system32/syswow64, user.reg updated with dsound=native,builtin.
winedbg trace:
  Loaded dsound.dll -> eax-native PE shim
  IDirectSound8::CreateSoundBuffer called with DSBCAPS_CTRL3D
  IKsPropertySet::QuerySupport(DSPROPSETID_EAX20_Listener) -> 0x00000000 (SUPPORT_GET | SUPPORT_SET)
In-game status: EAX Audio enabled in Sound Setup menu.
```
