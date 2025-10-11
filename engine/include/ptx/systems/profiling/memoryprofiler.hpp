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
#include "../../registry/reflect_macros.hpp"

namespace ptx {

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

    PTX_BEGIN_FIELDS(MemoryStats)
        PTX_FIELD(MemoryStats, totalAllocated, "Total allocated", 0, 0),
        PTX_FIELD(MemoryStats, currentUsage, "Current usage", 0, 0),
        PTX_FIELD(MemoryStats, peakUsage, "Peak usage", 0, 0),
        PTX_FIELD(MemoryStats, allocationCount, "Allocation count", 0, 0)
    PTX_END_FIELDS

    PTX_BEGIN_METHODS(MemoryStats)
    PTX_END_METHODS

    PTX_BEGIN_DESCRIBE(MemoryStats)
        PTX_CTOR0(MemoryStats)
    PTX_END_DESCRIBE(MemoryStats)
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

    /**
     * @brief Tracks an allocation with automatic tagging.
     */
    template<typename T>
    void TrackAllocation(T* ptr, size_t count = 1, const std::string& tag = "");

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

    PTX_BEGIN_FIELDS(MemoryProfiler)
        PTX_FIELD(MemoryProfiler, enabled, "Enabled", 0, 1),
        PTX_FIELD(MemoryProfiler, currentFrame, "Current frame", 0, 0)
    PTX_END_FIELDS

    PTX_BEGIN_METHODS(MemoryProfiler)
        PTX_METHOD_AUTO(MemoryProfiler, SetEnabled, "Set enabled"),
        PTX_METHOD_AUTO(MemoryProfiler, IsEnabled, "Is enabled"),
        PTX_METHOD_AUTO(MemoryProfiler, GetStats, "Get stats"),
        PTX_METHOD_AUTO(MemoryProfiler, PrintReport, "Print report"),
        PTX_METHOD_AUTO(MemoryProfiler, PrintLeaks, "Print leaks"),
        PTX_METHOD_AUTO(MemoryProfiler, Clear, "Clear")
    PTX_END_METHODS

    PTX_BEGIN_DESCRIBE(MemoryProfiler)
        PTX_CTOR0(MemoryProfiler)
    PTX_END_DESCRIBE(MemoryProfiler)
};

// === Template Implementations ===

template<typename T>
void MemoryProfiler::TrackAllocation(T* ptr, size_t count, const std::string& tag) {
    if (!enabled || !ptr) {
        return;
    }

    size_t size = sizeof(T) * count;
    std::string finalTag = tag.empty() ? typeid(T).name() : tag;
    TrackAllocation(static_cast<void*>(ptr), size, finalTag);
}

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
};

} // namespace ptx

// Convenience macros for memory tracking
#define PTX_TRACK_ALLOC(ptr, size, tag) ptx::MemoryProfiler::GetInstance().TrackAllocation(ptr, size, tag)
#define PTX_TRACK_FREE(ptr) ptx::MemoryProfiler::GetInstance().TrackFree(ptr)
#define PTX_TRACK_NEW(type, ptr, tag) ptx::MemoryProfiler::GetInstance().TrackAllocation<type>(ptr, 1, tag)
#define PTX_TRACK_DELETE(ptr) ptx::MemoryProfiler::GetInstance().TrackFree(ptr)
