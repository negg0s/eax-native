#include "dsound_com.h"
#include "eax_native_abi.h"
#include <atomic>
#include <cstring>
#include <new>

/* Known GUID definitions */
static const GUID GUID_IUnknown = {
    0x00000000, 0x0000, 0x0000, {0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46}
};
static const GUID GUID_IDirectSound = {
    0x279AFA83, 0x4981, 0x11CE, {0xA5, 0x21, 0x00, 0x20, 0xAF, 0x0B, 0xE5, 0x60}
};
static const GUID GUID_IDirectSound8 = {
    0xC50A7E93, 0xF395, 0x4834, {0x9E, 0xF6, 0x7F, 0xA9, 0x9D, 0xE5, 0x09, 0x66}
};
static const GUID GUID_IDirectSoundBuffer = {
    0x279AFA85, 0x4981, 0x11CE, {0xA5, 0x21, 0x00, 0x20, 0xAF, 0x0B, 0xE5, 0x60}
};
static const GUID GUID_IDirectSoundBuffer8 = {
    0x6825A449, 0x7524, 0x4D82, {0x92, 0x0F, 0x50, 0xE3, 0x6A, 0xB3, 0xAB, 0x1E}
};
static const GUID GUID_IDirectSound3DListener = {
    0x279AFA84, 0x4981, 0x11CE, {0xA5, 0x21, 0x00, 0x20, 0xAF, 0x0B, 0xE5, 0x60}
};
static const GUID GUID_IDirectSound3DBuffer = {
    0x279AFA86, 0x4981, 0x11CE, {0xA5, 0x21, 0x00, 0x20, 0xAF, 0x0B, 0xE5, 0x60}
};
static const GUID GUID_IKsPropertySet = {
    0x31EFAC30, 0x515C, 0x11D0, {0xA9, 0xAA, 0x00, 0xAA, 0x00, 0x61, 0xBE, 0x93}
};

static inline bool IsEqualGUID(const GUID* a, const GUID* b) {
    if (!a || !b) return false;
    return (a->data1 == b->data1) && (a->data2 == b->data2) && (a->data3 == b->data3) &&
           (std::memcmp(a->data4, b->data4, 8) == 0);
}

/* Forward declarations */
struct DirectSoundBufferImpl;
struct DirectSoundImpl;

/* -------------------------------------------------------------------------- */
/* DirectSoundBuffer Implementation                                           */
/* -------------------------------------------------------------------------- */
struct DirectSoundBufferImpl {
    IDirectSoundBuffer8 lpVtblBuffer;
    IDirectSound3DBuffer lpVtbl3DBuffer;
    IKsPropertySet lpVtblPropSet;

    std::atomic<ULONG> refCount{1};
    DirectSoundImpl* dsParent{nullptr};
    eax_buffer_h nativeBuffer{nullptr};
    DSBUFFERDESC desc{};
    WAVEFORMATEX format{};
    bool isPrimary{false};

    DirectSoundBufferImpl(DirectSoundImpl* parent, eax_buffer_h nb, const DSBUFFERDESC* d, bool primary);
    ~DirectSoundBufferImpl();

    HRESULT QueryInterface(REFIID riid, void** ppvObject);
    ULONG AddRef();
    ULONG Release();
};

/* -------------------------------------------------------------------------- */
/* DirectSound Implementation                                                 */
/* -------------------------------------------------------------------------- */
struct DirectSoundImpl {
    IDirectSound8 lpVtblDS8;
    IDirectSound3DListener lpVtblListener;
    IKsPropertySet lpVtblPropSet;

    std::atomic<ULONG> refCount{1};
    eax_context_h nativeContext{nullptr};
    DWORD speakerConfig{EAX_SPEAKER_CONFIG_STEREO};

    DirectSoundImpl();
    ~DirectSoundImpl();

    HRESULT QueryInterface(REFIID riid, void** ppvObject);
    ULONG AddRef();
    ULONG Release();
};

/* -------------------------------------------------------------------------- */
/* DirectSoundImpl COM Methods                                                */
/* -------------------------------------------------------------------------- */

static HRESULT DS_STDAPICALLTYPE DS_QueryInterface(IDirectSound8* This, REFIID riid, void** ppvObject) {
    if (!This || !ppvObject) return E_POINTER;
    auto* self = reinterpret_cast<DirectSoundImpl*>(This);
    return self->QueryInterface(riid, ppvObject);
}

static ULONG DS_STDAPICALLTYPE DS_AddRef(IDirectSound8* This) {
    if (!This) return 0;
    auto* self = reinterpret_cast<DirectSoundImpl*>(This);
    return self->AddRef();
}

static ULONG DS_STDAPICALLTYPE DS_Release(IDirectSound8* This) {
    if (!This) return 0;
    auto* self = reinterpret_cast<DirectSoundImpl*>(This);
    return self->Release();
}

static HRESULT DS_STDAPICALLTYPE DS_CreateSoundBuffer(IDirectSound8* This, LPCDSBUFFERDESC pcDSBufferDesc, IDirectSoundBuffer** ppDSBuffer, IUnknown* pUnkOuter) {
    if (!This || !pcDSBufferDesc || !ppDSBuffer) return DSERR_INVALIDPARAM;
    if (pUnkOuter) return DSERR_NOAGGREGATION;

    auto* self = reinterpret_cast<DirectSoundImpl*>(This);
    const bool isPrimary = (pcDSBufferDesc->dwFlags & 0x00000001 /* DSBCAPS_PRIMARYBUFFER */) != 0;

    eax_dsbcaps_t caps{};
    caps.dwSize = sizeof(eax_dsbcaps_t);
    caps.dwFlags = pcDSBufferDesc->dwFlags;
    caps.dwBufferBytes = pcDSBufferDesc->dwBufferBytes;

    eax_waveformat_ex_t fmt{};
    if (pcDSBufferDesc->lpwfxFormat) {
        fmt = *(pcDSBufferDesc->lpwfxFormat);
    } else {
        fmt.wFormatTag = 1;
        fmt.nChannels = 2;
        fmt.nSamplesPerSec = 44100;
        fmt.wBitsPerSample = 16;
        fmt.nBlockAlign = 4;
        fmt.nAvgBytesPerSec = 44100 * 4;
        fmt.cbSize = 0;
    }

    eax_buffer_h nb = nullptr;
    eax_result_t res = eax_buffer_create(self->nativeContext, &caps, &fmt, &nb);
    if (res != EAX_OK) return DSERR_GENERIC;

    auto* buf = new (std::nothrow) DirectSoundBufferImpl(self, nb, pcDSBufferDesc, isPrimary);
    if (!buf) {
        eax_buffer_destroy(nb);
        return E_OUTOFMEMORY;
    }

    *ppDSBuffer = reinterpret_cast<IDirectSoundBuffer*>(&buf->lpVtblBuffer);
    return S_OK;
}

static HRESULT DS_STDAPICALLTYPE DS_GetCaps(IDirectSound8* This, DSCAPS* pDSCaps) {
    if (!This || !pDSCaps) return DSERR_INVALIDPARAM;
    auto* self = reinterpret_cast<DirectSoundImpl*>(This);
    return (eax_context_get_caps(self->nativeContext, pDSCaps) == EAX_OK) ? S_OK : DSERR_GENERIC;
}

static HRESULT DS_STDAPICALLTYPE DS_DuplicateSoundBuffer(IDirectSound8* This, IDirectSoundBuffer* pDSBufferOriginal, IDirectSoundBuffer** ppDSBufferDuplicate) {
    if (!This || !pDSBufferOriginal || !ppDSBufferDuplicate) return DSERR_INVALIDPARAM;
    auto* self = reinterpret_cast<DirectSoundImpl*>(This);
    auto* srcImpl = reinterpret_cast<DirectSoundBufferImpl*>(pDSBufferOriginal);

    eax_buffer_h dupNative = nullptr;
    eax_result_t res = eax_buffer_duplicate(self->nativeContext, srcImpl->nativeBuffer, &dupNative);
    if (res != EAX_OK) return DSERR_GENERIC;

    auto* dup = new (std::nothrow) DirectSoundBufferImpl(self, dupNative, &srcImpl->desc, false);
    if (!dup) {
        eax_buffer_destroy(dupNative);
        return E_OUTOFMEMORY;
    }

    *ppDSBufferDuplicate = reinterpret_cast<IDirectSoundBuffer*>(&dup->lpVtblBuffer);
    return S_OK;
}

static HRESULT DS_STDAPICALLTYPE DS_SetCooperativeLevel(IDirectSound8* /*This*/, HWND /*hwnd*/, DWORD /*dwLevel*/) {
    return S_OK;
}

static HRESULT DS_STDAPICALLTYPE DS_Compact(IDirectSound8* /*This*/) {
    return S_OK;
}

static HRESULT DS_STDAPICALLTYPE DS_GetSpeakerConfig(IDirectSound8* This, DWORD* pdwSpeakerConfig) {
    if (!This || !pdwSpeakerConfig) return DSERR_INVALIDPARAM;
    auto* self = reinterpret_cast<DirectSoundImpl*>(This);
    return (eax_context_get_speaker_config(self->nativeContext, pdwSpeakerConfig) == EAX_OK) ? S_OK : DSERR_GENERIC;
}

static HRESULT DS_STDAPICALLTYPE DS_SetSpeakerConfig(IDirectSound8* This, DWORD dwSpeakerConfig) {
    if (!This) return DSERR_INVALIDPARAM;
    auto* self = reinterpret_cast<DirectSoundImpl*>(This);
    return (eax_context_set_speaker_config(self->nativeContext, dwSpeakerConfig) == EAX_OK) ? S_OK : DSERR_GENERIC;
}

static HRESULT DS_STDAPICALLTYPE DS_Initialize(IDirectSound8* /*This*/, const GUID* /*pcGuidDevice*/) {
    return S_OK;
}

static HRESULT DS_STDAPICALLTYPE DS_VerifyCertification(IDirectSound8* /*This*/, DWORD* pdwCertified) {
    if (pdwCertified) *pdwCertified = 0x00000001; /* DS_CERTIFIED */
    return S_OK;
}

/* -------------------------------------------------------------------------- */
/* IDirectSound3DListener COM Methods                                         */
/* -------------------------------------------------------------------------- */

static DirectSoundImpl* Listener_Impl(IDirectSound3DListener* This) {
    return reinterpret_cast<DirectSoundImpl*>(reinterpret_cast<char*>(This) - offsetof(DirectSoundImpl, lpVtblListener));
}

static HRESULT DS_STDAPICALLTYPE Listener_QueryInterface(IDirectSound3DListener* This, REFIID riid, void** ppvObject) {
    return Listener_Impl(This)->QueryInterface(riid, ppvObject);
}

static ULONG DS_STDAPICALLTYPE Listener_AddRef(IDirectSound3DListener* This) {
    return Listener_Impl(This)->AddRef();
}

static ULONG DS_STDAPICALLTYPE Listener_Release(IDirectSound3DListener* This) {
    return Listener_Impl(This)->Release();
}

static HRESULT DS_STDAPICALLTYPE Listener_GetAllParameters(IDirectSound3DListener* This, DS3DLISTENER* pListener) {
    if (!This || !pListener) return DSERR_INVALIDPARAM;
    return (eax_listener_get_all_parameters(Listener_Impl(This)->nativeContext, pListener) == EAX_OK) ? S_OK : DSERR_GENERIC;
}

static HRESULT DS_STDAPICALLTYPE Listener_GetDistanceFactor(IDirectSound3DListener* This, D3DVALUE* pflDistanceFactor) {
    if (!This || !pflDistanceFactor) return DSERR_INVALIDPARAM;
    return (eax_listener_get_distance_factor(Listener_Impl(This)->nativeContext, pflDistanceFactor) == EAX_OK) ? S_OK : DSERR_GENERIC;
}

static HRESULT DS_STDAPICALLTYPE Listener_GetDopplerFactor(IDirectSound3DListener* This, D3DVALUE* pflDopplerFactor) {
    if (!This || !pflDopplerFactor) return DSERR_INVALIDPARAM;
    return (eax_listener_get_doppler_factor(Listener_Impl(This)->nativeContext, pflDopplerFactor) == EAX_OK) ? S_OK : DSERR_GENERIC;
}

static HRESULT DS_STDAPICALLTYPE Listener_GetOrientation(IDirectSound3DListener* This, D3DVECTOR* pvOrientFront, D3DVECTOR* pvOrientTop) {
    if (!This || !pvOrientFront || !pvOrientTop) return DSERR_INVALIDPARAM;
    return (eax_listener_get_orientation(Listener_Impl(This)->nativeContext, pvOrientFront, pvOrientTop) == EAX_OK) ? S_OK : DSERR_GENERIC;
}

static HRESULT DS_STDAPICALLTYPE Listener_GetPosition(IDirectSound3DListener* This, D3DVECTOR* pvPosition) {
    if (!This || !pvPosition) return DSERR_INVALIDPARAM;
    return (eax_listener_get_position(Listener_Impl(This)->nativeContext, pvPosition) == EAX_OK) ? S_OK : DSERR_GENERIC;
}

static HRESULT DS_STDAPICALLTYPE Listener_GetRolloffFactor(IDirectSound3DListener* This, D3DVALUE* pflRolloffFactor) {
    if (!This || !pflRolloffFactor) return DSERR_INVALIDPARAM;
    return (eax_listener_get_rolloff_factor(Listener_Impl(This)->nativeContext, pflRolloffFactor) == EAX_OK) ? S_OK : DSERR_GENERIC;
}

static HRESULT DS_STDAPICALLTYPE Listener_GetVelocity(IDirectSound3DListener* This, D3DVECTOR* pvVelocity) {
    if (!This || !pvVelocity) return DSERR_INVALIDPARAM;
    return (eax_listener_get_velocity(Listener_Impl(This)->nativeContext, pvVelocity) == EAX_OK) ? S_OK : DSERR_GENERIC;
}

static HRESULT DS_STDAPICALLTYPE Listener_SetAllParameters(IDirectSound3DListener* This, LPCDS3DLISTENER pcListener, DWORD dwApply) {
    if (!This || !pcListener) return DSERR_INVALIDPARAM;
    return (eax_listener_set_all_parameters(Listener_Impl(This)->nativeContext, pcListener, dwApply) == EAX_OK) ? S_OK : DSERR_GENERIC;
}

static HRESULT DS_STDAPICALLTYPE Listener_SetDistanceFactor(IDirectSound3DListener* This, D3DVALUE flDistanceFactor, DWORD dwApply) {
    return (eax_listener_set_distance_factor(Listener_Impl(This)->nativeContext, flDistanceFactor, dwApply) == EAX_OK) ? S_OK : DSERR_GENERIC;
}

static HRESULT DS_STDAPICALLTYPE Listener_SetDopplerFactor(IDirectSound3DListener* This, D3DVALUE flDopplerFactor, DWORD dwApply) {
    return (eax_listener_set_doppler_factor(Listener_Impl(This)->nativeContext, flDopplerFactor, dwApply) == EAX_OK) ? S_OK : DSERR_GENERIC;
}

static HRESULT DS_STDAPICALLTYPE Listener_SetOrientation(IDirectSound3DListener* This, D3DVALUE xFront, D3DVALUE yFront, D3DVALUE zFront, D3DVALUE xTop, D3DVALUE yTop, D3DVALUE zTop, DWORD dwApply) {
    return (eax_listener_set_orientation(Listener_Impl(This)->nativeContext, xFront, yFront, zFront, xTop, yTop, zTop, dwApply) == EAX_OK) ? S_OK : DSERR_GENERIC;
}

static HRESULT DS_STDAPICALLTYPE Listener_SetPosition(IDirectSound3DListener* This, D3DVALUE x, D3DVALUE y, D3DVALUE z, DWORD dwApply) {
    return (eax_listener_set_position(Listener_Impl(This)->nativeContext, x, y, z, dwApply) == EAX_OK) ? S_OK : DSERR_GENERIC;
}

static HRESULT DS_STDAPICALLTYPE Listener_SetRolloffFactor(IDirectSound3DListener* This, D3DVALUE flRolloffFactor, DWORD dwApply) {
    return (eax_listener_set_rolloff_factor(Listener_Impl(This)->nativeContext, flRolloffFactor, dwApply) == EAX_OK) ? S_OK : DSERR_GENERIC;
}

static HRESULT DS_STDAPICALLTYPE Listener_SetVelocity(IDirectSound3DListener* This, D3DVALUE x, D3DVALUE y, D3DVALUE z, DWORD dwApply) {
    return (eax_listener_set_velocity(Listener_Impl(This)->nativeContext, x, y, z, dwApply) == EAX_OK) ? S_OK : DSERR_GENERIC;
}

static HRESULT DS_STDAPICALLTYPE Listener_CommitDeferredSettings(IDirectSound3DListener* This) {
    return (eax_listener_commit_deferred(Listener_Impl(This)->nativeContext) == EAX_OK) ? S_OK : DSERR_GENERIC;
}

/* -------------------------------------------------------------------------- */
/* DirectSound Context IKsPropertySet COM Methods                             */
/* -------------------------------------------------------------------------- */

static DirectSoundImpl* DSProp_Impl(IKsPropertySet* This) {
    return reinterpret_cast<DirectSoundImpl*>(reinterpret_cast<char*>(This) - offsetof(DirectSoundImpl, lpVtblPropSet));
}

static HRESULT DS_STDAPICALLTYPE DSProp_QueryInterface(IKsPropertySet* This, REFIID riid, void** ppvObject) {
    return DSProp_Impl(This)->QueryInterface(riid, ppvObject);
}

static ULONG DS_STDAPICALLTYPE DSProp_AddRef(IKsPropertySet* This) {
    return DSProp_Impl(This)->AddRef();
}

static ULONG DS_STDAPICALLTYPE DSProp_Release(IKsPropertySet* This) {
    return DSProp_Impl(This)->Release();
}

static HRESULT DS_STDAPICALLTYPE DSProp_Get(IKsPropertySet* This, REFGUID rguidPropSet, ULONG ulId, void* pInstanceData, ULONG ulInstanceLength, void* pPropertyData, ULONG ulDataLength, ULONG* pulBytesReturned) {
    if (!This || !rguidPropSet || !pPropertyData) return E_POINTER;
    eax_result_t res = eax_prop_get(DSProp_Impl(This)->nativeContext, nullptr, rguidPropSet, ulId, pInstanceData, ulInstanceLength, pPropertyData, ulDataLength, pulBytesReturned);
    if (res == EAX_OK) return S_OK;
    if (res == EAX_ERR_PROP_UNSUPPORTED) return E_PROP_ID_UNSUPPORTED;
    return E_FAIL;
}

static HRESULT DS_STDAPICALLTYPE DSProp_Set(IKsPropertySet* This, REFGUID rguidPropSet, ULONG ulId, void* pInstanceData, ULONG ulInstanceLength, void* pPropertyData, ULONG ulDataLength) {
    if (!This || !rguidPropSet || !pPropertyData) return E_POINTER;
    eax_result_t res = eax_prop_set(DSProp_Impl(This)->nativeContext, nullptr, rguidPropSet, ulId, pInstanceData, ulInstanceLength, pPropertyData, ulDataLength);
    if (res == EAX_OK) return S_OK;
    if (res == EAX_ERR_PROP_UNSUPPORTED) return E_PROP_ID_UNSUPPORTED;
    return E_FAIL;
}

static HRESULT DS_STDAPICALLTYPE DSProp_QuerySupport(IKsPropertySet* This, REFGUID rguidPropSet, ULONG ulId, ULONG* pulTypeSupport) {
    if (!This || !rguidPropSet || !pulTypeSupport) return E_POINTER;
    eax_result_t res = eax_prop_query_support(DSProp_Impl(This)->nativeContext, nullptr, rguidPropSet, ulId, pulTypeSupport);
    if (res == EAX_OK) return S_OK;
    return E_PROP_ID_UNSUPPORTED;
}

/* -------------------------------------------------------------------------- */
/* DirectSoundBuffer COM Methods                                              */
/* -------------------------------------------------------------------------- */

static HRESULT DS_STDAPICALLTYPE Buf_QueryInterface(IDirectSoundBuffer8* This, REFIID riid, void** ppvObject) {
    if (!This || !ppvObject) return E_POINTER;
    auto* self = reinterpret_cast<DirectSoundBufferImpl*>(This);
    return self->QueryInterface(riid, ppvObject);
}

static ULONG DS_STDAPICALLTYPE Buf_AddRef(IDirectSoundBuffer8* This) {
    if (!This) return 0;
    auto* self = reinterpret_cast<DirectSoundBufferImpl*>(This);
    return self->AddRef();
}

static ULONG DS_STDAPICALLTYPE Buf_Release(IDirectSoundBuffer8* This) {
    if (!This) return 0;
    auto* self = reinterpret_cast<DirectSoundBufferImpl*>(This);
    return self->Release();
}

static HRESULT DS_STDAPICALLTYPE Buf_GetCaps(IDirectSoundBuffer8* This, DSBCAPS* pDSBCaps) {
    if (!This || !pDSBCaps) return DSERR_INVALIDPARAM;
    auto* self = reinterpret_cast<DirectSoundBufferImpl*>(This);
    return (eax_buffer_get_caps(self->nativeBuffer, pDSBCaps) == EAX_OK) ? S_OK : DSERR_GENERIC;
}

static HRESULT DS_STDAPICALLTYPE Buf_GetCurrentPosition(IDirectSoundBuffer8* This, DWORD* pdwCurrentPlayCursor, DWORD* pdwCurrentWriteCursor) {
    if (!This) return DSERR_INVALIDPARAM;
    auto* self = reinterpret_cast<DirectSoundBufferImpl*>(This);
    return (eax_buffer_get_current_position(self->nativeBuffer, pdwCurrentPlayCursor, pdwCurrentWriteCursor) == EAX_OK) ? S_OK : DSERR_GENERIC;
}

static HRESULT DS_STDAPICALLTYPE Buf_GetFormat(IDirectSoundBuffer8* This, WAVEFORMATEX* pwfxFormat, DWORD dwSizeAllocated, DWORD* pdwSizeWritten) {
    if (!This) return DSERR_INVALIDPARAM;
    auto* self = reinterpret_cast<DirectSoundBufferImpl*>(This);
    if (pwfxFormat && dwSizeAllocated >= sizeof(WAVEFORMATEX)) {
        eax_buffer_get_format(self->nativeBuffer, pwfxFormat);
    }
    if (pdwSizeWritten) *pdwSizeWritten = sizeof(WAVEFORMATEX);
    return S_OK;
}

static HRESULT DS_STDAPICALLTYPE Buf_GetVolume(IDirectSoundBuffer8* This, LONG* plVolume) {
    if (!This || !plVolume) return DSERR_INVALIDPARAM;
    auto* self = reinterpret_cast<DirectSoundBufferImpl*>(This);
    return (eax_buffer_get_volume(self->nativeBuffer, plVolume) == EAX_OK) ? S_OK : DSERR_GENERIC;
}

static HRESULT DS_STDAPICALLTYPE Buf_GetPan(IDirectSoundBuffer8* This, LONG* plPan) {
    if (!This || !plPan) return DSERR_INVALIDPARAM;
    auto* self = reinterpret_cast<DirectSoundBufferImpl*>(This);
    return (eax_buffer_get_pan(self->nativeBuffer, plPan) == EAX_OK) ? S_OK : DSERR_GENERIC;
}

static HRESULT DS_STDAPICALLTYPE Buf_GetFrequency(IDirectSoundBuffer8* This, DWORD* pdwFrequency) {
    if (!This || !pdwFrequency) return DSERR_INVALIDPARAM;
    auto* self = reinterpret_cast<DirectSoundBufferImpl*>(This);
    return (eax_buffer_get_frequency(self->nativeBuffer, pdwFrequency) == EAX_OK) ? S_OK : DSERR_GENERIC;
}

static HRESULT DS_STDAPICALLTYPE Buf_GetStatus(IDirectSoundBuffer8* This, DWORD* pdwStatus) {
    if (!This || !pdwStatus) return DSERR_INVALIDPARAM;
    auto* self = reinterpret_cast<DirectSoundBufferImpl*>(This);
    return (eax_buffer_get_status(self->nativeBuffer, pdwStatus) == EAX_OK) ? S_OK : DSERR_GENERIC;
}

static HRESULT DS_STDAPICALLTYPE Buf_Initialize(IDirectSoundBuffer8* /*This*/, IDirectSound* /*pDirectSound*/, LPCDSBUFFERDESC /*pcDSBufferDesc*/) {
    return S_OK;
}

static HRESULT DS_STDAPICALLTYPE Buf_Lock(IDirectSoundBuffer8* This, DWORD dwOffset, DWORD dwBytes, void** ppvAudioPtr1, DWORD* pdwAudioBytes1, void** ppvAudioPtr2, DWORD* pdwAudioBytes2, DWORD dwFlags) {
    if (!This || !ppvAudioPtr1 || !pdwAudioBytes1) return DSERR_INVALIDPARAM;
    auto* self = reinterpret_cast<DirectSoundBufferImpl*>(This);
    return (eax_buffer_lock(self->nativeBuffer, dwOffset, dwBytes, ppvAudioPtr1, pdwAudioBytes1, ppvAudioPtr2, pdwAudioBytes2, dwFlags) == EAX_OK) ? S_OK : DSERR_GENERIC;
}

static HRESULT DS_STDAPICALLTYPE Buf_Play(IDirectSoundBuffer8* This, DWORD /*dwReserved1*/, DWORD dwPriority, DWORD dwFlags) {
    if (!This) return DSERR_INVALIDPARAM;
    auto* self = reinterpret_cast<DirectSoundBufferImpl*>(This);
    return (eax_buffer_play(self->nativeBuffer, dwPriority, dwFlags) == EAX_OK) ? S_OK : DSERR_GENERIC;
}

static HRESULT DS_STDAPICALLTYPE Buf_SetCurrentPosition(IDirectSoundBuffer8* This, DWORD dwNewPosition) {
    if (!This) return DSERR_INVALIDPARAM;
    auto* self = reinterpret_cast<DirectSoundBufferImpl*>(This);
    return (eax_buffer_set_current_position(self->nativeBuffer, dwNewPosition) == EAX_OK) ? S_OK : DSERR_GENERIC;
}

static HRESULT DS_STDAPICALLTYPE Buf_SetFormat(IDirectSoundBuffer8* This, const WAVEFORMATEX* pcfxFormat) {
    if (!This) return DSERR_INVALIDPARAM;
    auto* self = reinterpret_cast<DirectSoundBufferImpl*>(This);
    if (!self->isPrimary) {
        // DirectSound specification: SetFormat is only valid on the primary sound buffer
        return E_FAIL;
    }
    if (!pcfxFormat) return DSERR_INVALIDPARAM;
    // TODO: Dynamic hardware device rate/channel reconfiguration is Phase 3 scope.
    self->format = *pcfxFormat;
    return S_OK;
}

static HRESULT DS_STDAPICALLTYPE Buf_SetVolume(IDirectSoundBuffer8* This, LONG lVolume) {
    if (!This) return DSERR_INVALIDPARAM;
    auto* self = reinterpret_cast<DirectSoundBufferImpl*>(This);
    return (eax_buffer_set_volume(self->nativeBuffer, lVolume) == EAX_OK) ? S_OK : DSERR_GENERIC;
}

static HRESULT DS_STDAPICALLTYPE Buf_SetPan(IDirectSoundBuffer8* This, LONG lPan) {
    if (!This) return DSERR_INVALIDPARAM;
    auto* self = reinterpret_cast<DirectSoundBufferImpl*>(This);
    return (eax_buffer_set_pan(self->nativeBuffer, lPan) == EAX_OK) ? S_OK : DSERR_GENERIC;
}

static HRESULT DS_STDAPICALLTYPE Buf_SetFrequency(IDirectSoundBuffer8* This, DWORD dwFrequency) {
    if (!This) return DSERR_INVALIDPARAM;
    auto* self = reinterpret_cast<DirectSoundBufferImpl*>(This);
    return (eax_buffer_set_frequency(self->nativeBuffer, dwFrequency) == EAX_OK) ? S_OK : DSERR_GENERIC;
}

static HRESULT DS_STDAPICALLTYPE Buf_Stop(IDirectSoundBuffer8* This) {
    if (!This) return DSERR_INVALIDPARAM;
    auto* self = reinterpret_cast<DirectSoundBufferImpl*>(This);
    return (eax_buffer_stop(self->nativeBuffer) == EAX_OK) ? S_OK : DSERR_GENERIC;
}

static HRESULT DS_STDAPICALLTYPE Buf_Unlock(IDirectSoundBuffer8* This, void* pvAudioPtr1, DWORD dwAudioBytes1, void* pvAudioPtr2, DWORD dwAudioBytes2) {
    if (!This) return DSERR_INVALIDPARAM;
    auto* self = reinterpret_cast<DirectSoundBufferImpl*>(This);
    return (eax_buffer_unlock(self->nativeBuffer, pvAudioPtr1, dwAudioBytes1, pvAudioPtr2, dwAudioBytes2) == EAX_OK) ? S_OK : DSERR_GENERIC;
}

static HRESULT DS_STDAPICALLTYPE Buf_Restore(IDirectSoundBuffer8* /*This*/) {
    return S_OK;
}

static HRESULT DS_STDAPICALLTYPE Buf_SetFX(IDirectSoundBuffer8* /*This*/, DWORD /*dwEffectsCount*/, void* /*pDSFXDesc*/, DWORD* /*pdwResultCodes*/) {
    return DSERR_CONTROLUNAVAIL;
}

static HRESULT DS_STDAPICALLTYPE Buf_AcquireResources(IDirectSoundBuffer8* /*This*/, DWORD /*dwFlags*/, DWORD /*dwEffectsCount*/, DWORD* /*pdwResultCodes*/) {
    return S_OK;
}

static HRESULT DS_STDAPICALLTYPE Buf_GetObjectInPath(IDirectSoundBuffer8* /*This*/, REFGUID /*rguidObject*/, DWORD /*dwIndex*/, REFGUID /*rguidInterface*/, void** /*ppObject*/) {
    return DSERR_CONTROLUNAVAIL;
}

/* -------------------------------------------------------------------------- */
/* IDirectSound3DBuffer COM Methods                                           */
/* -------------------------------------------------------------------------- */

static DirectSoundBufferImpl* Buf3D_Impl(IDirectSound3DBuffer* This) {
    return reinterpret_cast<DirectSoundBufferImpl*>(reinterpret_cast<char*>(This) - offsetof(DirectSoundBufferImpl, lpVtbl3DBuffer));
}

static HRESULT DS_STDAPICALLTYPE Buf3D_QueryInterface(IDirectSound3DBuffer* This, REFIID riid, void** ppvObject) {
    return Buf3D_Impl(This)->QueryInterface(riid, ppvObject);
}

static ULONG DS_STDAPICALLTYPE Buf3D_AddRef(IDirectSound3DBuffer* This) {
    return Buf3D_Impl(This)->AddRef();
}

static ULONG DS_STDAPICALLTYPE Buf3D_Release(IDirectSound3DBuffer* This) {
    return Buf3D_Impl(This)->Release();
}

static HRESULT DS_STDAPICALLTYPE Buf3D_GetAllParameters(IDirectSound3DBuffer* This, DS3DBUFFER* pBuffer) {
    if (!This || !pBuffer) return DSERR_INVALIDPARAM;
    return (eax_buffer_get_3d_all_parameters(Buf3D_Impl(This)->nativeBuffer, pBuffer) == EAX_OK) ? S_OK : DSERR_GENERIC;
}

static HRESULT DS_STDAPICALLTYPE Buf3D_GetConeAngles(IDirectSound3DBuffer* This, DWORD* pdwInsideConeAngle, DWORD* pdwOutsideConeAngle) {
    if (!This || !pdwInsideConeAngle || !pdwOutsideConeAngle) return DSERR_INVALIDPARAM;
    return (eax_buffer_get_3d_cone_angles(Buf3D_Impl(This)->nativeBuffer, pdwInsideConeAngle, pdwOutsideConeAngle) == EAX_OK) ? S_OK : DSERR_GENERIC;
}

static HRESULT DS_STDAPICALLTYPE Buf3D_GetConeOrientation(IDirectSound3DBuffer* This, D3DVECTOR* pvConeOrientation) {
    if (!This || !pvConeOrientation) return DSERR_INVALIDPARAM;
    return (eax_buffer_get_3d_cone_orientation(Buf3D_Impl(This)->nativeBuffer, pvConeOrientation) == EAX_OK) ? S_OK : DSERR_GENERIC;
}

static HRESULT DS_STDAPICALLTYPE Buf3D_GetConeOutsideVolume(IDirectSound3DBuffer* This, LONG* plConeOutsideVolume) {
    if (!This || !plConeOutsideVolume) return DSERR_INVALIDPARAM;
    return (eax_buffer_get_3d_cone_outside_volume(Buf3D_Impl(This)->nativeBuffer, plConeOutsideVolume) == EAX_OK) ? S_OK : DSERR_GENERIC;
}

static HRESULT DS_STDAPICALLTYPE Buf3D_GetMaxDistance(IDirectSound3DBuffer* This, D3DVALUE* pflMaxDistance) {
    if (!This || !pflMaxDistance) return DSERR_INVALIDPARAM;
    return (eax_buffer_get_3d_max_distance(Buf3D_Impl(This)->nativeBuffer, pflMaxDistance) == EAX_OK) ? S_OK : DSERR_GENERIC;
}

static HRESULT DS_STDAPICALLTYPE Buf3D_GetMinDistance(IDirectSound3DBuffer* This, D3DVALUE* pflMinDistance) {
    if (!This || !pflMinDistance) return DSERR_INVALIDPARAM;
    return (eax_buffer_get_3d_min_distance(Buf3D_Impl(This)->nativeBuffer, pflMinDistance) == EAX_OK) ? S_OK : DSERR_GENERIC;
}

static HRESULT DS_STDAPICALLTYPE Buf3D_GetMode(IDirectSound3DBuffer* This, DWORD* pdwMode) {
    if (!This || !pdwMode) return DSERR_INVALIDPARAM;
    return (eax_buffer_get_3d_mode(Buf3D_Impl(This)->nativeBuffer, pdwMode) == EAX_OK) ? S_OK : DSERR_GENERIC;
}

static HRESULT DS_STDAPICALLTYPE Buf3D_GetPosition(IDirectSound3DBuffer* This, D3DVECTOR* pvPosition) {
    if (!This || !pvPosition) return DSERR_INVALIDPARAM;
    return (eax_buffer_get_3d_position(Buf3D_Impl(This)->nativeBuffer, pvPosition) == EAX_OK) ? S_OK : DSERR_GENERIC;
}

static HRESULT DS_STDAPICALLTYPE Buf3D_GetVelocity(IDirectSound3DBuffer* This, D3DVECTOR* pvVelocity) {
    if (!This || !pvVelocity) return DSERR_INVALIDPARAM;
    return (eax_buffer_get_3d_velocity(Buf3D_Impl(This)->nativeBuffer, pvVelocity) == EAX_OK) ? S_OK : DSERR_GENERIC;
}

static HRESULT DS_STDAPICALLTYPE Buf3D_SetAllParameters(IDirectSound3DBuffer* This, LPCDS3DBUFFER pcBuffer, DWORD dwApply) {
    if (!This || !pcBuffer) return DSERR_INVALIDPARAM;
    return (eax_buffer_set_3d_all_parameters(Buf3D_Impl(This)->nativeBuffer, pcBuffer, dwApply) == EAX_OK) ? S_OK : DSERR_GENERIC;
}

static HRESULT DS_STDAPICALLTYPE Buf3D_SetConeAngles(IDirectSound3DBuffer* This, DWORD dwInsideConeAngle, DWORD dwOutsideConeAngle, DWORD dwApply) {
    return (eax_buffer_set_3d_cone_angles(Buf3D_Impl(This)->nativeBuffer, dwInsideConeAngle, dwOutsideConeAngle, dwApply) == EAX_OK) ? S_OK : DSERR_GENERIC;
}

static HRESULT DS_STDAPICALLTYPE Buf3D_SetConeOrientation(IDirectSound3DBuffer* This, D3DVALUE x, D3DVALUE y, D3DVALUE z, DWORD dwApply) {
    return (eax_buffer_set_3d_cone_orientation(Buf3D_Impl(This)->nativeBuffer, x, y, z, dwApply) == EAX_OK) ? S_OK : DSERR_GENERIC;
}

static HRESULT DS_STDAPICALLTYPE Buf3D_SetConeOutsideVolume(IDirectSound3DBuffer* This, LONG lConeOutsideVolume, DWORD dwApply) {
    return (eax_buffer_set_3d_cone_outside_volume(Buf3D_Impl(This)->nativeBuffer, lConeOutsideVolume, dwApply) == EAX_OK) ? S_OK : DSERR_GENERIC;
}

static HRESULT DS_STDAPICALLTYPE Buf3D_SetMaxDistance(IDirectSound3DBuffer* This, D3DVALUE flMaxDistance, DWORD dwApply) {
    return (eax_buffer_set_3d_max_distance(Buf3D_Impl(This)->nativeBuffer, flMaxDistance, dwApply) == EAX_OK) ? S_OK : DSERR_GENERIC;
}

static HRESULT DS_STDAPICALLTYPE Buf3D_SetMinDistance(IDirectSound3DBuffer* This, D3DVALUE flMinDistance, DWORD dwApply) {
    return (eax_buffer_set_3d_min_distance(Buf3D_Impl(This)->nativeBuffer, flMinDistance, dwApply) == EAX_OK) ? S_OK : DSERR_GENERIC;
}

static HRESULT DS_STDAPICALLTYPE Buf3D_SetMode(IDirectSound3DBuffer* This, DWORD dwMode, DWORD dwApply) {
    return (eax_buffer_set_3d_mode(Buf3D_Impl(This)->nativeBuffer, dwMode, dwApply) == EAX_OK) ? S_OK : DSERR_GENERIC;
}

static HRESULT DS_STDAPICALLTYPE Buf3D_SetPosition(IDirectSound3DBuffer* This, D3DVALUE x, D3DVALUE y, D3DVALUE z, DWORD dwApply) {
    return (eax_buffer_set_3d_position(Buf3D_Impl(This)->nativeBuffer, x, y, z, dwApply) == EAX_OK) ? S_OK : DSERR_GENERIC;
}

static HRESULT DS_STDAPICALLTYPE Buf3D_SetVelocity(IDirectSound3DBuffer* This, D3DVALUE x, D3DVALUE y, D3DVALUE z, DWORD dwApply) {
    return (eax_buffer_set_3d_velocity(Buf3D_Impl(This)->nativeBuffer, x, y, z, dwApply) == EAX_OK) ? S_OK : DSERR_GENERIC;
}

/* -------------------------------------------------------------------------- */
/* DirectSoundBuffer IKsPropertySet COM Methods                               */
/* -------------------------------------------------------------------------- */

static DirectSoundBufferImpl* BufProp_Impl(IKsPropertySet* This) {
    return reinterpret_cast<DirectSoundBufferImpl*>(reinterpret_cast<char*>(This) - offsetof(DirectSoundBufferImpl, lpVtblPropSet));
}

static HRESULT DS_STDAPICALLTYPE BufProp_QueryInterface(IKsPropertySet* This, REFIID riid, void** ppvObject) {
    return BufProp_Impl(This)->QueryInterface(riid, ppvObject);
}

static ULONG DS_STDAPICALLTYPE BufProp_AddRef(IKsPropertySet* This) {
    return BufProp_Impl(This)->AddRef();
}

static ULONG DS_STDAPICALLTYPE BufProp_Release(IKsPropertySet* This) {
    return BufProp_Impl(This)->Release();
}

static HRESULT DS_STDAPICALLTYPE BufProp_Get(IKsPropertySet* This, REFGUID rguidPropSet, ULONG ulId, void* pInstanceData, ULONG ulInstanceLength, void* pPropertyData, ULONG ulDataLength, ULONG* pulBytesReturned) {
    if (!This || !rguidPropSet || !pPropertyData) return E_POINTER;
    auto* self = BufProp_Impl(This);
    eax_result_t res = eax_prop_get(self->dsParent->nativeContext, self->nativeBuffer, rguidPropSet, ulId, pInstanceData, ulInstanceLength, pPropertyData, ulDataLength, pulBytesReturned);
    if (res == EAX_OK) return S_OK;
    if (res == EAX_ERR_PROP_UNSUPPORTED) return E_PROP_ID_UNSUPPORTED;
    return E_FAIL;
}

static HRESULT DS_STDAPICALLTYPE BufProp_Set(IKsPropertySet* This, REFGUID rguidPropSet, ULONG ulId, void* pInstanceData, ULONG ulInstanceLength, void* pPropertyData, ULONG ulDataLength) {
    if (!This || !rguidPropSet || !pPropertyData) return E_POINTER;
    auto* self = BufProp_Impl(This);
    eax_result_t res = eax_prop_set(self->dsParent->nativeContext, self->nativeBuffer, rguidPropSet, ulId, pInstanceData, ulInstanceLength, pPropertyData, ulDataLength);
    if (res == EAX_OK) return S_OK;
    if (res == EAX_ERR_PROP_UNSUPPORTED) return E_PROP_ID_UNSUPPORTED;
    return E_FAIL;
}

static HRESULT DS_STDAPICALLTYPE BufProp_QuerySupport(IKsPropertySet* This, REFGUID rguidPropSet, ULONG ulId, ULONG* pulTypeSupport) {
    if (!This || !rguidPropSet || !pulTypeSupport) return E_POINTER;
    auto* self = BufProp_Impl(This);
    eax_result_t res = eax_prop_query_support(self->dsParent->nativeContext, self->nativeBuffer, rguidPropSet, ulId, pulTypeSupport);
    if (res == EAX_OK) return S_OK;
    return E_PROP_ID_UNSUPPORTED;
}

/* -------------------------------------------------------------------------- */
/* Static VTable Tables                                                       */
/* -------------------------------------------------------------------------- */

static const IDirectSound8Vtbl g_DS8Vtbl = {
    DS_QueryInterface,
    DS_AddRef,
    DS_Release,
    DS_CreateSoundBuffer,
    DS_GetCaps,
    DS_DuplicateSoundBuffer,
    DS_SetCooperativeLevel,
    DS_Compact,
    DS_GetSpeakerConfig,
    DS_SetSpeakerConfig,
    DS_Initialize,
    DS_VerifyCertification
};

static const IDirectSound3DListenerVtbl g_ListenerVtbl = {
    Listener_QueryInterface,
    Listener_AddRef,
    Listener_Release,
    Listener_GetAllParameters,
    Listener_GetDistanceFactor,
    Listener_GetDopplerFactor,
    Listener_GetOrientation,
    Listener_GetPosition,
    Listener_GetRolloffFactor,
    Listener_GetVelocity,
    Listener_SetAllParameters,
    Listener_SetDistanceFactor,
    Listener_SetDopplerFactor,
    Listener_SetOrientation,
    Listener_SetPosition,
    Listener_SetRolloffFactor,
    Listener_SetVelocity,
    Listener_CommitDeferredSettings
};

static const IKsPropertySetVtbl g_DSPropVtbl = {
    DSProp_QueryInterface,
    DSProp_AddRef,
    DSProp_Release,
    DSProp_Get,
    DSProp_Set,
    DSProp_QuerySupport
};

static const IDirectSoundBuffer8Vtbl g_Buf8Vtbl = {
    Buf_QueryInterface,
    Buf_AddRef,
    Buf_Release,
    Buf_GetCaps,
    Buf_GetCurrentPosition,
    Buf_GetFormat,
    Buf_GetVolume,
    Buf_GetPan,
    Buf_GetFrequency,
    Buf_GetStatus,
    Buf_Initialize,
    Buf_Lock,
    Buf_Play,
    Buf_SetCurrentPosition,
    Buf_SetFormat,
    Buf_SetVolume,
    Buf_SetPan,
    Buf_SetFrequency,
    Buf_Stop,
    Buf_Unlock,
    Buf_Restore,
    Buf_SetFX,
    Buf_AcquireResources,
    Buf_GetObjectInPath
};

static const IDirectSound3DBufferVtbl g_Buf3DVtbl = {
    Buf3D_QueryInterface,
    Buf3D_AddRef,
    Buf3D_Release,
    Buf3D_GetAllParameters,
    Buf3D_GetConeAngles,
    Buf3D_GetConeOrientation,
    Buf3D_GetConeOutsideVolume,
    Buf3D_GetMaxDistance,
    Buf3D_GetMinDistance,
    Buf3D_GetMode,
    Buf3D_GetPosition,
    Buf3D_GetVelocity,
    Buf3D_SetAllParameters,
    Buf3D_SetConeAngles,
    Buf3D_SetConeOrientation,
    Buf3D_SetConeOutsideVolume,
    Buf3D_SetMaxDistance,
    Buf3D_SetMinDistance,
    Buf3D_SetMode,
    Buf3D_SetPosition,
    Buf3D_SetVelocity
};

static const IKsPropertySetVtbl g_BufPropVtbl = {
    BufProp_QueryInterface,
    BufProp_AddRef,
    BufProp_Release,
    BufProp_Get,
    BufProp_Set,
    BufProp_QuerySupport
};

/* -------------------------------------------------------------------------- */
/* Object Lifecycle & QueryInterface implementations                         */
/* -------------------------------------------------------------------------- */

DirectSoundImpl::DirectSoundImpl() {
    lpVtblDS8.lpVtbl = &g_DS8Vtbl;
    lpVtblListener.lpVtbl = &g_ListenerVtbl;
    lpVtblPropSet.lpVtbl = &g_DSPropVtbl;
    eax_context_create(&nativeContext);
}

DirectSoundImpl::~DirectSoundImpl() {
    if (nativeContext) {
        eax_context_destroy(nativeContext);
        nativeContext = nullptr;
    }
}

HRESULT DirectSoundImpl::QueryInterface(REFIID riid, void** ppvObject) {
    if (!ppvObject) return E_POINTER;
    *ppvObject = nullptr;

    if (IsEqualGUID(riid, &GUID_IUnknown) ||
        IsEqualGUID(riid, &GUID_IDirectSound) ||
        IsEqualGUID(riid, &GUID_IDirectSound8)) {
        AddRef();
        *ppvObject = &lpVtblDS8;
        return S_OK;
    }
    if (IsEqualGUID(riid, &GUID_IDirectSound3DListener)) {
        AddRef();
        *ppvObject = &lpVtblListener;
        return S_OK;
    }
    if (IsEqualGUID(riid, &GUID_IKsPropertySet)) {
        AddRef();
        *ppvObject = &lpVtblPropSet;
        return S_OK;
    }

    return E_NOINTERFACE;
}

ULONG DirectSoundImpl::AddRef() {
    return refCount.fetch_add(1, std::memory_order_relaxed) + 1;
}

ULONG DirectSoundImpl::Release() {
    ULONG ret = refCount.fetch_sub(1, std::memory_order_relaxed) - 1;
    if (ret == 0) {
        delete this;
    }
    return ret;
}

DirectSoundBufferImpl::DirectSoundBufferImpl(DirectSoundImpl* parent, eax_buffer_h nb, const DSBUFFERDESC* d, bool primary)
    : dsParent(parent), nativeBuffer(nb), isPrimary(primary) {
    lpVtblBuffer.lpVtbl = &g_Buf8Vtbl;
    lpVtbl3DBuffer.lpVtbl = &g_Buf3DVtbl;
    lpVtblPropSet.lpVtbl = &g_BufPropVtbl;
    if (d) desc = *d;
    if (d && d->lpwfxFormat) format = *(d->lpwfxFormat);
    if (dsParent) dsParent->AddRef();
}

DirectSoundBufferImpl::~DirectSoundBufferImpl() {
    if (nativeBuffer) {
        eax_buffer_destroy(nativeBuffer);
        nativeBuffer = nullptr;
    }
    if (dsParent) {
        dsParent->Release();
        dsParent = nullptr;
    }
}

HRESULT DirectSoundBufferImpl::QueryInterface(REFIID riid, void** ppvObject) {
    if (!ppvObject) return E_POINTER;
    *ppvObject = nullptr;

    if (IsEqualGUID(riid, &GUID_IUnknown) ||
        IsEqualGUID(riid, &GUID_IDirectSoundBuffer) ||
        IsEqualGUID(riid, &GUID_IDirectSoundBuffer8)) {
        AddRef();
        *ppvObject = &lpVtblBuffer;
        return S_OK;
    }
    if (IsEqualGUID(riid, &GUID_IDirectSound3DBuffer)) {
        if (!(desc.dwFlags & 0x00000010 /* DSBCAPS_CTRL3D */)) {
            return E_NOINTERFACE;
        }
        AddRef();
        *ppvObject = &lpVtbl3DBuffer;
        return S_OK;
    }
    if (IsEqualGUID(riid, &GUID_IDirectSound3DListener) && isPrimary) {
        return dsParent->QueryInterface(riid, ppvObject);
    }
    if (IsEqualGUID(riid, &GUID_IKsPropertySet)) {
        AddRef();
        *ppvObject = &lpVtblPropSet;
        return S_OK;
    }

    return E_NOINTERFACE;
}

ULONG DirectSoundBufferImpl::AddRef() {
    return refCount.fetch_add(1, std::memory_order_relaxed) + 1;
}

ULONG DirectSoundBufferImpl::Release() {
    ULONG ret = refCount.fetch_sub(1, std::memory_order_relaxed) - 1;
    if (ret == 0) {
        delete this;
    }
    return ret;
}

/* -------------------------------------------------------------------------- */
/* Standard Windows PE Entry Points & DirectSound Factory Exports            */
/* -------------------------------------------------------------------------- */

#if defined(_WIN32) || defined(__CYGWIN__)
    #define DSOUND_EXPORT __declspec(dllexport)
#else
    #define DSOUND_EXPORT __attribute__((visibility("default")))
#endif

extern "C" {

DSOUND_EXPORT HRESULT DS_STDAPICALLTYPE DirectSoundCreate(const GUID* /*pcGuidDevice*/, IDirectSound** ppDS, IUnknown* pUnkOuter) {
    if (!ppDS) return DSERR_INVALIDPARAM;
    if (pUnkOuter) return DSERR_NOAGGREGATION;

    auto* impl = new (std::nothrow) DirectSoundImpl();
    if (!impl) return E_OUTOFMEMORY;

    *ppDS = reinterpret_cast<IDirectSound*>(&impl->lpVtblDS8);
    return S_OK;
}

DSOUND_EXPORT HRESULT DS_STDAPICALLTYPE DirectSoundCreate8(const GUID* pcGuidDevice, IDirectSound8** ppDS8, IUnknown* pUnkOuter) {
    if (!ppDS8) return DSERR_INVALIDPARAM;
    if (pUnkOuter) return DSERR_NOAGGREGATION;

    IDirectSound* ds = nullptr;
    HRESULT hr = DirectSoundCreate(pcGuidDevice, &ds, pUnkOuter);
    if (hr != S_OK) return hr;

    *ppDS8 = reinterpret_cast<IDirectSound8*>(ds);
    return S_OK;
}

DSOUND_EXPORT HRESULT DS_STDAPICALLTYPE DirectSoundEnumerateA(LPDSENUMCALLBACKA pDSEnumCallback, void* pContext) {
    if (pDSEnumCallback) {
        static const GUID DSDEVID_DefaultPlayback = {
            0xDEF00000, 0x9C6D, 0x47ED, {0xAA, 0xF1, 0x4D, 0xDA, 0x8F, 0x2B, 0x5C, 0x03}
        };
        GUID devGuid = DSDEVID_DefaultPlayback;
        pDSEnumCallback(&devGuid, "Primary Sound Driver", "", pContext);
    }
    return S_OK;
}

DSOUND_EXPORT HRESULT DS_STDAPICALLTYPE DirectSoundEnumerateW(LPDSENUMCALLBACKW pDSEnumCallback, void* pContext) {
    if (pDSEnumCallback) {
        static const GUID DSDEVID_DefaultPlayback = {
            0xDEF00000, 0x9C6D, 0x47ED, {0xAA, 0xF1, 0x4D, 0xDA, 0x8F, 0x2B, 0x5C, 0x03}
        };
        GUID devGuid = DSDEVID_DefaultPlayback;
        pDSEnumCallback(&devGuid, L"Primary Sound Driver", L"", pContext);
    }
    return S_OK;
}

DSOUND_EXPORT HRESULT DS_STDAPICALLTYPE GetDeviceID(const GUID* /*pGuidSrc*/, GUID* pGuidDst) {
    if (pGuidDst) {
        static const GUID DSDEVID_DefaultPlayback = {
            0xDEF00000, 0x9C6D, 0x47ED, {0xAA, 0xF1, 0x4D, 0xDA, 0x8F, 0x2B, 0x5C, 0x03}
        };
        *pGuidDst = DSDEVID_DefaultPlayback;
    }
    return S_OK;
}

DSOUND_EXPORT HRESULT DS_STDAPICALLTYPE DllCanUnloadNow(void) {
    return S_FALSE;
}

DSOUND_EXPORT HRESULT DS_STDAPICALLTYPE DllGetClassObject(REFCLSID /*rclsid*/, REFIID /*riid*/, void** ppv) {
    if (ppv) *ppv = nullptr;
    return E_NOTIMPL;
}

DSOUND_EXPORT HRESULT DS_STDAPICALLTYPE DllRegisterServer(void) {
    return S_OK;
}

DSOUND_EXPORT HRESULT DS_STDAPICALLTYPE DllUnregisterServer(void) {
    return S_OK;
}

} // extern "C"
