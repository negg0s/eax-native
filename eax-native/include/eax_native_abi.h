#ifndef EAX_NATIVE_ABI_H
#define EAX_NATIVE_ABI_H

#include <stdint.h>
#include <stddef.h>

#if defined(_WIN32) || defined(__CYGWIN__)
    #if defined(EAX_NATIVE_BUILD_SHARED)
        #define EAX_API __declspec(dllexport)
    #else
        #define EAX_API __declspec(dllimport)
    #endif
#else
    #if defined(__GNUC__) && __GNUC__ >= 4
        #define EAX_API __attribute__((visibility("default")))
    #else
        #define EAX_API
    #endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Result status codes matching standard COM / DirectSound HRESULTs */
typedef int32_t eax_result_t;
#define EAX_OK                      ((eax_result_t)0x00000000L) /* DS_OK */
#define EAX_FALSE                   ((eax_result_t)0x00000001L) /* S_FALSE */
#define EAX_ERR_GENERIC             ((eax_result_t)0x80004005L) /* E_FAIL */
#define EAX_ERR_POINTER             ((eax_result_t)0x80004003L) /* E_POINTER */
#define EAX_ERR_INVALID_PARAM       ((eax_result_t)0x80070057L) /* E_INVALIDARG / DSERR_INVALIDPARAM */
#define EAX_ERR_OUT_OF_MEMORY       ((eax_result_t)0x8007000EL) /* E_OUTOFMEMORY */
#define EAX_ERR_NOT_SUPPORTED       ((eax_result_t)0x80004001L) /* E_NOTIMPL */
#define EAX_ERR_BUFFER_LOST         ((eax_result_t)0x88780096L) /* DSERR_BUFFERLOST */
#define EAX_ERR_PROP_UNSUPPORTED    ((eax_result_t)0x80070490L) /* E_PROP_ID_UNSUPPORTED */

/* Opaque handles */
typedef struct eax_context_s*  eax_context_h;
typedef struct eax_buffer_s*   eax_buffer_h;
typedef struct eax_listener_s* eax_listener_h;

/* GUID structure matching Windows GUID */
#pragma pack(push, 4)
typedef struct eax_guid_s {
    uint32_t data1;
    uint16_t data2;
    uint16_t data3;
    uint8_t  data4[8];
} eax_guid_t;
#pragma pack(pop)

/* DirectSound Capabilities (DSCAPS) - 96 bytes (4-byte alignment) */
#pragma pack(push, 4)
typedef struct eax_dscaps_s {
    uint32_t dwSize;
    uint32_t dwFlags;
    uint32_t dwMinSecondarySampleRate;
    uint32_t dwMaxSecondarySampleRate;
    uint32_t dwPrimaryBuffers;
    uint32_t dwMaxHwMixingAllBuffers;
    uint32_t dwMaxHwMixingStaticBuffers;
    uint32_t dwMaxHwMixingStreamingBuffers;
    uint32_t dwFreeHwMixingAllBuffers;
    uint32_t dwFreeHwMixingStaticBuffers;
    uint32_t dwFreeHwMixingStreamingBuffers;
    uint32_t dwMaxHw3DAllBuffers;
    uint32_t dwMaxHw3DStaticBuffers;
    uint32_t dwMaxHw3DStreamingBuffers;
    uint32_t dwFreeHw3DAllBuffers;
    uint32_t dwFreeHw3DStaticBuffers;
    uint32_t dwFreeHw3DStreamingBuffers;
    uint32_t dwTotalHwMemBytes;
    uint32_t dwFreeHwMemBytes;
    uint32_t dwMaxContigFreeHwMemBytes;
    uint32_t dwUnlockTransferRateHwBuffers;
    uint32_t dwPlayCpuOverheadSwBuffers;
    uint32_t dwReserved1;
    uint32_t dwReserved2;
} eax_dscaps_t;
#pragma pack(pop)

/* DirectSound Buffer Capabilities (DSBCAPS) - 20 bytes (4-byte alignment) */
#pragma pack(push, 4)
typedef struct eax_dsbcaps_s {
    uint32_t dwSize;
    uint32_t dwFlags;
    uint32_t dwBufferBytes;
    uint32_t dwUnlockTransferRate;
    uint32_t dwPlayCpuOverhead;
} eax_dsbcaps_t;
#pragma pack(pop)

/* Audio Format Header (WAVEFORMATEX) - 18 bytes (strictly 2-byte packed per Win32 SDK) */
#pragma pack(push, 2)
typedef struct eax_waveformat_ex_s {
    uint16_t wFormatTag;
    uint16_t nChannels;
    uint32_t nSamplesPerSec;
    uint32_t nAvgBytesPerSec;
    uint16_t nBlockAlign;
    uint16_t wBitsPerSample;
    uint16_t cbSize;
} eax_waveformat_ex_t;
#pragma pack(pop)

/* 3D Vector (D3DVECTOR / DS3DVECTOR) - 12 bytes */
#pragma pack(push, 4)
typedef struct eax_vector_s {
    float x;
    float y;
    float z;
} eax_vector_t;
#pragma pack(pop)

/* DirectSound 3D Listener (DS3DLISTENER) - 64 bytes */
#pragma pack(push, 4)
typedef struct eax_ds3dlistener_s {
    uint32_t      dwSize;
    eax_vector_t  vPosition;
    eax_vector_t  vVelocity;
    eax_vector_t  vOrientFront;
    eax_vector_t  vOrientTop;
    float         flDistanceFactor;
    float         flRolloffFactor;
    float         flDopplerFactor;
} eax_ds3dlistener_t;
#pragma pack(pop)

/* DirectSound 3D Buffer (DS3DBUFFER) - 64 bytes */
#pragma pack(push, 4)
typedef struct eax_ds3dbuffer_s {
    uint32_t      dwSize;
    eax_vector_t  vPosition;
    eax_vector_t  vVelocity;
    uint32_t      dwInsideConeAngle;
    uint32_t      dwOutsideConeAngle;
    eax_vector_t  vConeOrientation;
    int32_t       lConeOutsideVolume;
    float         flMinDistance;
    float         flMaxDistance;
    uint32_t      dwMode;
} eax_ds3dbuffer_t;
#pragma pack(pop)

/* EAX 1.0 Reverb Properties - 16 bytes */
#pragma pack(push, 4)
typedef struct eax10_reverb_properties_s {
    uint32_t dwEnvironment;
    float    fVolume;
    float    fDecayTime_sec;
    float    fDamping;
} eax10_reverb_properties_t;
#pragma pack(pop)

/* EAX 1.0 Buffer Reverb Mix - 4 bytes */
#pragma pack(push, 4)
typedef struct eax10_buffer_properties_s {
    float fMix;
} eax10_buffer_properties_t;
#pragma pack(pop)

/* EAX 2.0 Listener Properties - 56 bytes */
#pragma pack(push, 4)
typedef struct eax20_listener_properties_s {
    int32_t  lRoom;
    int32_t  lRoomHF;
    float    flRoomRolloffFactor;
    float    flDecayTime;
    float    flDecayHFRatio;
    int32_t  lReflections;
    float    flReflectionsDelay;
    int32_t  lReverb;
    float    flReverbDelay;
    uint32_t dwEnvironment;
    float    flEnvironmentSize;
    float    flEnvironmentDiffusion;
    float    flAirAbsorptionHF;
    uint32_t dwFlags;
} eax20_listener_properties_t;
#pragma pack(pop)

/* EAX 2.0 Buffer Properties - 52 bytes */
#pragma pack(push, 4)
typedef struct eax20_buffer_properties_s {
    int32_t  lDirect;
    int32_t  lDirectHF;
    int32_t  lRoom;
    int32_t  lRoomHF;
    float    flRoomRolloffFactor;
    int32_t  lObstruction;
    float    flObstructionLFRatio;
    int32_t  lOcclusion;
    float    flOcclusionLFRatio;
    float    flOcclusionRoomRatio;
    int32_t  lOutsideVolumeHF;
    float    flAirAbsorptionFactor;
    uint32_t dwFlags;
} eax20_buffer_properties_t;
#pragma pack(pop)

/* Compile-time binary layout validation (per Fix R-003) */
#ifdef __cplusplus
static_assert(sizeof(eax_guid_t) == 16, "eax_guid_t size mismatch");
static_assert(sizeof(eax_waveformat_ex_t) == 18, "eax_waveformat_ex_t size mismatch (must be 2-byte packed)");
static_assert(sizeof(eax_dscaps_t) == 96, "eax_dscaps_t size mismatch");
static_assert(sizeof(eax_dsbcaps_t) == 20, "eax_dsbcaps_t size mismatch");
static_assert(sizeof(eax_vector_t) == 12, "eax_vector_t size mismatch");
static_assert(sizeof(eax_ds3dlistener_t) == 64, "eax_ds3dlistener_t size mismatch");
static_assert(sizeof(eax_ds3dbuffer_t) == 64, "eax_ds3dbuffer_t size mismatch");
static_assert(sizeof(eax10_reverb_properties_t) == 16, "eax10_reverb_properties_t size mismatch");
static_assert(sizeof(eax10_buffer_properties_t) == 4, "eax10_buffer_properties_t size mismatch");
static_assert(sizeof(eax20_listener_properties_t) == 56, "eax20_listener_properties_t size mismatch");
static_assert(sizeof(eax20_buffer_properties_t) == 52, "eax20_buffer_properties_t size mismatch");
#endif

/* 3D Apply modes */
#define EAX_DS3D_IMMEDIATE          0x00000000
#define EAX_DS3D_DEFERRED           0x00000001

/* 3D Buffer modes */
#define EAX_DS3DMODE_NORMAL         0x00000000
#define EAX_DS3DMODE_HEADRELATIVE   0x00000001
#define EAX_DS3DMODE_DISABLE        0x00000002

/* Buffer flags (DSBCAPS) */
#define EAX_DSBCAPS_PRIMARYBUFFER        0x00000001
#define EAX_DSBCAPS_STATIC               0x00000002
#define EAX_DSBCAPS_LOCHARDWARE          0x00000004
#define EAX_DSBCAPS_LOCSOFTWARE          0x00000008
#define EAX_DSBCAPS_CTRL3D               0x00000010
#define EAX_DSBCAPS_CTRLFREQUENCY        0x00000020
#define EAX_DSBCAPS_CTRLPAN              0x00000040
#define EAX_DSBCAPS_CTRLVOLUME           0x00000080
#define EAX_DSBCAPS_CTRLPOSITIONNOTIFY   0x00000100
#define EAX_DSBCAPS_CTRLFX               0x00000200

/* Play flags */
#define EAX_DSBPLAY_LOOPING              0x00000001

/* Buffer status flags */
#define EAX_DSBSTATUS_PLAYING            0x00000001
#define EAX_DSBSTATUS_BUFFERLOST         0x00000002
#define EAX_DSBSTATUS_LOOPING            0x00000004
#define EAX_DSBSTATUS_LOCHARDWARE        0x00000008
#define EAX_DSBSTATUS_LOCSOFTWARE        0x00000010

/* Property Support flags */
#define EAX_KSPROPERTY_SUPPORT_GET       0x00000001
#define EAX_KSPROPERTY_SUPPORT_SET       0x00000002

/* Speaker configurations */
#define EAX_SPEAKER_CONFIG_HEADPHONE     1
#define EAX_SPEAKER_CONFIG_STEREO        2
#define EAX_SPEAKER_CONFIG_QUAD          3
#define EAX_SPEAKER_CONFIG_SURROUND      4
#define EAX_SPEAKER_CONFIG_5POINT1       5
#define EAX_SPEAKER_CONFIG_7POINT1       6

/* EAX Presets */
#define EAX_ENVIRONMENT_GENERIC          0
#define EAX_ENVIRONMENT_PADDEDCELL       1
#define EAX_ENVIRONMENT_ROOM             2
#define EAX_ENVIRONMENT_BATHROOM         3
#define EAX_ENVIRONMENT_LIVINGROOM       4
#define EAX_ENVIRONMENT_STONEROOM        5
#define EAX_ENVIRONMENT_AUDITORIUM       6
#define EAX_ENVIRONMENT_CONCERTHALL      7
#define EAX_ENVIRONMENT_CAVE             8
#define EAX_ENVIRONMENT_ARENA            9
#define EAX_ENVIRONMENT_HANGAR           10
#define EAX_ENVIRONMENT_CARPETEDHALLWAY  11
#define EAX_ENVIRONMENT_HALLWAY          12
#define EAX_ENVIRONMENT_STONECORRIDOR    13
#define EAX_ENVIRONMENT_ALLEY            14
#define EAX_ENVIRONMENT_FOREST           15
#define EAX_ENVIRONMENT_CITY             16
#define EAX_ENVIRONMENT_MOUNTAINS        17
#define EAX_ENVIRONMENT_QUARRY           18
#define EAX_ENVIRONMENT_PLAIN            19
#define EAX_ENVIRONMENT_PARKINGLOT       20
#define EAX_ENVIRONMENT_SEWERPIPE        21
#define EAX_ENVIRONMENT_UNDERWATER       22
#define EAX_ENVIRONMENT_DRUGGED          23
#define EAX_ENVIRONMENT_DIZZY            24
#define EAX_ENVIRONMENT_PSYCHOTIC        25
#define EAX_ENVIRONMENT_COUNT            26

/* ----------------- Core Engine C ABI Export Functions ----------------- */

/* Context Lifecycle */
EAX_API eax_result_t eax_context_create(eax_context_h* out_context);
EAX_API eax_result_t eax_context_create_offline(eax_context_h* out_context, uint32_t sample_rate);
EAX_API eax_result_t eax_context_destroy(eax_context_h context);
EAX_API eax_result_t eax_context_get_caps(eax_context_h context, eax_dscaps_t* out_caps);
EAX_API eax_result_t eax_context_get_speaker_config(eax_context_h context, uint32_t* out_speaker_config);
EAX_API eax_result_t eax_context_set_speaker_config(eax_context_h context, uint32_t speaker_config);

/* Listener Configuration & DirectSound3D */
EAX_API eax_result_t eax_listener_get_all_parameters(eax_context_h context, eax_ds3dlistener_t* out_params);
EAX_API eax_result_t eax_listener_set_all_parameters(eax_context_h context, const eax_ds3dlistener_t* params, uint32_t apply_mode);
EAX_API eax_result_t eax_listener_get_position(eax_context_h context, eax_vector_t* out_pos);
EAX_API eax_result_t eax_listener_set_position(eax_context_h context, float x, float y, float z, uint32_t apply_mode);
EAX_API eax_result_t eax_listener_get_velocity(eax_context_h context, eax_vector_t* out_vel);
EAX_API eax_result_t eax_listener_set_velocity(eax_context_h context, float x, float y, float z, uint32_t apply_mode);
EAX_API eax_result_t eax_listener_get_orientation(eax_context_h context, eax_vector_t* out_front, eax_vector_t* out_top);
EAX_API eax_result_t eax_listener_set_orientation(eax_context_h context, float fx, float fy, float fz, float tx, float ty, float tz, uint32_t apply_mode);
EAX_API eax_result_t eax_listener_get_distance_factor(eax_context_h context, float* out_factor);
EAX_API eax_result_t eax_listener_set_distance_factor(eax_context_h context, float factor, uint32_t apply_mode);
EAX_API eax_result_t eax_listener_get_rolloff_factor(eax_context_h context, float* out_factor);
EAX_API eax_result_t eax_listener_set_rolloff_factor(eax_context_h context, float factor, uint32_t apply_mode);
EAX_API eax_result_t eax_listener_get_doppler_factor(eax_context_h context, float* out_factor);
EAX_API eax_result_t eax_listener_set_doppler_factor(eax_context_h context, float factor, uint32_t apply_mode);
EAX_API eax_result_t eax_listener_commit_deferred(eax_context_h context);

/* Buffer Lifecycle & Voice Management */
EAX_API eax_result_t eax_buffer_create(eax_context_h context, const eax_dsbcaps_t* caps, const eax_waveformat_ex_t* format, eax_buffer_h* out_buffer);
EAX_API eax_result_t eax_buffer_destroy(eax_buffer_h buffer);
EAX_API eax_result_t eax_buffer_duplicate(eax_context_h context, eax_buffer_h source_buffer, eax_buffer_h* out_buffer);
EAX_API eax_result_t eax_buffer_get_caps(eax_buffer_h buffer, eax_dsbcaps_t* out_caps);
EAX_API eax_result_t eax_buffer_get_format(eax_buffer_h buffer, eax_waveformat_ex_t* out_format);

/* Buffer Playback & Streaming (Hot Paths: zero malloc, lock-free) */
EAX_API eax_result_t eax_buffer_lock(eax_buffer_h buffer, uint32_t offset, uint32_t bytes, void** ptr1, uint32_t* bytes1, void** ptr2, uint32_t* bytes2, uint32_t flags);
EAX_API eax_result_t eax_buffer_unlock(eax_buffer_h buffer, void* ptr1, uint32_t bytes1, void* ptr2, uint32_t bytes2);
EAX_API eax_result_t eax_buffer_play(eax_buffer_h buffer, uint32_t priority, uint32_t flags);
EAX_API eax_result_t eax_buffer_stop(eax_buffer_h buffer);
EAX_API eax_result_t eax_buffer_get_status(eax_buffer_h buffer, uint32_t* out_status);
EAX_API eax_result_t eax_buffer_get_current_position(eax_buffer_h buffer, uint32_t* out_play_pos, uint32_t* out_write_pos);
EAX_API eax_result_t eax_buffer_set_current_position(eax_buffer_h buffer, uint32_t new_pos);

/* Buffer Properties (Volume, Pan via AL_PAN_SOFT, Frequency) */
EAX_API eax_result_t eax_buffer_get_volume(eax_buffer_h buffer, int32_t* out_volume_mb);
EAX_API eax_result_t eax_buffer_set_volume(eax_buffer_h buffer, int32_t volume_mb);
EAX_API eax_result_t eax_buffer_get_pan(eax_buffer_h buffer, int32_t* out_pan_mb);
EAX_API eax_result_t eax_buffer_set_pan(eax_buffer_h buffer, int32_t pan_mb);
EAX_API eax_result_t eax_buffer_get_frequency(eax_buffer_h buffer, uint32_t* out_freq_hz);
EAX_API eax_result_t eax_buffer_set_frequency(eax_buffer_h buffer, uint32_t freq_hz);

/* DirectSound3D Buffer Spatialization */
EAX_API eax_result_t eax_buffer_get_3d_all_parameters(eax_buffer_h buffer, eax_ds3dbuffer_t* out_params);
EAX_API eax_result_t eax_buffer_set_3d_all_parameters(eax_buffer_h buffer, const eax_ds3dbuffer_t* params, uint32_t apply_mode);
EAX_API eax_result_t eax_buffer_get_3d_position(eax_buffer_h buffer, eax_vector_t* out_pos);
EAX_API eax_result_t eax_buffer_set_3d_position(eax_buffer_h buffer, float x, float y, float z, uint32_t apply_mode);
EAX_API eax_result_t eax_buffer_get_3d_velocity(eax_buffer_h buffer, eax_vector_t* out_vel);
EAX_API eax_result_t eax_buffer_set_3d_velocity(eax_buffer_h buffer, float x, float y, float z, uint32_t apply_mode);
EAX_API eax_result_t eax_buffer_get_3d_cone_angles(eax_buffer_h buffer, uint32_t* out_inside, uint32_t* out_outside);
EAX_API eax_result_t eax_buffer_set_3d_cone_angles(eax_buffer_h buffer, uint32_t inside, uint32_t outside, uint32_t apply_mode);
EAX_API eax_result_t eax_buffer_get_3d_cone_orientation(eax_buffer_h buffer, eax_vector_t* out_orientation);
EAX_API eax_result_t eax_buffer_set_3d_cone_orientation(eax_buffer_h buffer, float x, float y, float z, uint32_t apply_mode);
EAX_API eax_result_t eax_buffer_get_3d_cone_outside_volume(eax_buffer_h buffer, int32_t* out_volume_mb);
EAX_API eax_result_t eax_buffer_set_3d_cone_outside_volume(eax_buffer_h buffer, int32_t volume_mb, uint32_t apply_mode);
EAX_API eax_result_t eax_buffer_get_3d_min_distance(eax_buffer_h buffer, float* out_dist);
EAX_API eax_result_t eax_buffer_set_3d_min_distance(eax_buffer_h buffer, float dist, uint32_t apply_mode);
EAX_API eax_result_t eax_buffer_get_3d_max_distance(eax_buffer_h buffer, float* out_dist);
EAX_API eax_result_t eax_buffer_set_3d_max_distance(eax_buffer_h buffer, float dist, uint32_t apply_mode);
EAX_API eax_result_t eax_buffer_get_3d_mode(eax_buffer_h buffer, uint32_t* out_mode);
EAX_API eax_result_t eax_buffer_set_3d_mode(eax_buffer_h buffer, uint32_t mode, uint32_t apply_mode);

/* EAX Property Sets (IKsPropertySet) */
EAX_API eax_result_t eax_prop_query_support(eax_context_h context, eax_buffer_h buffer, const eax_guid_t* prop_set, uint32_t prop_id, uint32_t* out_support);
EAX_API eax_result_t eax_prop_get(eax_context_h context, eax_buffer_h buffer, const eax_guid_t* prop_set, uint32_t prop_id, void* instance_data, uint32_t instance_len, void* prop_data, uint32_t prop_len, uint32_t* bytes_returned);
EAX_API eax_result_t eax_prop_set(eax_context_h context, eax_buffer_h buffer, const eax_guid_t* prop_set, uint32_t prop_id, void* instance_data, uint32_t instance_len, const void* prop_data, uint32_t prop_len);

/* Offline audio rendering test interface (renders mixed frames to memory buffer) */
EAX_API eax_result_t eax_context_render_offline(eax_context_h context, float* out_stereo_pcm, size_t frame_count, uint32_t sample_rate);

#ifdef __cplusplus
}
#endif

#endif /* EAX_NATIVE_ABI_H */
