// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file memoryprofiler.hpp
 * @brief Memory profiling and tracking.
 *
 * @date 11/10/2025
 * @author Coela
 */

#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <cstddef>
#include <koilo/registry/reflect_macros.hpp>

namespace koilo {

/**
 * @struct MemoryAllocation
 * @brief Information about a memory allocation.
 */
struct MemoryAllocation {
    void* address;                       ///< Memory address
    size_t size;                         ///< Size in bytes
    std::string tag;                     ///< Allocation tag
    int frameNumber;                     ///< Frame when allocated

    MemoryAllocation()
        : address(nullptr), size(0), frameNumber(0) {}

    MemoryAllocation(void* addr, size_t sz, const std::string& tag, int frame)
        : address(addr), size(sz), tag(tag), frameNumber(frame) {}

    KL_BEGIN_FIELDS(MemoryAllocation)
        KL_FIELD(MemoryAllocation, address, "Address", 0, 0)
    KL_END_FIELDS

    KL_BEGIN_METHODS(MemoryAllocation)
        /* No reflected methods. */
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(MemoryAllocation)
        /* No reflected ctors. */
    KL_END_DESCRIBE(MemoryAllocation)

};

/**
 * @struct MemoryStats
 * @brief Memory usage statistics.
 */
struct MemoryStats {
    size_t totalAllocated;               ///< Total bytes allocated
    size_t totalFreed;                   ///< Total bytes freed
    size_t currentUsage;                 ///< Current bytes in use
    size_t peakUsage;                    ///< Peak bytes used
    int allocationCount;                 ///< Total allocations
    int freeCount;                       ///< Total frees
    int activeAllocations;               ///< Current active allocations

    MemoryStats()
        : totalAllocated(0), totalFreed(0), currentUsage(0), peakUsage(0),
          allocationCount(0), freeCount(0), activeAllocations(0) {}

    KL_BEGIN_FIELDS(MemoryStats)
        KL_FIELD(MemoryStats, totalAllocated, "Total allocated", 0, 0),
        KL_FIELD(MemoryStats, currentUsage, "Current usage", 0, 0),
        KL_FIELD(MemoryStats, peakUsage, "Peak usage", 0, 0),
        KL_FIELD(MemoryStats, allocationCount, "Allocation count", 0, 0)
    KL_END_FIELDS

    KL_BEGIN_METHODS(MemoryStats)
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(MemoryStats)
        KL_CTOR0(MemoryStats)
    KL_END_DESCRIBE(MemoryStats)
};

/**
 * @class MemoryProfiler
 * @brief Memory profiling and tracking system.
 *
 * The MemoryProfiler tracks:
 * - Memory allocations and frees
 * - Memory usage by tag
 * - Memory leaks
 * - Peak memory usage
 */
class MemoryProfiler {
private:
    static MemoryProfiler* instance;

    bool enabled;                        ///< Is profiling enabled?
    MemoryStats stats;                   ///< Overall statistics
    std::unordered_map<void*, MemoryAllocation> allocations;
    std::unordered_map<std::string, size_t> usageByTag;

    int currentFrame;                    ///< Current frame number

public:
    /**
     * @brief Constructor.
     */
    MemoryProfiler();

    /**
     * @brief Destructor.
     */
    ~MemoryProfiler();

    /**
     * @brief Gets the singleton instance.
     */
    static MemoryProfiler& GetInstance();

    // === Configuration ===

    /**
     * @brief Enables/disables profiling.
     */
    void SetEnabled(bool enable) { enabled = enable; }

    /**
     * @brief Checks if profiling is enabled.
     */
    bool IsEnabled() const { return enabled; }

    /**
     * @brief Sets the current frame number.
     */
    void SetCurrentFrame(int frame) { currentFrame = frame; }

    // === Tracking ===

    /**
     * @brief Tracks an allocation.
     */
    void TrackAllocation(void* address, size_t size, const std::string& tag = "Untagged");

    /**
     * @brief Tracks a deallocation.
     */
    void TrackFree(void* address);

    // === Statistics ===

    /**
     * @brief Gets overall memory statistics.
     */
    const MemoryStats& GetStats() const { return stats; }

    /**
     * @brief Gets memory usage by tag.
     */
    size_t GetUsageByTag(const std::string& tag) const;

    /**
     * @brief Gets all allocations.
     */
    const std::unordered_map<void*, MemoryAllocation>& GetAllocations() const {
        return allocations;
    }

    /**
     * @brief Gets usage by tag map.
     */
    const std::unordered_map<std::string, size_t>& GetUsageByTagMap() const {
        return usageByTag;
    }

    // === Reporting ===

    /**
     * @brief Prints a memory report to console.
     */
    void PrintReport() const;

    /**
     * @brief Gets a memory report as a string.
     */
    std::string GetReportString() const;

    /**
     * @brief Prints memory leaks (active allocations).
     */
    void PrintLeaks() const;

    /**
     * @brief Clears all profiling data.
     */
    void Clear();

    // === Utility ===

    /**
     * @brief Formats bytes as human-readable string (KB, MB, GB).
     */
    static std::string FormatBytes(size_t bytes);

    KL_BEGIN_FIELDS(MemoryProfiler)
        KL_FIELD(MemoryProfiler, enabled, "Enabled", 0, 1),
        KL_FIELD(MemoryProfiler, currentFrame, "Current frame", 0, 0)
    KL_END_FIELDS

    KL_BEGIN_METHODS(MemoryProfiler)
        KL_METHOD_AUTO(MemoryProfiler, SetEnabled, "Set enabled"),
        KL_METHOD_AUTO(MemoryProfiler, IsEnabled, "Is enabled"),
        KL_METHOD_AUTO(MemoryProfiler, GetStats, "Get stats"),
        KL_METHOD_AUTO(MemoryProfiler, PrintReport, "Print report"),
        KL_METHOD_AUTO(MemoryProfiler, PrintLeaks, "Print leaks"),
        KL_METHOD_AUTO(MemoryProfiler, Clear, "Clear")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(MemoryProfiler)
        KL_CTOR0(MemoryProfiler)
    KL_END_DESCRIBE(MemoryProfiler)
};

/**
 * @class MemoryScope
 * @brief RAII helper for tracking scoped allocations.
 */
class MemoryScope {
private:
    std::string tag;
    std::vector<void*> trackedAllocations;

public:
    MemoryScope(const std::string& tag) : tag(tag) {}

    ~MemoryScope() {
        // All tracked allocations should be freed in this scope
        for (void* ptr : trackedAllocations) {
            MemoryProfiler::GetInstance().TrackFree(ptr);
        }
    }

    void Track(void* ptr, size_t size) {
        trackedAllocations.push_back(ptr);
        MemoryProfiler::GetInstance().TrackAllocation(ptr, size, tag);
    }

    KL_BEGIN_FIELDS(MemoryScope)
        /* No reflected fields. */
    KL_END_FIELDS

    KL_BEGIN_METHODS(MemoryScope)
        KL_METHOD_AUTO(MemoryScope, Track, "Track")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(MemoryScope)
        /* No reflected ctors - MemoryScope is engine-internal */
    KL_END_DESCRIBE(MemoryScope)

};

} // namespace koilo

// Convenience macros for memory tracking
#define KL_TRACK_ALLOC(ptr, size, tag) koilo::MemoryProfiler::GetInstance().TrackAllocation(ptr, size, tag)
#define KL_TRACK_FREE(ptr) koilo::MemoryProfiler::GetInstance().TrackFree(ptr)
#define KL_TRACK_NEW(type, ptr, tag) koilo::MemoryProfiler::GetInstance().TrackAllocation(static_cast<void*>(ptr), sizeof(type), tag)
#define KL_TRACK_DELETE(ptr) koilo::MemoryProfiler::GetInstance().TrackFree(ptr)
