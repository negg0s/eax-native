#ifndef EAX_NATIVE_SEQLOCK_H
#define EAX_NATIVE_SEQLOCK_H

#include <atomic>
#include <cstdint>

namespace eax {

/**
 * Lock-free Sequence Lock (SeqLock) for tearing-free snapshots of multi-field POD structs.
 * Allows writers to update data structures without blocking readers or acquiring mutexes on the RT thread.
 * Readers retry only when a concurrent write was in flight, guaranteeing complete data consistency.
 */
class SeqLock {
public:
    SeqLock() noexcept = default;

    void write_begin() noexcept {
        uint32_t s = m_seq.load(std::memory_order_relaxed);
        m_seq.store(s + 1, std::memory_order_release); // Odd indicates write in progress
        std::atomic_thread_fence(std::memory_order_release);
    }

    void write_end() noexcept {
        std::atomic_thread_fence(std::memory_order_release);
        uint32_t s = m_seq.load(std::memory_order_relaxed);
        m_seq.store(s + 1, std::memory_order_release); // Back to even indicates consistent state
    }

    [[nodiscard]] uint32_t read_begin() const noexcept {
        uint32_t s = m_seq.load(std::memory_order_acquire);
        while (s & 1) { // If write is in flight, spin wait for even sequence
            s = m_seq.load(std::memory_order_acquire);
        }
        return s;
    }

    [[nodiscard]] bool read_retry(uint32_t s) const noexcept {
        std::atomic_thread_fence(std::memory_order_acquire);
        return m_seq.load(std::memory_order_relaxed) != s;
    }

    [[nodiscard]] uint32_t sequence() const noexcept {
        return m_seq.load(std::memory_order_relaxed);
    }

private:
    std::atomic<uint32_t> m_seq{0};
};

} // namespace eax

#endif // EAX_NATIVE_SEQLOCK_H
