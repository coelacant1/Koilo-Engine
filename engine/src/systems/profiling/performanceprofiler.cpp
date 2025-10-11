#include <ptx/systems/profiling/performanceprofiler.hpp>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <algorithm>

namespace ptx {

PerformanceProfiler* PerformanceProfiler::instance = nullptr;

PerformanceProfiler::PerformanceProfiler()
    : enabled(true), currentFrame(0), historySize(60), frameDuration(0.0) {
    instance = this;
}

PerformanceProfiler::~PerformanceProfiler() {
    instance = nullptr;
}

PerformanceProfiler& PerformanceProfiler::GetInstance() {
    if (!instance) {
        static PerformanceProfiler defaultInstance;
        instance = &defaultInstance;
    }
    return *instance;
}

// === Timing ===

void PerformanceProfiler::BeginSample(const std::string& name) {
    if (!enabled) {
        return;
    }

    activeTimers[name] = std::chrono::high_resolution_clock::now();
}

void PerformanceProfiler::EndSample(const std::string& name) {
    if (!enabled) {
        return;
    }

    auto endTime = std::chrono::high_resolution_clock::now();

    auto it = activeTimers.find(name);
    if (it == activeTimers.end()) {
        return;  // Timer not started
    }

    auto startTime = it->second;
    std::chrono::duration<double, std::milli> duration = endTime - startTime;
    double durationMs = duration.count();

    // Update or create sample
    auto& sample = currentFrameData.samples[name];
    sample.name = name;
    sample.duration += durationMs;
    sample.callCount++;
    sample.minDuration = std::min(sample.minDuration, durationMs);
    sample.maxDuration = std::max(sample.maxDuration, durationMs);

    activeTimers.erase(it);
}

// === Frame Management ===

void PerformanceProfiler::BeginFrame() {
    if (!enabled) {
        return;
    }

    frameStartTime = std::chrono::high_resolution_clock::now();

    // Reset current frame data
    currentFrameData = ProfileFrame();
    currentFrameData.frameNumber = currentFrame;
}

void PerformanceProfiler::EndFrame() {
    if (!enabled) {
        return;
    }

    auto frameEndTime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> duration = frameEndTime - frameStartTime;
    frameDuration = duration.count();

    currentFrameData.totalTime = frameDuration;

    // Store in history
    history.push_back(currentFrameData);

    // Maintain history size
    if (history.size() > static_cast<size_t>(historySize)) {
        history.erase(history.begin());
    }

    currentFrame++;
}

// === Data Access ===

const ProfileSample* PerformanceProfiler::GetSample(const std::string& name) const {
    auto it = currentFrameData.samples.find(name);
    if (it != currentFrameData.samples.end()) {
        return &it->second;
    }
    return nullptr;
}

// === Reporting ===

void PerformanceProfiler::PrintReport() const {
    std::cout << GetReportString();
}

std::string PerformanceProfiler::GetReportString() const {
    std::ostringstream oss;

    oss << "\n========================================\n";
    oss << "     Performance Profile Report\n";
    oss << "========================================\n";
    oss << "Frame: " << currentFrame << "\n";
    oss << "Frame Time: " << std::fixed << std::setprecision(2) << frameDuration << " ms\n";
    oss << "FPS: " << std::fixed << std::setprecision(1) << GetFPS() << "\n";
    oss << "----------------------------------------\n";

    if (currentFrameData.samples.empty()) {
        oss << "No samples collected.\n";
    } else {
        oss << std::left << std::setw(30) << "Sample"
            << std::right << std::setw(10) << "Time (ms)"
            << std::right << std::setw(8) << "Calls"
            << std::right << std::setw(10) << "Avg (ms)"
            << std::right << std::setw(8) << "%\n";
        oss << "----------------------------------------\n";

        // Sort by duration (descending)
        std::vector<const ProfileSample*> sortedSamples;
        for (const auto& pair : currentFrameData.samples) {
            sortedSamples.push_back(&pair.second);
        }
        std::sort(sortedSamples.begin(), sortedSamples.end(),
            [](const ProfileSample* a, const ProfileSample* b) {
                return a->duration > b->duration;
            });

        for (const ProfileSample* sample : sortedSamples) {
            double avgDuration = sample->duration / sample->callCount;
            double percentage = (frameDuration > 0) ? (sample->duration / frameDuration) * 100.0 : 0.0;

            oss << std::left << std::setw(30) << sample->name
                << std::right << std::setw(10) << std::fixed << std::setprecision(2) << sample->duration
                << std::right << std::setw(8) << sample->callCount
                << std::right << std::setw(10) << std::fixed << std::setprecision(3) << avgDuration
                << std::right << std::setw(7) << std::fixed << std::setprecision(1) << percentage << "%\n";
        }
    }

    oss << "========================================\n";
    return oss.str();
}

void PerformanceProfiler::Clear() {
    currentFrameData = ProfileFrame();
    history.clear();
    activeTimers.clear();
    currentFrame = 0;
    frameDuration = 0.0;
}

// === Statistics ===

double PerformanceProfiler::GetAverageDuration(const std::string& name) const {
    if (history.empty()) {
        return 0.0;
    }

    double totalDuration = 0.0;
    int count = 0;

    for (const ProfileFrame& frame : history) {
        auto it = frame.samples.find(name);
        if (it != frame.samples.end()) {
            totalDuration += it->second.duration;
            count++;
        }
    }

    return count > 0 ? totalDuration / count : 0.0;
}

double PerformanceProfiler::GetMinDuration(const std::string& name) const {
    double minDuration = std::numeric_limits<double>::max();

    for (const ProfileFrame& frame : history) {
        auto it = frame.samples.find(name);
        if (it != frame.samples.end()) {
            minDuration = std::min(minDuration, it->second.minDuration);
        }
    }

    return minDuration != std::numeric_limits<double>::max() ? minDuration : 0.0;
}

double PerformanceProfiler::GetMaxDuration(const std::string& name) const {
    double maxDuration = 0.0;

    for (const ProfileFrame& frame : history) {
        auto it = frame.samples.find(name);
        if (it != frame.samples.end()) {
            maxDuration = std::max(maxDuration, it->second.maxDuration);
        }
    }

    return maxDuration;
}

} // namespace ptx
