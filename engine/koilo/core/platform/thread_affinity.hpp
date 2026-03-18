// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

/// @file thread_affinity.hpp
/// @brief Cross-platform CPU core pinning for the main thread.
///
/// Pins the calling thread to a single randomly-selected performance core
/// to reduce jitter from core migration and frequency scaling.
/// Supported: Linux, Windows, macOS. No-op on unsupported/embedded platforms.

#if defined(ARDUINO) || defined(TEENSYDUINO) || defined(ESP32)
    // Embedded: no-op
#elif defined(__linux__)
    #include <sched.h>
    #include <unistd.h>
    #include <cstdio>
    #include <cstdlib>
    #include <cstring>
    #include <fstream>
    #include <string>
    #include <vector>
#elif defined(_WIN32)
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #include <windows.h>
    #include <cstdio>
    #include <vector>
#elif defined(__APPLE__)
    #include <pthread.h>
    #include <mach/thread_act.h>
    #include <mach/thread_policy.h>
    #include <cstdio>
#endif

#include <cstdint>
#include <koilo/kernel/logging/log.hpp>

namespace koilo {
namespace platform {

/// Pin the calling thread to a single random performance core.
/// @return The core ID pinned to, or -1 on failure / unsupported platform.
inline int PinToPerformanceCore() {
#if defined(ARDUINO) || defined(TEENSYDUINO) || defined(ESP32)
    return -1; // No-op on embedded
#elif defined(__linux__)
    // Detect performance cores via sysfs cpu_capacity (big.LITTLE / hybrid)
    // or fall back to online cores if capacity info is unavailable.
    long nprocs = sysconf(_SC_NPROCESSORS_ONLN);
    if (nprocs <= 0) return -1;

    std::vector<int> perfCores;
    int maxCapacity = 0;

    // First pass: find max capacity to identify performance cores
    for (int i = 0; i < (int)nprocs; ++i) {
        char path[128];
        std::snprintf(path, sizeof(path),
                      "/sys/devices/system/cpu/cpu%d/cpu_capacity", i);
        std::ifstream f(path);
        if (f.is_open()) {
            int cap = 0;
            f >> cap;
            if (cap > maxCapacity) maxCapacity = cap;
        }
    }

    // Second pass: collect cores at max capacity (or all if no capacity info)
    for (int i = 0; i < (int)nprocs; ++i) {
        if (maxCapacity > 0) {
            char path[128];
            std::snprintf(path, sizeof(path),
                          "/sys/devices/system/cpu/cpu%d/cpu_capacity", i);
            std::ifstream f(path);
            if (f.is_open()) {
                int cap = 0;
                f >> cap;
                if (cap >= maxCapacity) perfCores.push_back(i);
                continue;
            }
        }
        perfCores.push_back(i);
    }

    if (perfCores.empty()) return -1;

    // Pick a random performance core
    unsigned seed = 0;
    std::ifstream urandom("/dev/urandom", std::ios::binary);
    if (urandom.is_open()) {
        urandom.read(reinterpret_cast<char*>(&seed), sizeof(seed));
    } else {
        seed = static_cast<unsigned>(nprocs * 7919);
    }
    int chosen = perfCores[seed % perfCores.size()];

    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(chosen, &cpuset);
    if (sched_setaffinity(0, sizeof(cpuset), &cpuset) == 0) {
        KL_DBG("ThreadAffinity", "Pinned to core %d (%zu performance cores available)",
                     chosen, perfCores.size());
        return chosen;
    }
    return -1;

#elif defined(_WIN32)
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    int nprocs = static_cast<int>(sysInfo.dwNumberOfProcessors);
    if (nprocs <= 0) return -1;

    // Windows doesn't expose big.LITTLE topology easily;
    // use all available processors as candidates.
    std::vector<int> cores;
    for (int i = 0; i < nprocs; ++i) cores.push_back(i);

    // Pick a random core using QueryPerformanceCounter as seed
    LARGE_INTEGER qpc;
    QueryPerformanceCounter(&qpc);
    unsigned seed = static_cast<unsigned>(qpc.LowPart);
    int chosen = cores[seed % cores.size()];

    DWORD_PTR mask = static_cast<DWORD_PTR>(1) << chosen;
    if (SetThreadAffinityMask(GetCurrentThread(), mask) != 0) {
        KL_DBG("ThreadAffinity", "Pinned to core %d (%d cores available)",
                     chosen, nprocs);
        return chosen;
    }
    return -1;

#elif defined(__APPLE__)
    // macOS doesn't support hard core pinning, but we can set an affinity tag
    // hint to the scheduler to keep the thread on one core.
    thread_affinity_policy_data_t policy;
    policy.affinity_tag = 1; // Non-zero tag groups thread to same core
    kern_return_t ret = thread_policy_set(
        pthread_mach_thread_np(pthread_self()),
        THREAD_AFFINITY_POLICY,
        reinterpret_cast<thread_policy_t>(&policy),
        THREAD_AFFINITY_POLICY_COUNT);
    if (ret == KERN_SUCCESS) {
        KL_DBG("ThreadAffinity", "Set affinity tag (macOS hint)");
        return 0;
    }
    return -1;

#else
    return -1; // Unsupported platform
#endif
}

} // namespace platform
} // namespace koilo
