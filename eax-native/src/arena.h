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
    void free(void* ptr, size_t bytes) noexcept;
    void reset() noexcept;

    [[nodiscard]] size_t bytes_allocated() const noexcept {
        return m_offset.load(std::memory_order_relaxed);
    }

    [[nodiscard]] size_t capacity() const noexcept {
        return m_capacity;
    }

private:
    static constexpr size_t MAX_FREE_LIST = 128;
    struct FreeBlock {
        std::atomic<uint8_t*> ptr{nullptr};
        size_t bytes{0};
    };

    uint8_t* m_memory{nullptr};
    size_t m_capacity{0};
    std::atomic<size_t> m_offset{0};
    bool m_heap_backed{false};
    FreeBlock m_free_list[MAX_FREE_LIST]{};
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

    bool lock_region(size_t offset, size_t bytes,
                     void** out_ptr1, uint32_t* out_bytes1,
                     void** out_ptr2, uint32_t* out_bytes2) noexcept {
        if (out_ptr1) *out_ptr1 = nullptr;
        if (out_bytes1) *out_bytes1 = 0;
        if (out_ptr2) *out_ptr2 = nullptr;
        if (out_bytes2) *out_bytes2 = 0;
        if (!m_data || m_capacity_bytes == 0) {
            return false;
        }
        if (offset >= m_capacity_bytes || bytes == 0 || bytes > m_capacity_bytes - offset) {
            return false;
        }

        if (out_ptr1) *out_ptr1 = m_data + offset;
        if (out_bytes1) *out_bytes1 = static_cast<uint32_t>(bytes);
        return true;
    }

private:
    uint8_t* m_data{nullptr};
    size_t m_capacity_bytes{0};
    std::atomic<size_t> m_write_pos{0};
    std::atomic<size_t> m_play_pos{0};
};

} // namespace eax

#endif // EAX_NATIVE_ARENA_H
