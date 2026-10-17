#include "openal_bridge.h"

namespace eax {

static OpenALExtensionFunctions g_ext;

OpenALExtensionFunctions& get_al_ext() noexcept {
    return g_ext;
}

void OpenALExtensionFunctions::init(ALCdevice* device) noexcept {
    if (!device) return;

    // Check EFX
    if (alcIsExtensionPresent(device, "ALC_EXT_EFX")) {
        has_efx = true;
        alGenEffects = reinterpret_cast<LPALGENEFFECTS>(alGetProcAddress("alGenEffects"));
        alDeleteEffects = reinterpret_cast<LPALDELETEEFFECTS>(alGetProcAddress("alDeleteEffects"));
        alIsEffect = reinterpret_cast<LPALISEFFECT>(alGetProcAddress("alIsEffect"));
        alEffecti = reinterpret_cast<LPALEFFECTI>(alGetProcAddress("alEffecti"));
        alEffectiv = reinterpret_cast<LPALEFFECTIV>(alGetProcAddress("alEffectiv"));
        alEffectf = reinterpret_cast<LPALEFFECTF>(alGetProcAddress("alEffectf"));
        alEffectfv = reinterpret_cast<LPALEFFECTFV>(alGetProcAddress("alEffectfv"));

        alGenAuxiliaryEffectSlots = reinterpret_cast<LPALGENAUXILIARYEFFECTSLOTS>(alGetProcAddress("alGenAuxiliaryEffectSlots"));
        alDeleteAuxiliaryEffectSlots = reinterpret_cast<LPALDELETEAUXILIARYEFFECTSLOTS>(alGetProcAddress("alDeleteAuxiliaryEffectSlots"));
        alIsAuxiliaryEffectSlot = reinterpret_cast<LPALISAUXILIARYEFFECTSLOT>(alGetProcAddress("alIsAuxiliaryEffectSlot"));
        alAuxiliaryEffectSloti = reinterpret_cast<LPALAUXILIARYEFFECTSLOTI>(alGetProcAddress("alAuxiliaryEffectSloti"));
        alAuxiliaryEffectSlotiv = reinterpret_cast<LPALAUXILIARYEFFECTSLOTIV>(alGetProcAddress("alAuxiliaryEffectSlotiv"));
        alAuxiliaryEffectSlotf = reinterpret_cast<LPALAUXILIARYEFFECTSLOTF>(alGetProcAddress("alAuxiliaryEffectSlotf"));
        alAuxiliaryEffectSlotfv = reinterpret_cast<LPALAUXILIARYEFFECTSLOTFV>(alGetProcAddress("alAuxiliaryEffectSlotfv"));

        alGenFilters = reinterpret_cast<LPALGENFILTERS>(alGetProcAddress("alGenFilters"));
        alDeleteFilters = reinterpret_cast<LPALDELETEFILTERS>(alGetProcAddress("alDeleteFilters"));
        alIsFilter = reinterpret_cast<LPALISFILTER>(alGetProcAddress("alIsFilter"));
        alFilteri = reinterpret_cast<LPALFILTERI>(alGetProcAddress("alFilteri"));
        alFilteriv = reinterpret_cast<LPALFILTERIV>(alGetProcAddress("alFilteriv"));
        alFilterf = reinterpret_cast<LPALFILTERF>(alGetProcAddress("alFilterf"));
        alFilterfv = reinterpret_cast<LPALFILTERFV>(alGetProcAddress("alFilterfv"));
    }

    // Check loopback
    if (alcIsExtensionPresent(nullptr, "ALC_SOFT_loopback")) {
        has_loopback = true;
        alcLoopbackOpenDeviceSOFT = reinterpret_cast<LPALCLOOPBACKOPENDEVICESOFT>(alcGetProcAddress(nullptr, "alcLoopbackOpenDeviceSOFT"));
        alcRenderSamplesSOFT = reinterpret_cast<LPALCRENDERSAMPLESSOFT>(alcGetProcAddress(nullptr, "alcRenderSamplesSOFT"));
    }

    // Check AL_EXT_EAX
    if (alIsExtensionPresent("AL_EXT_EAX")) {
        has_eax_ext = true;
        EAXSetDirect = reinterpret_cast<LPEAXSETDIRECT>(alGetProcAddress("EAXSetDirect"));
        EAXGetDirect = reinterpret_cast<LPEAXGETDIRECT>(alGetProcAddress("EAXGetDirect"));
    }

    // Check source panning
    if (alIsExtensionPresent("AL_SOFT_source_panning")) {
        has_source_panning = true;
    }
}

} // namespace eax
