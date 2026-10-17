#include "dsound_com.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cassert>

/* GUIDs */
static const GUID GUID_IUnknown = {
    0x00000000, 0x0000, 0x0000, {0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46}
};
static const GUID GUID_IDirectSound = {
    0x279AFA83, 0x4981, 0x11CE, {0xA5, 0x21, 0x00, 0x20, 0xAF, 0x0B, 0xE5, 0x60}
};
static const GUID GUID_IDirectSoundBuffer = {
    0x279AFA85, 0x4981, 0x11CE, {0xA5, 0x21, 0x00, 0x20, 0xAF, 0x0B, 0xE5, 0x60}
};
static const GUID GUID_IDirectSound3DBuffer = {
    0x279AFA86, 0x4981, 0x11CE, {0xA5, 0x21, 0x00, 0x20, 0xAF, 0x0B, 0xE5, 0x60}
};
static const GUID GUID_IDirectSound3DListener = {
    0x279AFA84, 0x4981, 0x11CE, {0xA5, 0x21, 0x00, 0x20, 0xAF, 0x0B, 0xE5, 0x60}
};
static const GUID GUID_IKsPropertySet = {
    0x31EFAC30, 0x515C, 0x11D0, {0xA9, 0xAA, 0x00, 0xAA, 0x00, 0x61, 0xBE, 0x93}
};

static const GUID GUID_EAX20_Listener = {
    0x0306A6A8, 0xB224, 0x11D2, {0x99, 0xE5, 0x00, 0x00, 0xE8, 0xD8, 0xC7, 0x22}
};
static const GUID GUID_EAX20_Buffer = {
    0x0306A6A7, 0xB224, 0x11D2, {0x99, 0xE5, 0x00, 0x00, 0xE8, 0xD8, 0xC7, 0x22}
};

extern "C" {
    HRESULT DS_STDAPICALLTYPE DirectSoundCreate(const GUID* pcGuidDevice, IDirectSound** ppDS, IUnknown* pUnkOuter);
    HRESULT DS_STDAPICALLTYPE DirectSoundCreate8(const GUID* pcGuidDevice, IDirectSound8** ppDS8, IUnknown* pUnkOuter);
}

int main() {
    printf("[wine_dsound_test] Starting DirectSound + EAX COM test harness...\n");

    IDirectSound8* ds8 = nullptr;
    HRESULT hr = DirectSoundCreate8(nullptr, &ds8, nullptr);
    if (hr != S_OK || !ds8) {
        fprintf(stderr, "FAIL: DirectSoundCreate8 returned 0x%08X\n", (unsigned int)hr);
        return 1;
    }
    printf("[wine_dsound_test] DirectSoundCreate8: OK\n");

    // 1. Verify SetCooperativeLevel
    hr = ds8->lpVtbl->SetCooperativeLevel(ds8, nullptr, 1 /* DSSCL_NORMAL */);
    if (hr != S_OK) {
        fprintf(stderr, "FAIL: SetCooperativeLevel returned 0x%08X\n", (unsigned int)hr);
        return 1;
    }
    printf("[wine_dsound_test] SetCooperativeLevel: OK\n");

    // 2. Verify GetCaps
    DSCAPS caps{};
    caps.dwSize = sizeof(DSCAPS);
    hr = ds8->lpVtbl->GetCaps(ds8, &caps);
    if (hr != S_OK || caps.dwMaxHwMixingAllBuffers == 0) {
        fprintf(stderr, "FAIL: GetCaps returned 0x%08X, hw mixing=%u\n", (unsigned int)hr, (unsigned int)caps.dwMaxHwMixingAllBuffers);
        return 1;
    }
    printf("[wine_dsound_test] GetCaps: OK (Max HW Mixing = %u)\n", (unsigned int)caps.dwMaxHwMixingAllBuffers);

    // 3. Query IKsPropertySet on Listener / DirectSound
    IKsPropertySet* dsProp = nullptr;
    hr = ds8->lpVtbl->QueryInterface(ds8, &GUID_IKsPropertySet, reinterpret_cast<void**>(&dsProp));
    if (hr != S_OK || !dsProp) {
        fprintf(stderr, "FAIL: QueryInterface for IKsPropertySet on DirectSound failed: 0x%08X\n", (unsigned int)hr);
        return 1;
    }
    printf("[wine_dsound_test] QueryInterface IKsPropertySet on DirectSound: OK\n");

    // 4. Verify EAX 2.0 Listener QuerySupport
    ULONG support = 0;
    hr = dsProp->lpVtbl->QuerySupport(dsProp, &GUID_EAX20_Listener, 0, &support);
    if (hr != S_OK || (support & (KSPROPERTY_SUPPORT_GET | KSPROPERTY_SUPPORT_SET)) == 0) {
        fprintf(stderr, "FAIL: QuerySupport for GUID_EAX20_Listener returned 0x%08X, support=0x%08X\n", (unsigned int)hr, (unsigned int)support);
        return 1;
    }
    printf("[wine_dsound_test] IKsPropertySet::QuerySupport(GUID_EAX20_Listener): OK (support=0x%08X)\n", (unsigned int)support);

    // 5. Create 3D Sound Buffer
    WAVEFORMATEX wfx{};
    wfx.wFormatTag = 1;
    wfx.nChannels = 1;
    wfx.nSamplesPerSec = 44100;
    wfx.wBitsPerSample = 16;
    wfx.nBlockAlign = 2;
    wfx.nAvgBytesPerSec = 44100 * 2;
    wfx.cbSize = 0;

    DSBUFFERDESC bdesc{};
    bdesc.dwSize = sizeof(DSBUFFERDESC);
    bdesc.dwFlags = 0x00000010 /* DSBCAPS_CTRL3D */ | 0x00000080 /* DSBCAPS_CTRLVOLUME */;
    bdesc.dwBufferBytes = 44100 * 2;
    bdesc.lpwfxFormat = &wfx;

    IDirectSoundBuffer* dsBuf = nullptr;
    hr = ds8->lpVtbl->CreateSoundBuffer(ds8, &bdesc, &dsBuf, nullptr);
    if (hr != S_OK || !dsBuf) {
        fprintf(stderr, "FAIL: CreateSoundBuffer returned 0x%08X\n", (unsigned int)hr);
        return 1;
    }
    printf("[wine_dsound_test] CreateSoundBuffer (3D): OK\n");

    // 6. Query IDirectSound3DBuffer
    IDirectSound3DBuffer* ds3dBuf = nullptr;
    hr = dsBuf->lpVtbl->QueryInterface(reinterpret_cast<IDirectSoundBuffer8*>(dsBuf), &GUID_IDirectSound3DBuffer, reinterpret_cast<void**>(&ds3dBuf));
    if (hr != S_OK || !ds3dBuf) {
        fprintf(stderr, "FAIL: QueryInterface IDirectSound3DBuffer returned 0x%08X\n", (unsigned int)hr);
        return 1;
    }
    printf("[wine_dsound_test] QueryInterface IDirectSound3DBuffer: OK\n");

    // 7. Set 3D Position
    hr = ds3dBuf->lpVtbl->SetPosition(ds3dBuf, 5.0f, 0.0f, 10.0f, DS3D_IMMEDIATE);
    if (hr != S_OK) {
        fprintf(stderr, "FAIL: SetPosition returned 0x%08X\n", (unsigned int)hr);
        return 1;
    }
    D3DVECTOR pos{};
    hr = ds3dBuf->lpVtbl->GetPosition(ds3dBuf, &pos);
    if (hr != S_OK || pos.x != 5.0f || pos.z != 10.0f) {
        fprintf(stderr, "FAIL: GetPosition returned 0x%08X, pos=(%f,%f,%f)\n", (unsigned int)hr, pos.x, pos.y, pos.z);
        return 1;
    }
    printf("[wine_dsound_test] IDirectSound3DBuffer SetPosition/GetPosition: OK (%f, %f, %f)\n", pos.x, pos.y, pos.z);

    // 8. Query IKsPropertySet on Buffer and query EAX 2.0 Buffer Support
    IKsPropertySet* bufProp = nullptr;
    hr = dsBuf->lpVtbl->QueryInterface(reinterpret_cast<IDirectSoundBuffer8*>(dsBuf), &GUID_IKsPropertySet, reinterpret_cast<void**>(&bufProp));
    if (hr != S_OK || !bufProp) {
        fprintf(stderr, "FAIL: QueryInterface IKsPropertySet on buffer returned 0x%08X\n", (unsigned int)hr);
        return 1;
    }

    support = 0;
    hr = bufProp->lpVtbl->QuerySupport(bufProp, &GUID_EAX20_Buffer, 0, &support);
    if (hr != S_OK || (support & (KSPROPERTY_SUPPORT_GET | KSPROPERTY_SUPPORT_SET)) == 0) {
        fprintf(stderr, "FAIL: QuerySupport for GUID_EAX20_Buffer returned 0x%08X, support=0x%08X\n", (unsigned int)hr, (unsigned int)support);
        return 1;
    }
    printf("[wine_dsound_test] IKsPropertySet::QuerySupport(GUID_EAX20_Buffer): OK (support=0x%08X)\n", (unsigned int)support);

    bufProp->lpVtbl->Release(bufProp);
    ds3dBuf->lpVtbl->Release(ds3dBuf);
    reinterpret_cast<IDirectSoundBuffer8*>(dsBuf)->lpVtbl->Release(reinterpret_cast<IDirectSoundBuffer8*>(dsBuf));
    dsProp->lpVtbl->Release(dsProp);
    ds8->lpVtbl->Release(ds8);

    printf("[wine_dsound_test] ALL COM & EAX COMPATIBILITY CHECKS PASSED!\n");
    return 0;
}
