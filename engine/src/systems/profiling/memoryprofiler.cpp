#include <ptx/systems/profiling/memoryprofiler.hpp>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <algorithm>

namespace ptx {

MemoryProfiler* MemoryProfiler::instance = nullptr;

MemoryProfiler::MemoryProfiler()
    : enabled(true), currentFrame(0) {
    instance = this;
}

MemoryProfiler::~MemoryProfiler() {
    instance = nullptr;
}

MemoryProfiler& MemoryProfiler::GetInstance() {
    if (!instance) {
        static MemoryProfiler defaultInstance;
        instance = &defaultInstance;
    }
    return *instance;
}

// === Tracking ===

void MemoryProfiler::TrackAllocation(void* address, size_t size, const std::string& tag) {
    if (!enabled || !address) {
        return;
    }

    // Store allocation info
    allocations[address] = MemoryAllocation(address, size, tag, currentFrame);

    // Update statistics
    stats.totalAllocated += size;
    stats.currentUsage += size;
    stats.allocationCount++;
    stats.activeAllocations++;

    // Update peak
    if (stats.currentUsage > stats.peakUsage) {
        stats.peakUsage = stats.currentUsage;
    }

    // Update usage by tag
    usageByTag[tag] += size;
}

void MemoryProfiler::TrackFree(void* address) {
    if (!enabled || !address) {
        return;
    }

    auto it = allocations.find(address);
    if (it == allocations.end()) {
        return;  // Not tracked or already freed
    }

    const MemoryAllocation& alloc = it->second;

    // Update statistics
    stats.totalFreed += alloc.size;
    stats.currentUsage -= alloc.size;
    stats.freeCount++;
    stats.activeAllocations--;

    // Update usage by tag
    usageByTag[alloc.tag] -= alloc.size;
    if (usageByTag[alloc.tag] == 0) {
        usageByTag.erase(alloc.tag);
    }

    // Remove allocation
    allocations.erase(it);
}

// === Statistics ===

size_t MemoryProfiler::GetUsageByTag(const std::string& tag) const {
    auto it = usageByTag.find(tag);
    if (it != usageByTag.end()) {
        return it->second;
    }
    return 0;
}

// === Reporting ===

void MemoryProfiler::PrintReport() const {
    std::cout << GetReportString();
}

std::string MemoryProfiler::GetReportString() const {
    std::ostringstream oss;

    oss << "\n========================================\n";
    oss << "       Memory Profile Report\n";
    oss << "========================================\n";
    oss << "Current Usage:    " << FormatBytes(stats.currentUsage) << "\n";
    oss << "Peak Usage:       " << FormatBytes(stats.peakUsage) << "\n";
    oss << "Total Allocated:  " << FormatBytes(stats.totalAllocated) << "\n";
    oss << "Total Freed:      " << FormatBytes(stats.totalFreed) << "\n";
    oss << "Allocations:      " << stats.allocationCount << "\n";
    oss << "Frees:            " << stats.freeCount << "\n";
    oss << "Active:           " << stats.activeAllocations << "\n";
    oss << "----------------------------------------\n";

    if (!usageByTag.empty()) {
        oss << "Usage by Tag:\n";
        oss << "----------------------------------------\n";

        // Sort by size (descending)
        std::vector<std::pair<std::string, size_t>> sortedTags(usageByTag.begin(), usageByTag.end());
        std::sort(sortedTags.begin(), sortedTags.end(),
            [](const auto& a, const auto& b) { return a.second > b.second; });

        for (const auto& pair : sortedTags) {
            double percentage = stats.currentUsage > 0 ? (static_cast<double>(pair.second) / stats.currentUsage) * 100.0 : 0.0;
            oss << std::left << std::setw(30) << pair.first
                << std::right << std::setw(12) << FormatBytes(pair.second)
                << std::right << std::setw(7) << std::fixed << std::setprecision(1) << percentage << "%\n";
        }
    }

    oss << "========================================\n";
    return oss.str();
}

void MemoryProfiler::PrintLeaks() const {
    if (allocations.empty()) {
        std::cout << "No memory leaks detected.\n";
        return;
    }

    std::cout << "\n========================================\n";
    std::cout << "         Memory Leaks Detected\n";
    std::cout << "========================================\n";
    std::cout << "Total leaks: " << allocations.size() << "\n";
    std::cout << "Total size: " << FormatBytes(stats.currentUsage) << "\n";
    std::cout << "----------------------------------------\n";

    // Sort by size (descending)
    std::vector<const MemoryAllocation*> sortedLeaks;
    for (const auto& pair : allocations) {
        sortedLeaks.push_back(&pair.second);
    }
    std::sort(sortedLeaks.begin(), sortedLeaks.end(),
        [](const MemoryAllocation* a, const MemoryAllocation* b) {
            return a->size > b->size;
        });

    std::cout << std::left << std::setw(18) << "Address"
              << std::right << std::setw(12) << "Size"
              << std::right << std::setw(10) << "Frame"
              << "  Tag\n";
    std::cout << "----------------------------------------\n";

    for (const MemoryAllocation* leak : sortedLeaks) {
        std::cout << std::left << std::setw(18) << leak->address
            << std::right << std::setw(12) << FormatBytes(leak->size)
            << std::right << std::setw(10) << leak->frameNumber
            << "  " << leak->tag << "\n";
    }

    std::cout << "========================================\n";
}

void MemoryProfiler::Clear() {
    stats = MemoryStats();
    allocations.clear();
    usageByTag.clear();
}

// === Utility ===

std::string MemoryProfiler::FormatBytes(size_t bytes) {
    const char* units[] = { "B", "KB", "MB", "GB", "TB" };
    int unitIndex = 0;
    double size = static_cast<double>(bytes);

    while (size >= 1024.0 && unitIndex < 4) {
        size /= 1024.0;
        unitIndex++;
    }

    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << size << " " << units[unitIndex];
    return oss.str();
}

} // namespace ptx
