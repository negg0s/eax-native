#ifndef EAX_NATIVE_SPATIALIZER_H
#define EAX_NATIVE_SPATIALIZER_H

#include <cmath>
#include <algorithm>
#include "eax_native_abi.h"

namespace eax {

// Mathematical conversion utilities
inline float mb_to_gain(int32_t mb) noexcept {
    if (mb <= -10000) return 0.0f;
    return std::pow(10.0f, static_cast<float>(mb) / 2000.0f);
}

// Convert DirectSound millibel pan to OpenAL Soft normalized pan [-1.0, 1.0]
// Formula per dsoal/src/buffer.cpp:672-675:
// panf = (pan <= 0) ? (mb_to_gain(pan) - 1.0f) : (1.0f - mb_to_gain(-pan))
inline float calculate_panning_soft(int32_t pan_mb) noexcept {
    if (pan_mb <= 0) {
        return mb_to_gain(pan_mb) - 1.0f;
    } else {
        return 1.0f - mb_to_gain(-pan_mb);
    }
}

// DirectSound (left-handed) to OpenAL (right-handed) vector conversion: negate Z
inline eax_vector_t ds_to_openal_vector(const eax_vector_t& v) noexcept {
    return eax_vector_t{ v.x, v.y, -v.z };
}

inline eax_vector_t openal_to_ds_vector(const eax_vector_t& v) noexcept {
    return eax_vector_t{ v.x, v.y, -v.z };
}

struct SpatialGains {
    float left_gain{1.0f};
    float right_gain{1.0f};
    float distance_gain{1.0f};
    float cone_gain{1.0f};
};

class Spatializer {
public:
    static float calculate_distance_attenuation(
        float distance,
        float min_distance,
        float max_distance,
        float rolloff_factor,
        float distance_factor = 1.0f) noexcept {
        const float scaled_dist = (distance_factor > 0.0f) ? (distance / distance_factor) : distance;
        const float d_min = std::max(min_distance, 0.0001f);

        if (scaled_dist <= d_min) {
            return 1.0f;
        }

        float d_eff = scaled_dist;
        if (max_distance > d_min && scaled_dist >= max_distance) {
            d_eff = max_distance;
        }

        const float rolloff = std::max(rolloff_factor, 0.0f);
        const float denominator = d_min + rolloff * (d_eff - d_min);
        if (denominator <= 0.00001f) {
            return 1.0f;
        }
        return std::clamp(d_min / denominator, 0.0f, 1.0f);
    }

    static float calculate_cone_attenuation(
        const eax_vector_t& sound_pos,
        const eax_vector_t& cone_dir,
        const eax_vector_t& listener_pos,
        uint32_t inside_angle_deg,
        uint32_t outside_angle_deg,
        int32_t outside_volume_mb) noexcept {
        if (inside_angle_deg >= 360 || outside_angle_deg >= 360) {
            return 1.0f;
        }

        // Vector from sound to listener
        const float dx = listener_pos.x - sound_pos.x;
        const float dy = listener_pos.y - sound_pos.y;
        const float dz = listener_pos.z - sound_pos.z;
        const float dist = std::sqrt(dx * dx + dy * dy + dz * dz);
        if (dist <= 0.0001f) {
            return 1.0f;
        }

        const float to_lis_x = dx / dist;
        const float to_lis_y = dy / dist;
        const float to_lis_z = dz / dist;

        const float c_len = std::sqrt(cone_dir.x * cone_dir.x + cone_dir.y * cone_dir.y + cone_dir.z * cone_dir.z);
        if (c_len <= 0.0001f) {
            return 1.0f;
        }
        const float c_x = cone_dir.x / c_len;
        const float c_y = cone_dir.y / c_len;
        const float c_z = cone_dir.z / c_len;

        // Dot product gives cosine of angle
        const float dot = std::clamp(to_lis_x * c_x + to_lis_y * c_y + to_lis_z * c_z, -1.0f, 1.0f);
        const float angle_rad = std::acos(dot);
        const float angle_deg = angle_rad * 180.0f / 3.141592653589793f;

        const float half_in = static_cast<float>(inside_angle_deg) * 0.5f;
        const float half_out = static_cast<float>(outside_angle_deg) * 0.5f;

        if (angle_deg <= half_in) {
            return 1.0f;
        }
        const float outside_gain = mb_to_gain(outside_volume_mb);
        if (angle_deg >= half_out) {
            return outside_gain;
        }

        const float t = (angle_deg - half_in) / (half_out - half_in);
        return 1.0f + t * (outside_gain - 1.0f);
    }

    static SpatialGains compute_gains(
        const eax_ds3dlistener_t& listener,
        const eax_ds3dbuffer_t& buffer,
        int32_t buffer_volume_mb) noexcept {
        SpatialGains gains;
        const float base_vol = mb_to_gain(buffer_volume_mb);

        if (buffer.dwMode == EAX_DS3DMODE_DISABLE) {
            gains.left_gain = base_vol * 0.7071f;
            gains.right_gain = base_vol * 0.7071f;
            gains.distance_gain = 1.0f;
            gains.cone_gain = 1.0f;
            return gains;
        }

        // Relative coordinates (Left-Handed DirectSound space)
        float rel_x = buffer.vPosition.x;
        float rel_y = buffer.vPosition.y;
        float rel_z = buffer.vPosition.z;

        if (buffer.dwMode == EAX_DS3DMODE_NORMAL) {
            rel_x -= listener.vPosition.x;
            rel_y -= listener.vPosition.y;
            rel_z -= listener.vPosition.z;
        }

        const float dist = std::sqrt(rel_x * rel_x + rel_y * rel_y + rel_z * rel_z);
        gains.distance_gain = calculate_distance_attenuation(
            dist, buffer.flMinDistance, buffer.flMaxDistance,
            listener.flRolloffFactor,
            listener.flDistanceFactor);

        gains.cone_gain = calculate_cone_attenuation(
            buffer.vPosition, buffer.vConeOrientation, listener.vPosition,
            buffer.dwInsideConeAngle, buffer.dwOutsideConeAngle, buffer.lConeOutsideVolume);

        // Azimuth in horizontal plane (-X = full left, +X = full right)
        // In DirectSound coordinates (+X right, +Z forward):
        // angle = atan2(rel_x, rel_z)
        float azimuth = 0.0f;
        if (std::abs(rel_x) > 0.0001f || std::abs(rel_z) > 0.0001f) {
            azimuth = std::atan2(rel_x, (rel_z == 0.0f ? 0.0001f : rel_z));
        }

        // Horizontal panning factor in [-1, 1]
        const float pan = std::sin(azimuth);
        // Constant-power panning law
        const float angle_panning = (pan + 1.0f) * 0.25f * 3.141592653589793f; // [0, pi/2]
        const float p_left = std::cos(angle_panning);
        const float p_right = std::sin(angle_panning);

        const float total_scalar = base_vol * gains.distance_gain * gains.cone_gain;
        gains.left_gain = total_scalar * p_left;
        gains.right_gain = total_scalar * p_right;
        return gains;
    }
};

} // namespace eax

#endif // EAX_NATIVE_SPATIALIZER_H
