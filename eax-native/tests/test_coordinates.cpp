#include "catch_amalgamated.hpp"
#include "spatializer.h"
#include "eax_native_abi.h"

TEST_CASE("DirectSound3D Coordinates & Attenuation", "[spatial]") {
    SECTION("Left-Handed to Right-Handed Vector Mapping") {
        eax_vector_t ds_vec = { 1.5f, 2.5f, 3.5f }; // DS +Z forward
        eax_vector_t oal_vec = eax::ds_to_openal_vector(ds_vec);
        REQUIRE(oal_vec.x == 1.5f);
        REQUIRE(oal_vec.y == 2.5f);
        REQUIRE(oal_vec.z == -3.5f); // OpenAL -Z forward

        eax_vector_t roundtrip = eax::openal_to_ds_vector(oal_vec);
        REQUIRE(roundtrip.x == ds_vec.x);
        REQUIRE(roundtrip.y == ds_vec.y);
        REQUIRE(roundtrip.z == ds_vec.z);
    }

    SECTION("Distance Attenuation Curve") {
        const float min_dist = 1.0f;
        const float max_dist = 100.0f;
        const float rolloff = 1.0f;

        // Inside min distance -> full gain
        float g0 = eax::Spatializer::calculate_distance_attenuation(0.5f, min_dist, max_dist, rolloff);
        REQUIRE(g0 == Catch::Approx(1.0f));

        // Exactly at min distance -> full gain
        float g1 = eax::Spatializer::calculate_distance_attenuation(1.0f, min_dist, max_dist, rolloff);
        REQUIRE(g1 == Catch::Approx(1.0f));

        // At 2x min distance with rolloff 1.0 -> gain should be 1 / (1 + 1*(2-1)) = 0.5
        float g2 = eax::Spatializer::calculate_distance_attenuation(2.0f, min_dist, max_dist, rolloff);
        REQUIRE(g2 == Catch::Approx(0.5f));

        // At 10x min distance -> gain should be 1 / (1 + 1*(10-1)) = 0.1
        float g10 = eax::Spatializer::calculate_distance_attenuation(10.0f, min_dist, max_dist, rolloff);
        REQUIRE(g10 == Catch::Approx(0.1f));

        // Beyond max distance -> clamped at max_dist (100m)
        float g100 = eax::Spatializer::calculate_distance_attenuation(100.0f, min_dist, max_dist, rolloff);
        float g200 = eax::Spatializer::calculate_distance_attenuation(200.0f, min_dist, max_dist, rolloff);
        REQUIRE(g200 == Catch::Approx(g100));
    }

    SECTION("Panning Soft Formula (Fix R-002)") {
        // Center pan (0 mB) -> panf = 0.0f
        float p_center = eax::calculate_panning_soft(0);
        REQUIRE(p_center == Catch::Approx(0.0f).margin(0.001f));

        // Full left (-10000 mB) -> panf = -1.0f
        float p_left = eax::calculate_panning_soft(-10000);
        REQUIRE(p_left == Catch::Approx(-1.0f).margin(0.001f));

        // Full right (+10000 mB) -> panf = +1.0f
        float p_right = eax::calculate_panning_soft(10000);
        REQUIRE(p_right == Catch::Approx(1.0f).margin(0.001f));
    }
}
