#include "arena.h"
#include <cstdlib>
#include <cstring>

#if defined(_WIN32)
    #include <windows.h>
#else
    #include <sys/mman.h>
    #include <unistd.h>
#endif

namespace eax {

MemoryArena::MemoryArena(size_t capacity) noexcept
    : m_capacity(capacity) {
#if defined(_WIN32)
    m_memory = static_cast<uint8_t*>(VirtualAlloc(
        nullptr, capacity, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
#else
    m_memory = static_cast<uint8_t*>(mmap(
        nullptr, capacity, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));
    if (m_memory == MAP_FAILED) {
        m_memory = nullptr;
    }
#endif
    m_heap_backed = false;
    if (!m_memory) {
        // Fallback to standard aligned_alloc
        m_heap_backed = true;
        #if defined(_WIN32)
            m_memory = static_cast<uint8_t*>(_aligned_malloc(capacity, CACHE_LINE_SIZE));
        #else
            m_memory = static_cast<uint8_t*>(std::aligned_alloc(CACHE_LINE_SIZE, capacity));
        #endif
    }
}

MemoryArena::~MemoryArena() noexcept {
    if (m_memory) {
#if defined(_WIN32)
        if (m_heap_backed) {
            _aligned_free(m_memory);
        } else {
            VirtualFree(m_memory, 0, MEM_RELEASE);
        }
#else
        if (m_heap_backed) {
            std::free(m_memory);
        } else {
            munmap(m_memory, m_capacity);
        }
#endif
        m_memory = nullptr;
    }
}

void* MemoryArena::allocate(size_t bytes, size_t alignment) noexcept {
    if (!m_memory || bytes == 0) {
        return nullptr;
    }

    if (alignment <= CACHE_LINE_SIZE) {
        for (auto& entry : m_free_list) {
            uint8_t* expected = entry.ptr.load(std::memory_order_acquire);
            if (expected && entry.bytes >= bytes && entry.ptr.compare_exchange_strong(
                    expected, nullptr, std::memory_order_acq_rel, std::memory_order_relaxed)) {
                entry.bytes = 0;
                return expected;
            }
        }
    }

    size_t current = m_offset.load(std::memory_order_relaxed);
    while (true) {
        const size_t remainder = current % alignment;
        const size_t padding = (remainder == 0) ? 0 : (alignment - remainder);
        const size_t aligned_offset = current + padding;
        const size_t next_offset = aligned_offset + bytes;

        if (next_offset > m_capacity) {
            return nullptr; // Arena exhausted
        }

        if (m_offset.compare_exchange_weak(
                current, next_offset, std::memory_order_release, std::memory_order_relaxed)) {
            return m_memory + aligned_offset;
        }
    }
}

void MemoryArena::free(void* ptr, size_t bytes) noexcept {
    if (!m_memory || !ptr || bytes == 0) {
        return;
    }

    const auto* block = static_cast<const uint8_t*>(ptr);
    if (block < m_memory || block + bytes > m_memory + m_capacity) {
        return;
    }

    for (auto& entry : m_free_list) {
        uint8_t* expected = nullptr;
        if (entry.ptr.compare_exchange_strong(
                expected, static_cast<uint8_t*>(ptr), std::memory_order_acq_rel, std::memory_order_relaxed)) {
            entry.bytes = bytes;
            return;
        }
    }
}

void MemoryArena::reset() noexcept {
    for (auto& entry : m_free_list) {
        entry.ptr.store(nullptr, std::memory_order_relaxed);
        entry.bytes = 0;
    }
    m_offset.store(0, std::memory_order_release);
    if (m_memory) {
        std::memset(m_memory, 0, m_capacity);
    }
}

} // namespace eax
