#ifndef EAX_PIPEWIRE_H
#define EAX_PIPEWIRE_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "eax_native_abi.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Opaque filter handle */
typedef struct eax_pipewire_filter_s eax_pipewire_filter_t;

/* Status codes */
#define EAX_PW_OK                 0
#define EAX_PW_ERR_GENERIC       -1
#define EAX_PW_ERR_INVALID_PARAM -2
#define EAX_PW_ERR_NOT_CONNECTED -3

/**
 * Create a PipeWire DSP real-time node filter instance wrapping an eax_context_h.
 * Note: Only stereo (channels == 2) is supported. Rejects channels != 2 with nullptr.
 *
 * @param context       eax-native audio context handle
 * @param name          Media / node name for PipeWire graph (e.g. "eax-filter")
 * @param sample_rate   Sample rate in Hz (e.g. 44100 or 48000)
 * @param channels      Number of output channels (must be exactly 2 for stereo)
 * @return              Pointer to filter instance, or nullptr on failure or invalid parameter
 */
EAX_API eax_pipewire_filter_t* eax_pipewire_filter_create(
    eax_context_h context,
    const char* name,
    uint32_t sample_rate,
    uint32_t channels
);

/**
 * Start the real-time PipeWire processing thread / loop.
 */
EAX_API int eax_pipewire_filter_start(eax_pipewire_filter_t* filter);

/**
 * Stop the real-time PipeWire processing thread / loop.
 */
EAX_API int eax_pipewire_filter_stop(eax_pipewire_filter_t* filter);

/**
 * Check if connected to an active PipeWire daemon graph.
 */
EAX_API bool eax_pipewire_filter_is_connected(const eax_pipewire_filter_t* filter);

/**
 * Process a single audio frame/block in real-time.
 * RT-safe: guaranteed zero malloc, zero syscalls, lock-free.
 *
 * @param filter        Filter instance
 * @param in_pcm        Optional input interleaved audio (can be null for generator)
 * @param out_pcm       Output stereo interleaved float audio buffer
 * @param n_frames      Number of frames to render
 * @return              EAX_PW_OK on success
 */
EAX_API int eax_pipewire_filter_process_block(
    eax_pipewire_filter_t* filter,
    const float* in_pcm,
    float* out_pcm,
    uint32_t n_frames
);

/**
 * Destroy the PipeWire filter instance and release resources.
 */
EAX_API void eax_pipewire_filter_destroy(eax_pipewire_filter_t* filter);

/**
 * Note on libspa-eax-filter.so:
 * The accompanying SPA module is an experimental architectural scaffold for factory
 * enumeration ("audio.filter.eax"). Full in-graph port negotiation (SPA_PARAM_PortConfig)
 * is scheduled for Phase 4. The primary production PipeWire interface is eax_pipewire_filter_*.
 */

#ifdef __cplusplus
}
#endif

#endif /* EAX_PIPEWIRE_H */
