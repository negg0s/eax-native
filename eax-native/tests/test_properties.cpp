#include "catch_amalgamated.hpp"
#include "eax_native_abi.h"
#include <cstring>

TEST_CASE("EAX Property Sets: IKsPropertySet Query, Get, Set, and Deferred Commit", "[properties]") {
    eax_context_h ctx = nullptr;
    REQUIRE(eax_context_create(&ctx) == EAX_OK);

    eax_dsbcaps_t caps{};
    caps.dwSize = sizeof(eax_dsbcaps_t);
    caps.dwFlags = EAX_DSBCAPS_CTRL3D | EAX_DSBCAPS_CTRLVOLUME | EAX_DSBCAPS_CTRLPAN | EAX_DSBCAPS_CTRLFREQUENCY;
    caps.dwBufferBytes = 4096;

    eax_waveformat_ex_t format{};
    format.wFormatTag = 1;
    format.nChannels = 1;
    format.nSamplesPerSec = 44100;
    format.wBitsPerSample = 16;
    format.nBlockAlign = 2;
    format.nAvgBytesPerSec = 88200;
    format.cbSize = 0;

    eax_buffer_h buf = nullptr;
    REQUIRE(eax_buffer_create(ctx, &caps, &format, &buf) == EAX_OK);

    const eax_guid_t guid_eax10_listener = {
        0x4A4E6FC1, 0xC341, 0x11D1, {0xB7, 0x3A, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00}
    };
    const eax_guid_t guid_eax10_buffer = {
        0x4A4E6FC0, 0xC341, 0x11D1, {0xB7, 0x3A, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00}
    };
    const eax_guid_t guid_eax20_listener = {
        0x0306A6A8, 0xB224, 0x11D2, {0x99, 0xE5, 0x00, 0x00, 0xE8, 0xD8, 0xC7, 0x22}
    };
    const eax_guid_t guid_eax20_buffer = {
        0x0306A6A7, 0xB224, 0x11D2, {0x99, 0xE5, 0x00, 0x00, 0xE8, 0xD8, 0xC7, 0x22}
    };
    const eax_guid_t guid_eax40_context = {
        0x1D4870AD, 0x0DEF, 0x43C0, {0xA4, 0x0C, 0x52, 0x36, 0x32, 0x29, 0x63, 0x42}
    };
    const eax_guid_t guid_unknown = {
        0x12345678, 0x1234, 0x1234, {0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0}
    };

    SECTION("QuerySupport") {
        uint32_t support = 0;
        REQUIRE(eax_prop_query_support(ctx, buf, &guid_eax10_listener, 0, &support) == EAX_OK);
        REQUIRE((support & (EAX_KSPROPERTY_SUPPORT_GET | EAX_KSPROPERTY_SUPPORT_SET)) != 0);

        REQUIRE(eax_prop_query_support(ctx, buf, &guid_eax20_listener, 0, &support) == EAX_OK);
        REQUIRE((support & (EAX_KSPROPERTY_SUPPORT_GET | EAX_KSPROPERTY_SUPPORT_SET)) != 0);

        REQUIRE(eax_prop_query_support(ctx, buf, &guid_eax20_buffer, 0, &support) == EAX_OK);
        REQUIRE((support & (EAX_KSPROPERTY_SUPPORT_GET | EAX_KSPROPERTY_SUPPORT_SET)) != 0);

        REQUIRE(eax_prop_query_support(ctx, buf, &guid_eax40_context, 0, &support) == EAX_ERR_PROP_UNSUPPORTED);

        REQUIRE(eax_prop_query_support(ctx, buf, &guid_unknown, 0, &support) == EAX_ERR_PROP_UNSUPPORTED);
    }

    SECTION("EAX 2.0 Listener Get & Set") {
        eax20_listener_properties_t set_props{};
        set_props.lRoom = -1500;
        set_props.lRoomHF = -500;
        set_props.flRoomRolloffFactor = 1.5f;
        set_props.flDecayTime = 3.5f;
        set_props.flDecayHFRatio = 0.8f;
        set_props.lReflections = -800;
        set_props.flReflectionsDelay = 0.02f;
        set_props.lReverb = -400;
        set_props.flReverbDelay = 0.03f;
        set_props.dwEnvironment = EAX_ENVIRONMENT_HANGAR;
        set_props.flEnvironmentSize = 25.0f;
        set_props.flEnvironmentDiffusion = 0.9f;
        set_props.flAirAbsorptionHF = -2.0f;
        set_props.dwFlags = 0x1F;

        REQUIRE(eax_prop_set(ctx, nullptr, &guid_eax20_listener, 1, nullptr, 0, &set_props, sizeof(set_props)) == EAX_OK);

        eax20_listener_properties_t get_props{};
        uint32_t returned = 0;
        REQUIRE(eax_prop_get(ctx, nullptr, &guid_eax20_listener, 1, nullptr, 0, &get_props, sizeof(get_props), &returned) == EAX_OK);
        REQUIRE(returned == sizeof(eax20_listener_properties_t));
        REQUIRE(get_props.lRoom == -1500);
        REQUIRE(get_props.flDecayTime == Catch::Approx(3.5f));
        REQUIRE(get_props.dwEnvironment == EAX_ENVIRONMENT_HANGAR);
    }

    SECTION("EAX 2.0 Buffer Get & Set") {
        eax20_buffer_properties_t set_buf_props{};
        set_buf_props.lDirect = -200;
        set_buf_props.lDirectHF = -100;
        set_buf_props.lObstruction = -1200;
        set_buf_props.lOcclusion = -1500;
        set_buf_props.dwFlags = 0x07;

        REQUIRE(eax_prop_set(ctx, buf, &guid_eax20_buffer, 1, nullptr, 0, &set_buf_props, sizeof(set_buf_props)) == EAX_OK);

        eax20_buffer_properties_t get_buf_props{};
        uint32_t returned = 0;
        REQUIRE(eax_prop_get(ctx, buf, &guid_eax20_buffer, 1, nullptr, 0, &get_buf_props, sizeof(get_buf_props), &returned) == EAX_OK);
        REQUIRE(returned == sizeof(eax20_buffer_properties_t));
        REQUIRE(get_buf_props.lObstruction == -1200);
        REQUIRE(get_buf_props.lOcclusion == -1500);
    }

    SECTION("Deferred Property Commit") {
        eax20_listener_properties_t deferred_props{};
        deferred_props.dwEnvironment = EAX_ENVIRONMENT_AUDITORIUM;
        deferred_props.flDecayTime = 5.0f;

        // Bit 31 set indicates deferred mode; deferred ALLPARAMETERS = 0x80000001
        const uint32_t deferred_prop_id = 0x80000001u;
        REQUIRE(eax_prop_set(ctx, nullptr, &guid_eax20_listener, deferred_prop_id, nullptr, 0, &deferred_props, sizeof(deferred_props)) == EAX_OK);

        // Before commit: old value remains
        eax20_listener_properties_t current_props{};
        REQUIRE(eax_prop_get(ctx, nullptr, &guid_eax20_listener, 1, nullptr, 0, &current_props, sizeof(current_props), nullptr) == EAX_OK);
        REQUIRE(current_props.dwEnvironment != EAX_ENVIRONMENT_AUDITORIUM);

        // After commit: new value takes effect
        REQUIRE(eax_listener_commit_deferred(ctx) == EAX_OK);
        REQUIRE(eax_prop_get(ctx, nullptr, &guid_eax20_listener, 1, nullptr, 0, &current_props, sizeof(current_props), nullptr) == EAX_OK);
        REQUIRE(current_props.dwEnvironment == EAX_ENVIRONMENT_AUDITORIUM);
        REQUIRE(current_props.flDecayTime == Catch::Approx(5.0f));
    }

    SECTION("Boundary Values and Edge Case Clamping") {
        // Zero decay time should clamp to valid minimum (0.1s)
        float zero_decay = 0.0f;
        REQUIRE(eax_prop_set(ctx, nullptr, &guid_eax20_listener, 5, nullptr, 0, &zero_decay, sizeof(float)) == EAX_OK);
        eax20_listener_properties_t props{};
        REQUIRE(eax_prop_get(ctx, nullptr, &guid_eax20_listener, 1, nullptr, 0, &props, sizeof(props), nullptr) == EAX_OK);
        REQUIRE(props.flDecayTime >= 0.1f);

        // Negative decay HF ratio should clamp to valid range
        float neg_hf = -2.0f;
        REQUIRE(eax_prop_set(ctx, nullptr, &guid_eax20_listener, 6, nullptr, 0, &neg_hf, sizeof(float)) == EAX_OK);
        REQUIRE(eax_prop_get(ctx, nullptr, &guid_eax20_listener, 1, nullptr, 0, &props, sizeof(props), nullptr) == EAX_OK);
        REQUIRE(props.flDecayHFRatio >= 0.1f);

        // Positive volume should clamp to 0 mB (DirectSound max volume)
        REQUIRE(eax_buffer_set_volume(buf, 500) == EAX_OK);
        int32_t vol = 0;
        REQUIRE(eax_buffer_get_volume(buf, &vol) == EAX_OK);
        REQUIRE(vol == 0);

        // Pan out of bounds should clamp to [-10000, 10000]
        REQUIRE(eax_buffer_set_pan(buf, 25000) == EAX_OK);
        int32_t pan = 0;
        REQUIRE(eax_buffer_get_pan(buf, &pan) == EAX_OK);
        REQUIRE(pan == 10000);

        // EAX 1.0 Buffer mix negative clamping
        float neg_mix = -0.5f;
        REQUIRE(eax_prop_set(ctx, buf, &guid_eax10_buffer, 1, nullptr, 0, &neg_mix, sizeof(float)) == EAX_OK);
        float out_mix = 0.0f;
        REQUIRE(eax_prop_get(ctx, buf, &guid_eax10_buffer, 1, nullptr, 0, &out_mix, sizeof(float), nullptr) == EAX_OK);
        REQUIRE(out_mix == Catch::Approx(0.0f));

        // Cone outside volume clamping [-10000, 0]
        REQUIRE(eax_buffer_set_3d_cone_outside_volume(buf, 200, EAX_DS3D_IMMEDIATE) == EAX_OK);
        int32_t out_vol = 0;
        REQUIRE(eax_buffer_get_3d_cone_outside_volume(buf, &out_vol) == EAX_OK);
        REQUIRE(out_vol == 0);

        REQUIRE(eax_buffer_set_3d_cone_outside_volume(buf, -15000, EAX_DS3D_IMMEDIATE) == EAX_OK);
        REQUIRE(eax_buffer_get_3d_cone_outside_volume(buf, &out_vol) == EAX_OK);
        REQUIRE(out_vol == -10000);

        // Cone angles clamping [0, 360]
        REQUIRE(eax_buffer_set_3d_cone_angles(buf, 400, 500, EAX_DS3D_IMMEDIATE) == EAX_OK);
        uint32_t in_angle = 0, out_angle = 0;
        REQUIRE(eax_buffer_get_3d_cone_angles(buf, &in_angle, &out_angle) == EAX_OK);
        REQUIRE(in_angle == 360);
        REQUIRE(out_angle == 360);

        // Frequency original (0) and clamping [100, 200000]
        REQUIRE(eax_buffer_set_frequency(buf, 0) == EAX_OK);
        uint32_t freq = 0;
        REQUIRE(eax_buffer_get_frequency(buf, &freq) == EAX_OK);
        REQUIRE(freq >= 8000);

        REQUIRE(eax_buffer_set_frequency(buf, 500000) == EAX_OK);
        REQUIRE(eax_buffer_get_frequency(buf, &freq) == EAX_OK);
        REQUIRE(freq == 200000);

        // All 3D buffer parameters sanitize
        eax_ds3dbuffer_t ds3d_test{};
        ds3d_test.dwSize = sizeof(eax_ds3dbuffer_t);
        ds3d_test.lConeOutsideVolume = 1000;
        ds3d_test.dwInsideConeAngle = 720;
        ds3d_test.dwOutsideConeAngle = 720;
        ds3d_test.flMinDistance = -5.0f;
        ds3d_test.flMaxDistance = -10.0f;
        REQUIRE(eax_buffer_set_3d_all_parameters(buf, &ds3d_test, EAX_DS3D_IMMEDIATE) == EAX_OK);
        eax_ds3dbuffer_t ds3d_out{};
        REQUIRE(eax_buffer_get_3d_all_parameters(buf, &ds3d_out) == EAX_OK);
        REQUIRE(ds3d_out.lConeOutsideVolume == 0);
        REQUIRE(ds3d_out.dwInsideConeAngle == 360);
        REQUIRE(ds3d_out.dwOutsideConeAngle == 360);
        REQUIRE(ds3d_out.flMinDistance >= 1.0f);
        REQUIRE(ds3d_out.flMaxDistance >= ds3d_out.flMinDistance);
    }

    REQUIRE(eax_buffer_destroy(buf) == EAX_OK);
    REQUIRE(eax_context_destroy(ctx) == EAX_OK);
}
