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
    if (!m_memory) {
        // Fallback to standard aligned_alloc
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
        VirtualFree(m_memory, 0, MEM_RELEASE);
#else
        munmap(m_memory, m_capacity);
#endif
        m_memory = nullptr;
    }
}

void* MemoryArena::allocate(size_t bytes, size_t alignment) noexcept {
    if (!m_memory || bytes == 0) {
        return nullptr;
    }

    size_t current = m_offset.load(std::memory_order_relaxed);
    while (true) {
        // Calculate aligned position
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

void MemoryArena::reset() noexcept {
    m_offset.store(0, std::memory_order_release);
    if (m_memory) {
        std::memset(m_memory, 0, m_capacity);
    }
}

} // namespace eax
