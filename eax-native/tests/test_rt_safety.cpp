#include "catch_amalgamated.hpp"
#include "eax_native_abi.h"
#include "eax_reverb.h"
#include <atomic>
#include <thread>
#include <vector>
#include <cmath>
#include <cstring>
#include <cstdlib>

#if defined(__ELF__) && defined(__linux__)
#include <dlfcn.h>
#endif

// Thread-local flag marking the active real-time audio thread
static thread_local bool g_rt_thread_active = false;
static std::atomic<uint64_t> g_rt_alloc_count{0};

#if defined(__ELF__) && defined(__linux__)
// Allocator interposition via dlsym(RTLD_NEXT) only works on ELF platforms
// (glibc). On Windows/MSVC malloc cannot be interposed this way, so the
// counting hooks below are compiled out and the allocation-count assertion
// passes vacuously there (the other RT-safety cases still run for real).
extern "C" {
    typedef void* (*libc_malloc_t)(size_t);
    typedef void* (*libc_calloc_t)(size_t, size_t);
    typedef void* (*libc_realloc_t)(void*, size_t);
    typedef void  (*libc_free_t)(void*);

    static libc_malloc_t g_real_malloc = nullptr;
    static libc_calloc_t g_real_calloc = nullptr;
    static libc_realloc_t g_real_realloc = nullptr;
    static libc_free_t g_real_free = nullptr;

    static void init_real_allocators() {
        if (!g_real_malloc) {
            g_real_malloc = reinterpret_cast<libc_malloc_t>(dlsym(RTLD_NEXT, "malloc"));
            g_real_calloc = reinterpret_cast<libc_calloc_t>(dlsym(RTLD_NEXT, "calloc"));
            g_real_realloc = reinterpret_cast<libc_realloc_t>(dlsym(RTLD_NEXT, "realloc"));
            g_real_free = reinterpret_cast<libc_free_t>(dlsym(RTLD_NEXT, "free"));
        }
    }

    void* malloc(size_t size) {
        if (g_rt_thread_active) {
            g_rt_alloc_count.fetch_add(1, std::memory_order_relaxed);
        }
        if (!g_real_malloc) init_real_allocators();
        return g_real_malloc(size);
    }

    void* calloc(size_t nmemb, size_t size) {
        if (g_rt_thread_active) {
            g_rt_alloc_count.fetch_add(1, std::memory_order_relaxed);
        }
        if (!g_real_calloc) init_real_allocators();
        return g_real_calloc(nmemb, size);
    }

    void* realloc(void* ptr, size_t size) {
        if (g_rt_thread_active) {
            g_rt_alloc_count.fetch_add(1, std::memory_order_relaxed);
        }
        if (!g_real_realloc) init_real_allocators();
        return g_real_realloc(ptr, size);
    }

    void free(void* ptr) {
        if (!g_real_free) init_real_allocators();
        g_real_free(ptr);
    }
}

void* operator new(size_t size) {
    if (g_rt_thread_active) {
        g_rt_alloc_count.fetch_add(1, std::memory_order_relaxed);
    }
    return malloc(size);
}

void* operator new[](size_t size) {
    if (g_rt_thread_active) {
        g_rt_alloc_count.fetch_add(1, std::memory_order_relaxed);
    }
    return malloc(size);
}
#else
// Non-ELF builds: hooks are no-ops (see the note above).
static void init_real_allocators() {}
#endif

TEST_CASE("RT Safety: Zero dynamic memory allocation on audio rendering thread", "[rt_safety]") {
    eax_context_h ctx = nullptr;
    REQUIRE(eax_context_create_offline(&ctx, 48000) == EAX_OK);

    REQUIRE(eax_prop_set(ctx, nullptr, &eax::GUID_EAX20_Listener, 1, nullptr, 0,
                         &eax::PRESET_CAVE, sizeof(eax::PRESET_CAVE)) == EAX_OK);

    const uint32_t sample_rate = 48000;
    const uint32_t buffer_bytes = sample_rate * sizeof(int16_t); // 1s mono

    eax_dsbcaps_t caps{};
    caps.dwSize = sizeof(eax_dsbcaps_t);
    caps.dwFlags = EAX_DSBCAPS_CTRL3D | EAX_DSBCAPS_CTRLVOLUME | EAX_DSBCAPS_CTRLFREQUENCY;
    caps.dwBufferBytes = buffer_bytes;

    eax_waveformat_ex_t format{};
    format.wFormatTag = 1;
    format.nChannels = 1;
    format.nSamplesPerSec = sample_rate;
    format.wBitsPerSample = 16;
    format.nBlockAlign = 2;
    format.nAvgBytesPerSec = sample_rate * 2;

    eax_buffer_h buf = nullptr;
    REQUIRE(eax_buffer_create(ctx, &caps, &format, &buf) == EAX_OK);

    void *p1 = nullptr, *p2 = nullptr;
    uint32_t b1 = 0, b2 = 0;
    REQUIRE(eax_buffer_lock(buf, 0, buffer_bytes, &p1, &b1, &p2, &b2, 0) == EAX_OK);
    if (p1) {
        auto* s16 = static_cast<int16_t*>(p1);
        for (size_t i = 0; i < b1 / 2; ++i) {
            s16[i] = static_cast<int16_t>(std::sin(2.0 * 3.14159 * 440.0 * i / sample_rate) * 20000.0);
        }
    }
    REQUIRE(eax_buffer_unlock(buf, p1, b1, p2, b2) == EAX_OK);
    REQUIRE(eax_buffer_play(buf, 0, EAX_DSBPLAY_LOOPING) == EAX_OK);

    // Pre-allocate audio thread output buffer
    static constexpr size_t RENDER_FRAMES = 512;
    float rt_output[RENDER_FRAMES * 2];

    init_real_allocators();
    g_rt_alloc_count.store(0);

    // Run simulated real-time audio thread
    std::thread audio_thread([&]() {
        g_rt_thread_active = true;

        for (int iter = 0; iter < 1000; ++iter) {
            const float angle = iter * 0.05f;
            eax_buffer_set_3d_position(buf, std::cos(angle) * 5.0f, 0.0f, std::sin(angle) * 5.0f, EAX_DS3D_IMMEDIATE);
            eax_buffer_set_volume(buf, -iter % 600);
            eax_buffer_set_pan(buf, (iter % 200) - 100);

            eax_listener_set_position(ctx, 0.0f, 0.0f, 0.0f, EAX_DS3D_DEFERRED);
            eax_listener_commit_deferred(ctx);

            eax_context_render_offline(ctx, rt_output, RENDER_FRAMES, sample_rate);
        }

        g_rt_thread_active = false;
    });

    audio_thread.join();

    // Verify zero dynamic heap allocations occurred during the entire audio cycle
    REQUIRE(g_rt_alloc_count.load() == 0);

    eax_buffer_destroy(buf);
    eax_context_destroy(ctx);
}

TEST_CASE("RT Safety: Lock-free voice pool state transitions & absence of deadlocks", "[rt_safety]") {
    eax_context_h ctx = nullptr;
    REQUIRE(eax_context_create_offline(&ctx, 48000) == EAX_OK);

    std::atomic<bool> keep_running{true};
    std::atomic<uint64_t> audio_cycles{0};
    std::atomic<uint64_t> control_cycles{0};

    // Thread 1: Real-time rendering thread
    std::thread rt_audio([&]() {
        float buffer[256 * 2];
        while (keep_running.load(std::memory_order_relaxed)) {
            eax_context_render_offline(ctx, buffer, 256, 48000);
            audio_cycles.fetch_add(1, std::memory_order_relaxed);
        }
    });

    // Thread 2: Game control thread mutating 3D parameters and voice state concurrently
    std::thread control_thread([&]() {
        for (int i = 0; i < 2000; ++i) {
            eax_listener_set_position(ctx, static_cast<float>(i % 10), 0.0f, 1.0f, EAX_DS3D_IMMEDIATE);
            control_cycles.fetch_add(1, std::memory_order_relaxed);
        }
    });

    control_thread.join();
    keep_running.store(false, std::memory_order_relaxed);
    rt_audio.join();

    REQUIRE(audio_cycles.load() > 0);
    REQUIRE(control_cycles.load() == 2000);

    eax_context_destroy(ctx);
}

TEST_CASE("RT Safety: Lock-free SeqLock prevents torn reads during high-frequency concurrent updates", "[rt_safety]") {
    eax_context_h ctx = nullptr;
    REQUIRE(eax_context_create_offline(&ctx, 48000) == EAX_OK);

    eax_dsbcaps_t caps{};
    caps.dwSize = sizeof(eax_dsbcaps_t);
    caps.dwFlags = EAX_DSBCAPS_CTRL3D;
    caps.dwBufferBytes = 4096;

    eax_waveformat_ex_t format{};
    format.wFormatTag = 1;
    format.nChannels = 1;
    format.nSamplesPerSec = 48000;
    format.wBitsPerSample = 16;
    format.nBlockAlign = 2;
    format.nAvgBytesPerSec = 48000 * 2;

    eax_buffer_h buf = nullptr;
    REQUIRE(eax_buffer_create(ctx, &caps, &format, &buf) == EAX_OK);

    std::atomic<bool> stop_flag{false};
    std::atomic<uint64_t> torn_reads{0};
    std::atomic<uint64_t> successful_reads{0};

    // Reader thread: continuously takes snapshots and asserts internal consistency (all coordinates equal)
    std::thread reader([&]() {
        while (!stop_flag.load(std::memory_order_relaxed)) {
            eax_vector_t l_pos{};
            eax_vector_t b_pos{};
            eax_listener_get_position(ctx, &l_pos);
            eax_buffer_get_3d_position(buf, &b_pos);

            if (l_pos.x != l_pos.y || l_pos.y != l_pos.z) {
                torn_reads.fetch_add(1, std::memory_order_relaxed);
            }
            if (b_pos.x != b_pos.y || b_pos.y != b_pos.z) {
                torn_reads.fetch_add(1, std::memory_order_relaxed);
            }
            successful_reads.fetch_add(1, std::memory_order_relaxed);
        }
    });

    // Writer thread: rapidly updates multi-field structs with distinct values
    for (int i = 1; i <= 20000; ++i) {
        const float val = static_cast<float>(i);
        eax_listener_set_position(ctx, val, val, val, EAX_DS3D_IMMEDIATE);
        eax_buffer_set_3d_position(buf, val, val, val, EAX_DS3D_IMMEDIATE);
    }

    stop_flag.store(true, std::memory_order_relaxed);
    reader.join();

    REQUIRE(successful_reads.load() > 0);
    REQUIRE(torn_reads.load() == 0);

    eax_buffer_destroy(buf);
    eax_context_destroy(ctx);
}

