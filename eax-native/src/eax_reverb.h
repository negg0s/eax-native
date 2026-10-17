#ifndef EAX_NATIVE_EAX_REVERB_H
#define EAX_NATIVE_EAX_REVERB_H

#include <vector>
#include <array>
#include <cstring>
#include "eax_native_abi.h"

namespace eax {

// Known GUIDs
inline constexpr eax_guid_t GUID_EAX10_Listener = {
    0x4A4E6FC1, 0xC341, 0x11D1, {0xB7, 0x3A, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00}
};
inline constexpr eax_guid_t GUID_EAX10_Buffer = {
    0x4A4E6FC0, 0xC341, 0x11D1, {0xB7, 0x3A, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00}
};
inline constexpr eax_guid_t GUID_EAX20_Listener = {
    0x0306A6A8, 0xB224, 0x11D2, {0x99, 0xE5, 0x00, 0x00, 0xE8, 0xD8, 0xC7, 0x22}
};
inline constexpr eax_guid_t GUID_EAX20_Buffer = {
    0x0306A6A7, 0xB224, 0x11D2, {0x99, 0xE5, 0x00, 0x00, 0xE8, 0xD8, 0xC7, 0x22}
};
inline constexpr eax_guid_t GUID_EAX30_Listener = {
    0xA8FA6882, 0xB476, 0x11D3, {0xBD, 0xB9, 0x00, 0xC0, 0xF0, 0x2D, 0xDF, 0x87}
};
inline constexpr eax_guid_t GUID_EAX30_Buffer = {
    0xA8FA6881, 0xB476, 0x11D3, {0xBD, 0xB9, 0x00, 0xC0, 0xF0, 0x2D, 0xDF, 0x87}
};
inline constexpr eax_guid_t GUID_EAX40_Context = {
    0x1D4870AD, 0x0DEF, 0x43C0, {0xA4, 0x0C, 0x52, 0x36, 0x32, 0x29, 0x63, 0x42}
};
inline constexpr eax_guid_t GUID_EAX40_FXSlot0 = {
    0xC4D79F1E, 0xF1AC, 0x436B, {0xA8, 0x1D, 0xA7, 0x38, 0xE7, 0x04, 0x54, 0x69}
};
inline constexpr eax_guid_t GUID_EAX40_Source = {
    0x1B86B823, 0x22DF, 0x4EAE, {0x8B, 0x3C, 0x12, 0x78, 0xCE, 0x54, 0x42, 0x27}
};

inline bool are_guids_equal(const eax_guid_t& a, const eax_guid_t& b) noexcept {
    return (a.data1 == b.data1) && (a.data2 == b.data2) && (a.data3 == b.data3) &&
           (std::memcmp(a.data4, b.data4, 8) == 0);
}

// Preset definition for REVERB_CAVE per openal-soft al/eax/presets.hpp
inline constexpr eax20_listener_properties_t PRESET_CAVE = {
    -1000,          // lRoom (-10.0 dB)
    0,              // lRoomHF (0.0 dB)
    0.0f,           // flRoomRolloffFactor
    2.91f,          // flDecayTime (2.91 seconds)
    1.30f,          // flDecayHFRatio
    -602,           // lReflections (-6.02 dB)
    0.015f,         // flReflectionsDelay (15 ms)
    -302,           // lReverb (-3.02 dB)
    0.022f,         // flReverbDelay (22 ms)
    EAX_ENVIRONMENT_CAVE,
    14.6f,          // flEnvironmentSize
    1.0f,           // flEnvironmentDiffusion
    -5.0f,          // flAirAbsorptionHF
    0x3F            // dwFlags
};

inline constexpr eax20_listener_properties_t PRESET_GENERIC = {
    -1000,
    -100,
    0.0f,
    1.49f,
    0.83f,
    -2602,
    0.007f,
    200,
    0.011f,
    EAX_ENVIRONMENT_GENERIC,
    7.5f,
    1.0f,
    -5.0f,
    0x3F
};

// Reverb DSP Comb Filter
class CombFilter {
public:
    void reserve(size_t n) noexcept {
        if (m_buffer.capacity() < n) {
            m_buffer.reserve(n);
        }
    }

    void init(size_t delay_samples, float feedback, float damping) noexcept {
        m_buffer.assign(delay_samples + 1, 0.0f);
        m_index = 0;
        m_feedback = feedback;
        m_damping = damping;
        m_filter_state = 0.0f;
    }

    [[nodiscard]] float process(float input) noexcept {
        if (m_buffer.empty()) return input;

        const float output = m_buffer[m_index];
        m_filter_state = output * (1.0f - m_damping) + m_filter_state * m_damping;
        m_buffer[m_index] = input + m_filter_state * m_feedback;

        m_index = (m_index + 1) % m_buffer.size();
        return output;
    }

private:
    std::vector<float> m_buffer;
    size_t m_index{0};
    float m_feedback{0.5f};
    float m_damping{0.2f};
    float m_filter_state{0.0f};
};

// Reverb DSP Allpass Filter
class AllpassFilter {
public:
    void reserve(size_t n) noexcept {
        if (m_buffer.capacity() < n) {
            m_buffer.reserve(n);
        }
    }

    void init(size_t delay_samples, float feedback) noexcept {
        m_buffer.assign(delay_samples + 1, 0.0f);
        m_index = 0;
        m_feedback = feedback;
    }

    [[nodiscard]] float process(float input) noexcept {
        if (m_buffer.empty()) return input;

        const float buf_out = m_buffer[m_index];
        const float output = -input + buf_out;
        m_buffer[m_index] = input + (buf_out * m_feedback);

        m_index = (m_index + 1) % m_buffer.size();
        return output;
    }

private:
    std::vector<float> m_buffer;
    size_t m_index{0};
    float m_feedback{0.5f};
};

// Reverb Processor simulating EAX environment
class ReverbDSP {
public:
    ReverbDSP() noexcept;

    void configure(const eax20_listener_properties_t& props, uint32_t sample_rate) noexcept;
    void process(float input_mono, float& out_left, float& out_right) noexcept;
    void reset() noexcept;

    [[nodiscard]] float get_room_gain() const noexcept { return m_room_gain; }
    [[nodiscard]] float get_decay_time() const noexcept { return m_decay_time; }

private:
    uint32_t m_sample_rate{44100};
    float m_room_gain{0.0f};
    float m_decay_time{1.49f};
    float m_reverb_gain{0.5f};
    float m_reflections_gain{0.3f};

    // Pre-delay ring buffer
    std::vector<float> m_predelay_buffer;
    size_t m_predelay_idx{0};

    // 4 Parallel Comb Filters
    std::array<CombFilter, 4> m_combs;
    // 2 Cascaded Allpass Filters
    std::array<AllpassFilter, 2> m_allpasses;
};

} // namespace eax

#endif // EAX_NATIVE_EAX_REVERB_H
