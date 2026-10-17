#ifndef WINE_INTEGRATION_DSOUND_COM_H
#define WINE_INTEGRATION_DSOUND_COM_H

#include <stdint.h>
#include <stddef.h>
#include "eax_native_abi.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Calling convention for COM methods and Win32 exports */
#ifndef DS_STDAPICALLTYPE
#  if defined(_WIN32) || defined(__WIN32__) || defined(__CYGWIN__)
#    define DS_STDAPICALLTYPE __stdcall
#  elif defined(__i386__) || defined(_M_IX86)
#    define DS_STDAPICALLTYPE __attribute__((stdcall))
#  else
#    define DS_STDAPICALLTYPE
#  endif
#endif

/* Standard Windows COM types */
typedef int32_t HRESULT;
typedef uint32_t ULONG;
typedef uint32_t DWORD;
typedef int32_t LONG;
typedef int32_t BOOL;
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
typedef const GUID* REFCLSID;

/* Callback function types */
typedef BOOL (DS_STDAPICALLTYPE *LPDSENUMCALLBACKA)(GUID* pGuid, LPCSTR strDescription, LPCSTR strModule, void* lpContext);
typedef BOOL (DS_STDAPICALLTYPE *LPDSENUMCALLBACKW)(GUID* pGuid, LPCWSTR strDescription, LPCWSTR strModule, void* lpContext);

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
/* COM VTable Definitions (All stdcall for 32-bit PE / COM ABI)               */
/* -------------------------------------------------------------------------- */

/* IUnknown */
typedef struct IUnknownVtbl {
    HRESULT (DS_STDAPICALLTYPE *QueryInterface)(IUnknown* This, REFIID riid, void** ppvObject);
    ULONG (DS_STDAPICALLTYPE *AddRef)(IUnknown* This);
    ULONG (DS_STDAPICALLTYPE *Release)(IUnknown* This);
} IUnknownVtbl;

struct IUnknown {
    const IUnknownVtbl* lpVtbl;
};

/* IDirectSound8 */
typedef struct IDirectSound8Vtbl {
    /* IUnknown */
    HRESULT (DS_STDAPICALLTYPE *QueryInterface)(IDirectSound8* This, REFIID riid, void** ppvObject);
    ULONG (DS_STDAPICALLTYPE *AddRef)(IDirectSound8* This);
    ULONG (DS_STDAPICALLTYPE *Release)(IDirectSound8* This);
    /* IDirectSound */
    HRESULT (DS_STDAPICALLTYPE *CreateSoundBuffer)(IDirectSound8* This, LPCDSBUFFERDESC pcDSBufferDesc, IDirectSoundBuffer** ppDSBuffer, IUnknown* pUnkOuter);
    HRESULT (DS_STDAPICALLTYPE *GetCaps)(IDirectSound8* This, DSCAPS* pDSCaps);
    HRESULT (DS_STDAPICALLTYPE *DuplicateSoundBuffer)(IDirectSound8* This, IDirectSoundBuffer* pDSBufferOriginal, IDirectSoundBuffer** ppDSBufferDuplicate);
    HRESULT (DS_STDAPICALLTYPE *SetCooperativeLevel)(IDirectSound8* This, HWND hwnd, DWORD dwLevel);
    HRESULT (DS_STDAPICALLTYPE *Compact)(IDirectSound8* This);
    HRESULT (DS_STDAPICALLTYPE *GetSpeakerConfig)(IDirectSound8* This, DWORD* pdwSpeakerConfig);
    HRESULT (DS_STDAPICALLTYPE *SetSpeakerConfig)(IDirectSound8* This, DWORD dwSpeakerConfig);
    HRESULT (DS_STDAPICALLTYPE *Initialize)(IDirectSound8* This, const GUID* pcGuidDevice);
    /* IDirectSound8 */
    HRESULT (DS_STDAPICALLTYPE *VerifyCertification)(IDirectSound8* This, DWORD* pdwCertified);
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
    HRESULT (DS_STDAPICALLTYPE *QueryInterface)(IDirectSoundBuffer8* This, REFIID riid, void** ppvObject);
    ULONG (DS_STDAPICALLTYPE *AddRef)(IDirectSoundBuffer8* This);
    ULONG (DS_STDAPICALLTYPE *Release)(IDirectSoundBuffer8* This);
    /* IDirectSoundBuffer */
    HRESULT (DS_STDAPICALLTYPE *GetCaps)(IDirectSoundBuffer8* This, DSBCAPS* pDSBCaps);
    HRESULT (DS_STDAPICALLTYPE *GetCurrentPosition)(IDirectSoundBuffer8* This, DWORD* pdwCurrentPlayCursor, DWORD* pdwCurrentWriteCursor);
    HRESULT (DS_STDAPICALLTYPE *GetFormat)(IDirectSoundBuffer8* This, WAVEFORMATEX* pwfxFormat, DWORD dwSizeAllocated, DWORD* pdwSizeWritten);
    HRESULT (DS_STDAPICALLTYPE *GetVolume)(IDirectSoundBuffer8* This, LONG* plVolume);
    HRESULT (DS_STDAPICALLTYPE *GetPan)(IDirectSoundBuffer8* This, LONG* plPan);
    HRESULT (DS_STDAPICALLTYPE *GetFrequency)(IDirectSoundBuffer8* This, DWORD* pdwFrequency);
    HRESULT (DS_STDAPICALLTYPE *GetStatus)(IDirectSoundBuffer8* This, DWORD* pdwStatus);
    HRESULT (DS_STDAPICALLTYPE *Initialize)(IDirectSoundBuffer8* This, IDirectSound* pDirectSound, LPCDSBUFFERDESC pcDSBufferDesc);
    HRESULT (DS_STDAPICALLTYPE *Lock)(IDirectSoundBuffer8* This, DWORD dwOffset, DWORD dwBytes, void** ppvAudioPtr1, DWORD* pdwAudioBytes1, void** ppvAudioPtr2, DWORD* pdwAudioBytes2, DWORD dwFlags);
    HRESULT (DS_STDAPICALLTYPE *Play)(IDirectSoundBuffer8* This, DWORD dwReserved1, DWORD dwPriority, DWORD dwFlags);
    HRESULT (DS_STDAPICALLTYPE *SetCurrentPosition)(IDirectSoundBuffer8* This, DWORD dwNewPosition);
    HRESULT (DS_STDAPICALLTYPE *SetFormat)(IDirectSoundBuffer8* This, const WAVEFORMATEX* pcfxFormat);
    HRESULT (DS_STDAPICALLTYPE *SetVolume)(IDirectSoundBuffer8* This, LONG lVolume);
    HRESULT (DS_STDAPICALLTYPE *SetPan)(IDirectSoundBuffer8* This, LONG lPan);
    HRESULT (DS_STDAPICALLTYPE *SetFrequency)(IDirectSoundBuffer8* This, DWORD dwFrequency);
    HRESULT (DS_STDAPICALLTYPE *Stop)(IDirectSoundBuffer8* This);
    HRESULT (DS_STDAPICALLTYPE *Unlock)(IDirectSoundBuffer8* This, void* pvAudioPtr1, DWORD dwAudioBytes1, void* pvAudioPtr2, DWORD dwAudioBytes2);
    HRESULT (DS_STDAPICALLTYPE *Restore)(IDirectSoundBuffer8* This);
    /* IDirectSoundBuffer8 */
    HRESULT (DS_STDAPICALLTYPE *SetFX)(IDirectSoundBuffer8* This, DWORD dwEffectsCount, void* pDSFXDesc, DWORD* pdwResultCodes);
    HRESULT (DS_STDAPICALLTYPE *AcquireResources)(IDirectSoundBuffer8* This, DWORD dwFlags, DWORD dwEffectsCount, DWORD* pdwResultCodes);
    HRESULT (DS_STDAPICALLTYPE *GetObjectInPath)(IDirectSoundBuffer8* This, REFGUID rguidObject, DWORD dwIndex, REFGUID rguidInterface, void** ppObject);
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
    HRESULT (DS_STDAPICALLTYPE *QueryInterface)(IDirectSound3DListener* This, REFIID riid, void** ppvObject);
    ULONG (DS_STDAPICALLTYPE *AddRef)(IDirectSound3DListener* This);
    ULONG (DS_STDAPICALLTYPE *Release)(IDirectSound3DListener* This);
    /* IDirectSound3DListener */
    HRESULT (DS_STDAPICALLTYPE *GetAllParameters)(IDirectSound3DListener* This, DS3DLISTENER* pListener);
    HRESULT (DS_STDAPICALLTYPE *GetDistanceFactor)(IDirectSound3DListener* This, D3DVALUE* pflDistanceFactor);
    HRESULT (DS_STDAPICALLTYPE *GetDopplerFactor)(IDirectSound3DListener* This, D3DVALUE* pflDopplerFactor);
    HRESULT (DS_STDAPICALLTYPE *GetOrientation)(IDirectSound3DListener* This, D3DVECTOR* pvOrientFront, D3DVECTOR* pvOrientTop);
    HRESULT (DS_STDAPICALLTYPE *GetPosition)(IDirectSound3DListener* This, D3DVECTOR* pvPosition);
    HRESULT (DS_STDAPICALLTYPE *GetRolloffFactor)(IDirectSound3DListener* This, D3DVALUE* pflRolloffFactor);
    HRESULT (DS_STDAPICALLTYPE *GetVelocity)(IDirectSound3DListener* This, D3DVECTOR* pvVelocity);
    HRESULT (DS_STDAPICALLTYPE *SetAllParameters)(IDirectSound3DListener* This, LPCDS3DLISTENER pcListener, DWORD dwApply);
    HRESULT (DS_STDAPICALLTYPE *SetDistanceFactor)(IDirectSound3DListener* This, D3DVALUE flDistanceFactor, DWORD dwApply);
    HRESULT (DS_STDAPICALLTYPE *SetDopplerFactor)(IDirectSound3DListener* This, D3DVALUE flDopplerFactor, DWORD dwApply);
    HRESULT (DS_STDAPICALLTYPE *SetOrientation)(IDirectSound3DListener* This, D3DVALUE xFront, D3DVALUE yFront, D3DVALUE zFront, D3DVALUE xTop, D3DVALUE yTop, D3DVALUE zTop, DWORD dwApply);
    HRESULT (DS_STDAPICALLTYPE *SetPosition)(IDirectSound3DListener* This, D3DVALUE x, D3DVALUE y, D3DVALUE z, DWORD dwApply);
    HRESULT (DS_STDAPICALLTYPE *SetRolloffFactor)(IDirectSound3DListener* This, D3DVALUE flRolloffFactor, DWORD dwApply);
    HRESULT (DS_STDAPICALLTYPE *SetVelocity)(IDirectSound3DListener* This, D3DVALUE x, D3DVALUE y, D3DVALUE z, DWORD dwApply);
    HRESULT (DS_STDAPICALLTYPE *CommitDeferredSettings)(IDirectSound3DListener* This);
} IDirectSound3DListenerVtbl;

struct IDirectSound3DListener {
    const IDirectSound3DListenerVtbl* lpVtbl;
};

/* IDirectSound3DBuffer */
typedef struct IDirectSound3DBufferVtbl {
    /* IUnknown */
    HRESULT (DS_STDAPICALLTYPE *QueryInterface)(IDirectSound3DBuffer* This, REFIID riid, void** ppvObject);
    ULONG (DS_STDAPICALLTYPE *AddRef)(IDirectSound3DBuffer* This);
    ULONG (DS_STDAPICALLTYPE *Release)(IDirectSound3DBuffer* This);
    /* IDirectSound3DBuffer */
    HRESULT (DS_STDAPICALLTYPE *GetAllParameters)(IDirectSound3DBuffer* This, DS3DBUFFER* pBuffer);
    HRESULT (DS_STDAPICALLTYPE *GetConeAngles)(IDirectSound3DBuffer* This, DWORD* pdwInsideConeAngle, DWORD* pdwOutsideConeAngle);
    HRESULT (DS_STDAPICALLTYPE *GetConeOrientation)(IDirectSound3DBuffer* This, D3DVECTOR* pvConeOrientation);
    HRESULT (DS_STDAPICALLTYPE *GetConeOutsideVolume)(IDirectSound3DBuffer* This, LONG* plConeOutsideVolume);
    HRESULT (DS_STDAPICALLTYPE *GetMaxDistance)(IDirectSound3DBuffer* This, D3DVALUE* pflMaxDistance);
    HRESULT (DS_STDAPICALLTYPE *GetMinDistance)(IDirectSound3DBuffer* This, D3DVALUE* pflMinDistance);
    HRESULT (DS_STDAPICALLTYPE *GetMode)(IDirectSound3DBuffer* This, DWORD* pdwMode);
    HRESULT (DS_STDAPICALLTYPE *GetPosition)(IDirectSound3DBuffer* This, D3DVECTOR* pvPosition);
    HRESULT (DS_STDAPICALLTYPE *GetVelocity)(IDirectSound3DBuffer* This, D3DVECTOR* pvVelocity);
    HRESULT (DS_STDAPICALLTYPE *SetAllParameters)(IDirectSound3DBuffer* This, LPCDS3DBUFFER pcBuffer, DWORD dwApply);
    HRESULT (DS_STDAPICALLTYPE *SetConeAngles)(IDirectSound3DBuffer* This, DWORD dwInsideConeAngle, DWORD dwOutsideConeAngle, DWORD dwApply);
    HRESULT (DS_STDAPICALLTYPE *SetConeOrientation)(IDirectSound3DBuffer* This, D3DVALUE x, D3DVALUE y, D3DVALUE z, DWORD dwApply);
    HRESULT (DS_STDAPICALLTYPE *SetConeOutsideVolume)(IDirectSound3DBuffer* This, LONG lConeOutsideVolume, DWORD dwApply);
    HRESULT (DS_STDAPICALLTYPE *SetMaxDistance)(IDirectSound3DBuffer* This, D3DVALUE flMaxDistance, DWORD dwApply);
    HRESULT (DS_STDAPICALLTYPE *SetMinDistance)(IDirectSound3DBuffer* This, D3DVALUE flMinDistance, DWORD dwApply);
    HRESULT (DS_STDAPICALLTYPE *SetMode)(IDirectSound3DBuffer* This, DWORD dwMode, DWORD dwApply);
    HRESULT (DS_STDAPICALLTYPE *SetPosition)(IDirectSound3DBuffer* This, D3DVALUE x, D3DVALUE y, D3DVALUE z, DWORD dwApply);
    HRESULT (DS_STDAPICALLTYPE *SetVelocity)(IDirectSound3DBuffer* This, D3DVALUE x, D3DVALUE y, D3DVALUE z, DWORD dwApply);
} IDirectSound3DBufferVtbl;

struct IDirectSound3DBuffer {
    const IDirectSound3DBufferVtbl* lpVtbl;
};

/* IKsPropertySet */
typedef struct IKsPropertySetVtbl {
    /* IUnknown */
    HRESULT (DS_STDAPICALLTYPE *QueryInterface)(IKsPropertySet* This, REFIID riid, void** ppvObject);
    ULONG (DS_STDAPICALLTYPE *AddRef)(IKsPropertySet* This);
    ULONG (DS_STDAPICALLTYPE *Release)(IKsPropertySet* This);
    /* IKsPropertySet */
    HRESULT (DS_STDAPICALLTYPE *Get)(IKsPropertySet* This, REFGUID rguidPropSet, ULONG ulId, void* pInstanceData, ULONG ulInstanceLength, void* pPropertyData, ULONG ulDataLength, ULONG* pulBytesReturned);
    HRESULT (DS_STDAPICALLTYPE *Set)(IKsPropertySet* This, REFGUID rguidPropSet, ULONG ulId, void* pInstanceData, ULONG ulInstanceLength, void* pPropertyData, ULONG ulDataLength);
    HRESULT (DS_STDAPICALLTYPE *QuerySupport)(IKsPropertySet* This, REFGUID rguidPropSet, ULONG ulId, ULONG* pulTypeSupport);
} IKsPropertySetVtbl;

struct IKsPropertySet {
    const IKsPropertySetVtbl* lpVtbl;
};

/* Standard DirectSound DLL Entrypoint Functions */
HRESULT DS_STDAPICALLTYPE DirectSoundCreate(const GUID* pcGuidDevice, IDirectSound** ppDS, IUnknown* pUnkOuter);
HRESULT DS_STDAPICALLTYPE DirectSoundCreate8(const GUID* pcGuidDevice, IDirectSound8** ppDS8, IUnknown* pUnkOuter);
HRESULT DS_STDAPICALLTYPE DirectSoundEnumerateA(LPDSENUMCALLBACKA pDSEnumCallback, void* pContext);
HRESULT DS_STDAPICALLTYPE DirectSoundEnumerateW(LPDSENUMCALLBACKW pDSEnumCallback, void* pContext);
HRESULT DS_STDAPICALLTYPE GetDeviceID(const GUID* pGuidSrc, GUID* pGuidDst);
HRESULT DS_STDAPICALLTYPE DllCanUnloadNow(void);
HRESULT DS_STDAPICALLTYPE DllGetClassObject(REFCLSID rclsid, REFIID riid, void** ppv);
HRESULT DS_STDAPICALLTYPE DllRegisterServer(void);
HRESULT DS_STDAPICALLTYPE DllUnregisterServer(void);

#ifdef __cplusplus
}
#endif

#endif /* WINE_INTEGRATION_DSOUND_COM_H */
