#ifndef WINE_INTEGRATION_DSOUND_COM_H
#define WINE_INTEGRATION_DSOUND_COM_H

#include <stdint.h>
#include <stddef.h>
#include "eax_native_abi.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Standard Windows COM types */
typedef int32_t HRESULT;
typedef uint32_t ULONG;
typedef uint32_t DWORD;
typedef int32_t LONG;
typedef float D3DVALUE;
typedef void* LPVOID;
typedef const void* LPCVOID;
typedef char* LPSTR;
typedef const char* LPCSTR;
typedef wchar_t* LPWSTR;
typedef const wchar_t* LPCWSTR;
typedef void* HWND;
typedef void* HANDLE;

#define S_OK                        ((HRESULT)0x00000000L)
#define S_FALSE                     ((HRESULT)0x00000001L)
#define E_FAIL                      ((HRESULT)0x80004005L)
#define E_POINTER                   ((HRESULT)0x80004003L)
#define E_INVALIDARG                ((HRESULT)0x80070057L)
#define E_OUTOFMEMORY               ((HRESULT)0x8007000EL)
#define E_NOINTERFACE               ((HRESULT)0x80004002L)
#define E_NOTIMPL                   ((HRESULT)0x80004001L)
#define DSERR_INVALIDPARAM          E_INVALIDARG
#define DSERR_BUFFERLOST            ((HRESULT)0x88780096L)
#define DSERR_CONTROLUNAVAIL        ((HRESULT)0x8878001EL)
#define DSERR_GENERIC               E_FAIL
#define DSERR_NOAGGREGATION         ((HRESULT)0x88780008L)
#define DSERR_NODRIVER              ((HRESULT)0x88780078L)
#define E_PROP_ID_UNSUPPORTED       ((HRESULT)0x80070490L)
#define E_PROP_SET_UNSUPPORTED      ((HRESULT)0x80070492L)

#define KSPROPERTY_SUPPORT_GET      0x00000001
#define KSPROPERTY_SUPPORT_SET      0x00000002

#define DS3D_IMMEDIATE              0x00000000
#define DS3D_DEFERRED               0x00000001

typedef eax_guid_t GUID;
typedef const GUID* REFGUID;
typedef const GUID* REFIID;

/* COM Interface forward declarations */
typedef struct IUnknown IUnknown;
typedef struct IDirectSound IDirectSound;
typedef struct IDirectSound8 IDirectSound8;
typedef struct IDirectSoundBuffer IDirectSoundBuffer;
typedef struct IDirectSoundBuffer8 IDirectSoundBuffer8;
typedef struct IDirectSound3DListener IDirectSound3DListener;
typedef struct IDirectSound3DBuffer IDirectSound3DBuffer;
typedef struct IKsPropertySet IKsPropertySet;

/* -------------------------------------------------------------------------- */
/* DirectSound POD structures                                                 */
/* -------------------------------------------------------------------------- */

typedef eax_waveformat_ex_t WAVEFORMATEX;
typedef eax_dscaps_t DSCAPS;
typedef eax_dsbcaps_t DSBCAPS;
typedef eax_vector_t D3DVECTOR;
typedef eax_ds3dlistener_t DS3DLISTENER;
typedef eax_ds3dbuffer_t DS3DBUFFER;

#pragma pack(push, 4)
typedef struct _DSBUFFERDESC {
    DWORD dwSize;
    DWORD dwFlags;
    DWORD dwBufferBytes;
    DWORD dwReserved;
    WAVEFORMATEX* lpwfxFormat;
    GUID guid3DAlgorithm;
} DSBUFFERDESC;
#pragma pack(pop)

typedef const DSBUFFERDESC* LPCDSBUFFERDESC;
typedef const DS3DLISTENER* LPCDS3DLISTENER;
typedef const DS3DBUFFER* LPCDS3DBUFFER;

/* -------------------------------------------------------------------------- */
/* COM VTable Definitions                                                     */
/* -------------------------------------------------------------------------- */

/* IUnknown */
typedef struct IUnknownVtbl {
    HRESULT (*QueryInterface)(IUnknown* This, REFIID riid, void** ppvObject);
    ULONG (*AddRef)(IUnknown* This);
    ULONG (*Release)(IUnknown* This);
} IUnknownVtbl;

struct IUnknown {
    const IUnknownVtbl* lpVtbl;
};

/* IDirectSound8 */
typedef struct IDirectSound8Vtbl {
    /* IUnknown */
    HRESULT (*QueryInterface)(IDirectSound8* This, REFIID riid, void** ppvObject);
    ULONG (*AddRef)(IDirectSound8* This);
    ULONG (*Release)(IDirectSound8* This);
    /* IDirectSound */
    HRESULT (*CreateSoundBuffer)(IDirectSound8* This, LPCDSBUFFERDESC pcDSBufferDesc, IDirectSoundBuffer** ppDSBuffer, IUnknown* pUnkOuter);
    HRESULT (*GetCaps)(IDirectSound8* This, DSCAPS* pDSCaps);
    HRESULT (*DuplicateSoundBuffer)(IDirectSound8* This, IDirectSoundBuffer* pDSBufferOriginal, IDirectSoundBuffer** ppDSBufferDuplicate);
    HRESULT (*SetCooperativeLevel)(IDirectSound8* This, HWND hwnd, DWORD dwLevel);
    HRESULT (*Compact)(IDirectSound8* This);
    HRESULT (*GetSpeakerConfig)(IDirectSound8* This, DWORD* pdwSpeakerConfig);
    HRESULT (*SetSpeakerConfig)(IDirectSound8* This, DWORD dwSpeakerConfig);
    HRESULT (*Initialize)(IDirectSound8* This, const GUID* pcGuidDevice);
    /* IDirectSound8 */
    HRESULT (*VerifyCertification)(IDirectSound8* This, DWORD* pdwCertified);
} IDirectSound8Vtbl;

struct IDirectSound8 {
    const IDirectSound8Vtbl* lpVtbl;
};

struct IDirectSound {
    const IDirectSound8Vtbl* lpVtbl;
};

/* IDirectSoundBuffer8 */
typedef struct IDirectSoundBuffer8Vtbl {
    /* IUnknown */
    HRESULT (*QueryInterface)(IDirectSoundBuffer8* This, REFIID riid, void** ppvObject);
    ULONG (*AddRef)(IDirectSoundBuffer8* This);
    ULONG (*Release)(IDirectSoundBuffer8* This);
    /* IDirectSoundBuffer */
    HRESULT (*GetCaps)(IDirectSoundBuffer8* This, DSBCAPS* pDSBCaps);
    HRESULT (*GetCurrentPosition)(IDirectSoundBuffer8* This, DWORD* pdwCurrentPlayCursor, DWORD* pdwCurrentWriteCursor);
    HRESULT (*GetFormat)(IDirectSoundBuffer8* This, WAVEFORMATEX* pwfxFormat, DWORD dwSizeAllocated, DWORD* pdwSizeWritten);
    HRESULT (*GetVolume)(IDirectSoundBuffer8* This, LONG* plVolume);
    HRESULT (*GetPan)(IDirectSoundBuffer8* This, LONG* plPan);
    HRESULT (*GetFrequency)(IDirectSoundBuffer8* This, DWORD* pdwFrequency);
    HRESULT (*GetStatus)(IDirectSoundBuffer8* This, DWORD* pdwStatus);
    HRESULT (*Initialize)(IDirectSoundBuffer8* This, IDirectSound* pDirectSound, LPCDSBUFFERDESC pcDSBufferDesc);
    HRESULT (*Lock)(IDirectSoundBuffer8* This, DWORD dwOffset, DWORD dwBytes, void** ppvAudioPtr1, DWORD* pdwAudioBytes1, void** ppvAudioPtr2, DWORD* pdwAudioBytes2, DWORD dwFlags);
    HRESULT (*Play)(IDirectSoundBuffer8* This, DWORD dwReserved1, DWORD dwPriority, DWORD dwFlags);
    HRESULT (*SetCurrentPosition)(IDirectSoundBuffer8* This, DWORD dwNewPosition);
    HRESULT (*SetFormat)(IDirectSoundBuffer8* This, const WAVEFORMATEX* pcfxFormat);
    HRESULT (*SetVolume)(IDirectSoundBuffer8* This, LONG lVolume);
    HRESULT (*SetPan)(IDirectSoundBuffer8* This, LONG lPan);
    HRESULT (*SetFrequency)(IDirectSoundBuffer8* This, DWORD dwFrequency);
    HRESULT (*Stop)(IDirectSoundBuffer8* This);
    HRESULT (*Unlock)(IDirectSoundBuffer8* This, void* pvAudioPtr1, DWORD dwAudioBytes1, void* pvAudioPtr2, DWORD dwAudioBytes2);
    HRESULT (*Restore)(IDirectSoundBuffer8* This);
    /* IDirectSoundBuffer8 */
    HRESULT (*SetFX)(IDirectSoundBuffer8* This, DWORD dwEffectsCount, void* pDSFXDesc, DWORD* pdwResultCodes);
    HRESULT (*AcquireResources)(IDirectSoundBuffer8* This, DWORD dwFlags, DWORD dwEffectsCount, DWORD* pdwResultCodes);
    HRESULT (*GetObjectInPath)(IDirectSoundBuffer8* This, REFGUID rguidObject, DWORD dwIndex, REFGUID rguidInterface, void** ppObject);
} IDirectSoundBuffer8Vtbl;

struct IDirectSoundBuffer8 {
    const IDirectSoundBuffer8Vtbl* lpVtbl;
};

struct IDirectSoundBuffer {
    const IDirectSoundBuffer8Vtbl* lpVtbl;
};

/* IDirectSound3DListener */
typedef struct IDirectSound3DListenerVtbl {
    /* IUnknown */
    HRESULT (*QueryInterface)(IDirectSound3DListener* This, REFIID riid, void** ppvObject);
    ULONG (*AddRef)(IDirectSound3DListener* This);
    ULONG (*Release)(IDirectSound3DListener* This);
    /* IDirectSound3DListener */
    HRESULT (*GetAllParameters)(IDirectSound3DListener* This, DS3DLISTENER* pListener);
    HRESULT (*GetDistanceFactor)(IDirectSound3DListener* This, D3DVALUE* pflDistanceFactor);
    HRESULT (*GetDopplerFactor)(IDirectSound3DListener* This, D3DVALUE* pflDopplerFactor);
    HRESULT (*GetOrientation)(IDirectSound3DListener* This, D3DVECTOR* pvOrientFront, D3DVECTOR* pvOrientTop);
    HRESULT (*GetPosition)(IDirectSound3DListener* This, D3DVECTOR* pvPosition);
    HRESULT (*GetRolloffFactor)(IDirectSound3DListener* This, D3DVALUE* pflRolloffFactor);
    HRESULT (*GetVelocity)(IDirectSound3DListener* This, D3DVECTOR* pvVelocity);
    HRESULT (*SetAllParameters)(IDirectSound3DListener* This, LPCDS3DLISTENER pcListener, DWORD dwApply);
    HRESULT (*SetDistanceFactor)(IDirectSound3DListener* This, D3DVALUE flDistanceFactor, DWORD dwApply);
    HRESULT (*SetDopplerFactor)(IDirectSound3DListener* This, D3DVALUE flDopplerFactor, DWORD dwApply);
    HRESULT (*SetOrientation)(IDirectSound3DListener* This, D3DVALUE xFront, D3DVALUE yFront, D3DVALUE zFront, D3DVALUE xTop, D3DVALUE yTop, D3DVALUE zTop, DWORD dwApply);
    HRESULT (*SetPosition)(IDirectSound3DListener* This, D3DVALUE x, D3DVALUE y, D3DVALUE z, DWORD dwApply);
    HRESULT (*SetRolloffFactor)(IDirectSound3DListener* This, D3DVALUE flRolloffFactor, DWORD dwApply);
    HRESULT (*SetVelocity)(IDirectSound3DListener* This, D3DVALUE x, D3DVALUE y, D3DVALUE z, DWORD dwApply);
    HRESULT (*CommitDeferredSettings)(IDirectSound3DListener* This);
} IDirectSound3DListenerVtbl;

struct IDirectSound3DListener {
    const IDirectSound3DListenerVtbl* lpVtbl;
};

/* IDirectSound3DBuffer */
typedef struct IDirectSound3DBufferVtbl {
    /* IUnknown */
    HRESULT (*QueryInterface)(IDirectSound3DBuffer* This, REFIID riid, void** ppvObject);
    ULONG (*AddRef)(IDirectSound3DBuffer* This);
    ULONG (*Release)(IDirectSound3DBuffer* This);
    /* IDirectSound3DBuffer */
    HRESULT (*GetAllParameters)(IDirectSound3DBuffer* This, DS3DBUFFER* pBuffer);
    HRESULT (*GetConeAngles)(IDirectSound3DBuffer* This, DWORD* pdwInsideConeAngle, DWORD* pdwOutsideConeAngle);
    HRESULT (*GetConeOrientation)(IDirectSound3DBuffer* This, D3DVECTOR* pvConeOrientation);
    HRESULT (*GetConeOutsideVolume)(IDirectSound3DBuffer* This, LONG* plConeOutsideVolume);
    HRESULT (*GetMaxDistance)(IDirectSound3DBuffer* This, D3DVALUE* pflMaxDistance);
    HRESULT (*GetMinDistance)(IDirectSound3DBuffer* This, D3DVALUE* pflMinDistance);
    HRESULT (*GetMode)(IDirectSound3DBuffer* This, DWORD* pdwMode);
    HRESULT (*GetPosition)(IDirectSound3DBuffer* This, D3DVECTOR* pvPosition);
    HRESULT (*GetVelocity)(IDirectSound3DBuffer* This, D3DVECTOR* pvVelocity);
    HRESULT (*SetAllParameters)(IDirectSound3DBuffer* This, LPCDS3DBUFFER pcBuffer, DWORD dwApply);
    HRESULT (*SetConeAngles)(IDirectSound3DBuffer* This, DWORD dwInsideConeAngle, DWORD dwOutsideConeAngle, DWORD dwApply);
    HRESULT (*SetConeOrientation)(IDirectSound3DBuffer* This, D3DVALUE x, D3DVALUE y, D3DVALUE z, DWORD dwApply);
    HRESULT (*SetConeOutsideVolume)(IDirectSound3DBuffer* This, LONG lConeOutsideVolume, DWORD dwApply);
    HRESULT (*SetMaxDistance)(IDirectSound3DBuffer* This, D3DVALUE flMaxDistance, DWORD dwApply);
    HRESULT (*SetMinDistance)(IDirectSound3DBuffer* This, D3DVALUE flMinDistance, DWORD dwApply);
    HRESULT (*SetMode)(IDirectSound3DBuffer* This, DWORD dwMode, DWORD dwApply);
    HRESULT (*SetPosition)(IDirectSound3DBuffer* This, D3DVALUE x, D3DVALUE y, D3DVALUE z, DWORD dwApply);
    HRESULT (*SetVelocity)(IDirectSound3DBuffer* This, D3DVALUE x, D3DVALUE y, D3DVALUE z, DWORD dwApply);
} IDirectSound3DBufferVtbl;

struct IDirectSound3DBuffer {
    const IDirectSound3DBufferVtbl* lpVtbl;
};

/* IKsPropertySet */
typedef struct IKsPropertySetVtbl {
    /* IUnknown */
    HRESULT (*QueryInterface)(IKsPropertySet* This, REFIID riid, void** ppvObject);
    ULONG (*AddRef)(IKsPropertySet* This);
    ULONG (*Release)(IKsPropertySet* This);
    /* IKsPropertySet */
    HRESULT (*Get)(IKsPropertySet* This, REFGUID rguidPropSet, ULONG ulId, void* pInstanceData, ULONG ulInstanceLength, void* pPropertyData, ULONG ulDataLength, ULONG* pulBytesReturned);
    HRESULT (*Set)(IKsPropertySet* This, REFGUID rguidPropSet, ULONG ulId, void* pInstanceData, ULONG ulInstanceLength, void* pPropertyData, ULONG ulDataLength);
    HRESULT (*QuerySupport)(IKsPropertySet* This, REFGUID rguidPropSet, ULONG ulId, ULONG* pulTypeSupport);
} IKsPropertySetVtbl;

struct IKsPropertySet {
    const IKsPropertySetVtbl* lpVtbl;
};

#ifdef __cplusplus
}
#endif

#endif /* WINE_INTEGRATION_DSOUND_COM_H */
