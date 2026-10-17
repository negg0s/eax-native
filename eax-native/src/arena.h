#ifndef EAX_NATIVE_ARENA_H
#define EAX_NATIVE_ARENA_H

#include <cstddef>
#include <cstdint>
#include <span>
#include <atomic>
#include <new>

namespace eax {

// Pre-allocated memory arena guaranteeing zero dynamic malloc/free during audio hot paths
class MemoryArena {
public:
    static constexpr size_t DEFAULT_ARENA_SIZE = 64 * 1024 * 1024; // 64 MB
    static constexpr size_t CACHE_LINE_SIZE = 64;

    explicit MemoryArena(size_t capacity = DEFAULT_ARENA_SIZE) noexcept;
    ~MemoryArena() noexcept;

    MemoryArena(const MemoryArena&) = delete;
    MemoryArena& operator=(const MemoryArena&) = delete;
    MemoryArena(MemoryArena&&) noexcept = delete;
    MemoryArena& operator=(MemoryArena&&) noexcept = delete;

    [[nodiscard]] void* allocate(size_t bytes, size_t alignment = CACHE_LINE_SIZE) noexcept;
    void reset() noexcept;

    [[nodiscard]] size_t bytes_allocated() const noexcept {
        return m_offset.load(std::memory_order_relaxed);
    }

    [[nodiscard]] size_t capacity() const noexcept {
        return m_capacity;
    }

private:
    uint8_t* m_memory{nullptr};
    size_t m_capacity{0};
    std::atomic<size_t> m_offset{0};
};

// Circular ring buffer operating on pre-allocated arena slices
class CircularSampleBuffer {
public:
    CircularSampleBuffer() noexcept = default;

    void initialize(void* memory, size_t capacity_bytes) noexcept {
        m_data = static_cast<uint8_t*>(memory);
        m_capacity_bytes = capacity_bytes;
        m_write_pos.store(0, std::memory_order_relaxed);
        m_play_pos.store(0, std::memory_order_relaxed);
    }

    [[nodiscard]] uint8_t* data() noexcept { return m_data; }
    [[nodiscard]] const uint8_t* data() const noexcept { return m_data; }
    [[nodiscard]] size_t capacity() const noexcept { return m_capacity_bytes; }

    [[nodiscard]] size_t get_write_pos() const noexcept {
        return m_write_pos.load(std::memory_order_acquire);
    }

    void set_write_pos(size_t pos) noexcept {
        m_write_pos.store(pos % (m_capacity_bytes ? m_capacity_bytes : 1), std::memory_order_release);
    }

    [[nodiscard]] size_t get_play_pos() const noexcept {
        return m_play_pos.load(std::memory_order_acquire);
    }

    void set_play_pos(size_t pos) noexcept {
        m_play_pos.store(pos % (m_capacity_bytes ? m_capacity_bytes : 1), std::memory_order_release);
    }

    void lock_region(size_t offset, size_t bytes,
                     void** out_ptr1, uint32_t* out_bytes1,
                     void** out_ptr2, uint32_t* out_bytes2) noexcept {
        if (!m_data || m_capacity_bytes == 0) {
            if (out_ptr1) *out_ptr1 = nullptr;
            if (out_bytes1) *out_bytes1 = 0;
            if (out_ptr2) *out_ptr2 = nullptr;
            if (out_bytes2) *out_bytes2 = 0;
            return;
        }

        const size_t norm_offset = offset % m_capacity_bytes;
        const size_t clamped_bytes = (bytes > m_capacity_bytes) ? m_capacity_bytes : bytes;

        if (norm_offset + clamped_bytes <= m_capacity_bytes) {
            if (out_ptr1) *out_ptr1 = m_data + norm_offset;
            if (out_bytes1) *out_bytes1 = static_cast<uint32_t>(clamped_bytes);
            if (out_ptr2) *out_ptr2 = nullptr;
            if (out_bytes2) *out_bytes2 = 0;
        } else {
            const size_t first_part = m_capacity_bytes - norm_offset;
            const size_t second_part = clamped_bytes - first_part;
            if (out_ptr1) *out_ptr1 = m_data + norm_offset;
            if (out_bytes1) *out_bytes1 = static_cast<uint32_t>(first_part);
            if (out_ptr2) *out_ptr2 = m_data;
            if (out_bytes2) *out_bytes2 = static_cast<uint32_t>(second_part);
        }
    }

private:
    uint8_t* m_data{nullptr};
    size_t m_capacity_bytes{0};
    std::atomic<size_t> m_write_pos{0};
    std::atomic<size_t> m_play_pos{0};
};

} // namespace eax

#endif // EAX_NATIVE_ARENA_H
