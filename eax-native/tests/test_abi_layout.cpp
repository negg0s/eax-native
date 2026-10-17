#include "catch_amalgamated.hpp"
#include "eax_native_abi.h"
#include <cstddef>

TEST_CASE("ABI Boundary Layout: sizeof and offsetof validation", "[abi]") {
    SECTION("eax_guid_t") {
        REQUIRE(sizeof(eax_guid_t) == 16);
        REQUIRE(offsetof(eax_guid_t, data1) == 0);
        REQUIRE(offsetof(eax_guid_t, data2) == 4);
        REQUIRE(offsetof(eax_guid_t, data3) == 6);
        REQUIRE(offsetof(eax_guid_t, data4) == 8);
    }

    SECTION("eax_waveformat_ex_t (2-byte packed)") {
        REQUIRE(sizeof(eax_waveformat_ex_t) == 18);
        REQUIRE(offsetof(eax_waveformat_ex_t, wFormatTag) == 0);
        REQUIRE(offsetof(eax_waveformat_ex_t, nChannels) == 2);
        REQUIRE(offsetof(eax_waveformat_ex_t, nSamplesPerSec) == 4);
        REQUIRE(offsetof(eax_waveformat_ex_t, nAvgBytesPerSec) == 8);
        REQUIRE(offsetof(eax_waveformat_ex_t, nBlockAlign) == 12);
        REQUIRE(offsetof(eax_waveformat_ex_t, wBitsPerSample) == 14);
        REQUIRE(offsetof(eax_waveformat_ex_t, cbSize) == 16);
    }

    SECTION("eax_dscaps_t") {
        REQUIRE(sizeof(eax_dscaps_t) == 96);
        REQUIRE(offsetof(eax_dscaps_t, dwSize) == 0);
        REQUIRE(offsetof(eax_dscaps_t, dwFlags) == 4);
        REQUIRE(offsetof(eax_dscaps_t, dwMinSecondarySampleRate) == 8);
        REQUIRE(offsetof(eax_dscaps_t, dwMaxSecondarySampleRate) == 12);
        REQUIRE(offsetof(eax_dscaps_t, dwPrimaryBuffers) == 16);
        REQUIRE(offsetof(eax_dscaps_t, dwMaxHwMixingAllBuffers) == 20);
        REQUIRE(offsetof(eax_dscaps_t, dwTotalHwMemBytes) == 68);
        REQUIRE(offsetof(eax_dscaps_t, dwReserved2) == 92);
    }

    SECTION("eax_dsbcaps_t") {
        REQUIRE(sizeof(eax_dsbcaps_t) == 20);
        REQUIRE(offsetof(eax_dsbcaps_t, dwSize) == 0);
        REQUIRE(offsetof(eax_dsbcaps_t, dwFlags) == 4);
        REQUIRE(offsetof(eax_dsbcaps_t, dwBufferBytes) == 8);
        REQUIRE(offsetof(eax_dsbcaps_t, dwUnlockTransferRate) == 12);
        REQUIRE(offsetof(eax_dsbcaps_t, dwPlayCpuOverhead) == 16);
    }

    SECTION("eax_vector_t") {
        REQUIRE(sizeof(eax_vector_t) == 12);
        REQUIRE(offsetof(eax_vector_t, x) == 0);
        REQUIRE(offsetof(eax_vector_t, y) == 4);
        REQUIRE(offsetof(eax_vector_t, z) == 8);
    }

    SECTION("eax_ds3dlistener_t") {
        REQUIRE(sizeof(eax_ds3dlistener_t) == 64);
        REQUIRE(offsetof(eax_ds3dlistener_t, dwSize) == 0);
        REQUIRE(offsetof(eax_ds3dlistener_t, vPosition) == 4);
        REQUIRE(offsetof(eax_ds3dlistener_t, vVelocity) == 16);
        REQUIRE(offsetof(eax_ds3dlistener_t, vOrientFront) == 28);
        REQUIRE(offsetof(eax_ds3dlistener_t, vOrientTop) == 40);
        REQUIRE(offsetof(eax_ds3dlistener_t, flDistanceFactor) == 52);
        REQUIRE(offsetof(eax_ds3dlistener_t, flRolloffFactor) == 56);
        REQUIRE(offsetof(eax_ds3dlistener_t, flDopplerFactor) == 60);
    }

    SECTION("eax_ds3dbuffer_t") {
        REQUIRE(sizeof(eax_ds3dbuffer_t) == 64);
        REQUIRE(offsetof(eax_ds3dbuffer_t, dwSize) == 0);
        REQUIRE(offsetof(eax_ds3dbuffer_t, vPosition) == 4);
        REQUIRE(offsetof(eax_ds3dbuffer_t, vVelocity) == 16);
        REQUIRE(offsetof(eax_ds3dbuffer_t, dwInsideConeAngle) == 28);
        REQUIRE(offsetof(eax_ds3dbuffer_t, dwOutsideConeAngle) == 32);
        REQUIRE(offsetof(eax_ds3dbuffer_t, vConeOrientation) == 36);
        REQUIRE(offsetof(eax_ds3dbuffer_t, lConeOutsideVolume) == 48);
        REQUIRE(offsetof(eax_ds3dbuffer_t, flMinDistance) == 52);
        REQUIRE(offsetof(eax_ds3dbuffer_t, flMaxDistance) == 56);
        REQUIRE(offsetof(eax_ds3dbuffer_t, dwMode) == 60);
    }

    SECTION("eax10_reverb_properties_t") {
        REQUIRE(sizeof(eax10_reverb_properties_t) == 16);
        REQUIRE(offsetof(eax10_reverb_properties_t, dwEnvironment) == 0);
        REQUIRE(offsetof(eax10_reverb_properties_t, fVolume) == 4);
        REQUIRE(offsetof(eax10_reverb_properties_t, fDecayTime_sec) == 8);
        REQUIRE(offsetof(eax10_reverb_properties_t, fDamping) == 12);
    }

    SECTION("eax10_buffer_properties_t") {
        REQUIRE(sizeof(eax10_buffer_properties_t) == 4);
        REQUIRE(offsetof(eax10_buffer_properties_t, fMix) == 0);
    }

    SECTION("eax20_listener_properties_t") {
        REQUIRE(sizeof(eax20_listener_properties_t) == 56);
        REQUIRE(offsetof(eax20_listener_properties_t, lRoom) == 0);
        REQUIRE(offsetof(eax20_listener_properties_t, lRoomHF) == 4);
        REQUIRE(offsetof(eax20_listener_properties_t, flRoomRolloffFactor) == 8);
        REQUIRE(offsetof(eax20_listener_properties_t, flDecayTime) == 12);
        REQUIRE(offsetof(eax20_listener_properties_t, flDecayHFRatio) == 16);
        REQUIRE(offsetof(eax20_listener_properties_t, lReflections) == 20);
        REQUIRE(offsetof(eax20_listener_properties_t, flReflectionsDelay) == 24);
        REQUIRE(offsetof(eax20_listener_properties_t, lReverb) == 28);
        REQUIRE(offsetof(eax20_listener_properties_t, flReverbDelay) == 32);
        REQUIRE(offsetof(eax20_listener_properties_t, dwEnvironment) == 36);
        REQUIRE(offsetof(eax20_listener_properties_t, flEnvironmentSize) == 40);
        REQUIRE(offsetof(eax20_listener_properties_t, flEnvironmentDiffusion) == 44);
        REQUIRE(offsetof(eax20_listener_properties_t, flAirAbsorptionHF) == 48);
        REQUIRE(offsetof(eax20_listener_properties_t, dwFlags) == 52);
    }

    SECTION("eax20_buffer_properties_t") {
        REQUIRE(sizeof(eax20_buffer_properties_t) == 52);
        REQUIRE(offsetof(eax20_buffer_properties_t, lDirect) == 0);
        REQUIRE(offsetof(eax20_buffer_properties_t, lDirectHF) == 4);
        REQUIRE(offsetof(eax20_buffer_properties_t, lRoom) == 8);
        REQUIRE(offsetof(eax20_buffer_properties_t, lRoomHF) == 12);
        REQUIRE(offsetof(eax20_buffer_properties_t, flRoomRolloffFactor) == 16);
        REQUIRE(offsetof(eax20_buffer_properties_t, lObstruction) == 20);
        REQUIRE(offsetof(eax20_buffer_properties_t, flObstructionLFRatio) == 24);
        REQUIRE(offsetof(eax20_buffer_properties_t, lOcclusion) == 28);
        REQUIRE(offsetof(eax20_buffer_properties_t, flOcclusionLFRatio) == 32);
        REQUIRE(offsetof(eax20_buffer_properties_t, flOcclusionRoomRatio) == 36);
        REQUIRE(offsetof(eax20_buffer_properties_t, lOutsideVolumeHF) == 40);
        REQUIRE(offsetof(eax20_buffer_properties_t, flAirAbsorptionFactor) == 44);
        REQUIRE(offsetof(eax20_buffer_properties_t, dwFlags) == 48);
    }
}
