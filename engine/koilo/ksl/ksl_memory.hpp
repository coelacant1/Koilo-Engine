// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

// Platform-specific executable memory allocation.
// Used by the ELF loader to allocate memory for loaded shader code.

#include <cstddef>

#if defined(__linux__) || defined(__APPLE__)
#include <sys/mman.h>
#elif defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

namespace ksl {

// Allocate a block of memory with read+write+execute permissions.
inline void* AllocExecutable(size_t size) {
#if defined(__linux__) || defined(__APPLE__)
    void* p = mmap(nullptr, size,
                   PROT_READ | PROT_WRITE | PROT_EXEC,
                   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    return (p == MAP_FAILED) ? nullptr : p;
#elif defined(_WIN32)
    return VirtualAlloc(nullptr, size,
                        MEM_COMMIT | MEM_RESERVE,
                        PAGE_EXECUTE_READWRITE);
#elif defined(TEENSYDUINO)
    // Teensy 4.x: RAM2 (OCRAM) is executable on Cortex-M7.
    // Simple bump allocator - shader modules are rarely unloaded.
    // Pool size configurable via KSL_RAM2_POOL_SIZE (default 64KB).
    #ifndef KSL_RAM2_POOL_SIZE
    #define KSL_RAM2_POOL_SIZE (64 * 1024)
    #endif
    static DMAMEM uint8_t pool[KSL_RAM2_POOL_SIZE];
    static size_t used = 0;
    size = (size + 7) & ~size_t(7); // 8-byte align
    if (used + size > sizeof(pool)) return nullptr;
    void* p = &pool[used];
    used += size;
    return p;
#else
    (void)size;
    return nullptr;
#endif
}

// Free a block of executable memory.
inline void FreeExecutable(void* ptr, size_t size) {
#if defined(__linux__) || defined(__APPLE__)
    if (ptr) munmap(ptr, size);
#elif defined(_WIN32)
    if (ptr) VirtualFree(ptr, 0, MEM_RELEASE);
    (void)size;
#else
    // Teensy bump allocator and other platforms don't support individual frees
    (void)ptr; (void)size;
#endif
}

} // namespace ksl
