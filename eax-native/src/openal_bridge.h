#ifndef EAX_NATIVE_OPENAL_BRIDGE_H
#define EAX_NATIVE_OPENAL_BRIDGE_H

#include <cstdint>
#include <cstddef>
#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alext.h>
#include <AL/efx.h>

namespace eax {

struct OpenALExtensionFunctions {
    // EFX Functions
    LPALGENEFFECTS alGenEffects{nullptr};
    LPALDELETEEFFECTS alDeleteEffects{nullptr};
    LPALISEFFECT alIsEffect{nullptr};
    LPALEFFECTI alEffecti{nullptr};
    LPALEFFECTIV alEffectiv{nullptr};
    LPALEFFECTF alEffectf{nullptr};
    LPALEFFECTFV alEffectfv{nullptr};

    LPALGENAUXILIARYEFFECTSLOTS alGenAuxiliaryEffectSlots{nullptr};
    LPALDELETEAUXILIARYEFFECTSLOTS alDeleteAuxiliaryEffectSlots{nullptr};
    LPALISAUXILIARYEFFECTSLOT alIsAuxiliaryEffectSlot{nullptr};
    LPALAUXILIARYEFFECTSLOTI alAuxiliaryEffectSloti{nullptr};
    LPALAUXILIARYEFFECTSLOTIV alAuxiliaryEffectSlotiv{nullptr};
    LPALAUXILIARYEFFECTSLOTF alAuxiliaryEffectSlotf{nullptr};
    LPALAUXILIARYEFFECTSLOTFV alAuxiliaryEffectSlotfv{nullptr};

    LPALGENFILTERS alGenFilters{nullptr};
    LPALDELETEFILTERS alDeleteFilters{nullptr};
    LPALISFILTER alIsFilter{nullptr};
    LPALFILTERI alFilteri{nullptr};
    LPALFILTERIV alFilteriv{nullptr};
    LPALFILTERF alFilterf{nullptr};
    LPALFILTERFV alFilterfv{nullptr};

    // Loopback Functions (ALC_SOFT_loopback)
    LPALCLOOPBACKOPENDEVICESOFT alcLoopbackOpenDeviceSOFT{nullptr};
    LPALCRENDERSAMPLESSOFT alcRenderSamplesSOFT{nullptr};

    // AL_EXT_EAX Functions
    LPEAXSETDIRECT EAXSetDirect{nullptr};
    LPEAXGETDIRECT EAXGetDirect{nullptr};

    bool has_efx{false};
    bool has_loopback{false};
    bool has_eax_ext{false};
    bool has_source_panning{false};

    void init(ALCdevice* device) noexcept;
};

// Global or context-local extension function table
OpenALExtensionFunctions& get_al_ext() noexcept;

} // namespace eax

#endif // EAX_NATIVE_OPENAL_BRIDGE_H
