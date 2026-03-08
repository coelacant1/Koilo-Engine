// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file performanceprofiler.hpp
 * @brief Performance profiling and timing measurements.
 *
 * @date 11/10/2025
 * @author Coela
 */

#pragma once

#include <string>
#include <chrono>
#include <unordered_map>
#include <vector>
#include <koilo/registry/reflect_macros.hpp>

namespace koilo {

/**
 * @struct ProfileSample
 * @brief A single profiling measurement.
 */
struct ProfileSample {
    std::string name;                    ///< Sample name
    double duration;                     ///< Duration in milliseconds
    int callCount;                       ///< Number of times called
    double minDuration;                  ///< Minimum duration
    double maxDuration;                  ///< Maximum duration
    std::chrono::high_resolution_clock::time_point startTime;

    ProfileSample(const std::string& name = "")
        : name(name), duration(0.0), callCount(0),
          minDuration(std::numeric_limits<double>::max()),
          maxDuration(0.0) {}

    KL_BEGIN_FIELDS(ProfileSample)
        KL_FIELD(ProfileSample, name, "Name", 0, 0),
        KL_FIELD(ProfileSample, duration, "Duration", 0, 0),
        KL_FIELD(ProfileSample, callCount, "Call count", 0, 0)
    KL_END_FIELDS

    KL_BEGIN_METHODS(ProfileSample)
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(ProfileSample)
        KL_CTOR(ProfileSample, std::string)
    KL_END_DESCRIBE(ProfileSample)
};

/**
 * @struct ProfileFrame
 * @brief Profiling data for a single frame.
 */
struct ProfileFrame {
    int frameNumber;                     ///< Frame number
    double totalTime;                    ///< Total frame time (ms)
    std::unordered_map<std::string, ProfileSample> samples;

    ProfileFrame() : frameNumber(0), totalTime(0.0) {}

    KL_BEGIN_FIELDS(ProfileFrame)
        KL_FIELD(ProfileFrame, frameNumber, "Frame number", -2147483648, 2147483647)
    KL_END_FIELDS

    KL_BEGIN_METHODS(ProfileFrame)
        /* No reflected methods. */
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(ProfileFrame)
        /* No reflected ctors. */
    KL_END_DESCRIBE(ProfileFrame)

};

/**
 * @class PerformanceProfiler
 * @brief Performance profiling system for measuring execution time.
 *
 * The PerformanceProfiler allows you to:
 * - Time code sections
 * - Track per-frame performance
 * - Generate performance reports
 * - Identify bottlenecks
 */
class PerformanceProfiler {
private:
    static PerformanceProfiler* instance;

    bool enabled;                        ///< Is profiling enabled?
    int currentFrame;                    ///< Current frame number
    ProfileFrame currentFrameData;       ///< Current frame samples
    std::vector<ProfileFrame> history;   ///< Historical frame data
    int historySize;                     ///< Max frames to keep

    std::unordered_map<std::string, std::chrono::high_resolution_clock::time_point> activeTimers;

    // Frame timing
    std::chrono::high_resolution_clock::time_point frameStartTime;
    double frameDuration;                ///< Last frame duration (ms)

public:
    /**
     * @brief Constructor.
     */
    PerformanceProfiler();

    /**
     * @brief Destructor.
     */
    ~PerformanceProfiler();

    /**
     * @brief Gets the singleton instance.
     */
    static PerformanceProfiler& GetInstance();

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
     * @brief Sets the history size (number of frames to keep).
     */
    void SetHistorySize(int size) { historySize = size; }

    /**
     * @brief Gets the history size.
     */
    int GetHistorySize() const { return historySize; }

    // === Timing ===

    /**
     * @brief Starts timing a section.
     */
    void BeginSample(const std::string& name);

    /**
     * @brief Ends timing a section.
     */
    void EndSample(const std::string& name);

    // === Frame Management ===

    /**
     * @brief Marks the beginning of a frame.
     */
    void BeginFrame();

    /**
     * @brief Marks the end of a frame.
     */
    void EndFrame();

    /**
     * @brief Gets the current frame number.
     */
    int GetCurrentFrame() const { return currentFrame; }

    /**
     * @brief Gets the last frame duration in milliseconds.
     */
    double GetFrameDuration() const { return frameDuration; }

    /**
     * @brief Gets the current FPS.
     */
    double GetFPS() const { return frameDuration > 0 ? 1000.0 / frameDuration : 0.0; }

    // === Data Access ===

    /**
     * @brief Gets the current frame data.
     */
    const ProfileFrame& GetCurrentFrameData() const { return currentFrameData; }

    /**
     * @brief Gets a sample from the current frame.
     */
    const ProfileSample* GetSample(const std::string& name) const;

    /**
     * @brief Gets all samples from the current frame.
     */
    const std::unordered_map<std::string, ProfileSample>& GetAllSamples() const {
        return currentFrameData.samples;
    }

    /**
     * @brief Gets the frame history.
     */
    const std::vector<ProfileFrame>& GetHistory() const { return history; }

    // === Reporting ===

    /**
     * @brief Prints a performance report to console.
     */
    void PrintReport() const;

    /**
     * @brief Gets a performance report as a string.
     */
    std::string GetReportString() const;

    /**
     * @brief Clears all profiling data.
     */
    void Clear();

    // === Statistics ===

    /**
     * @brief Gets average duration for a sample over history.
     */
    double GetAverageDuration(const std::string& name) const;

    /**
     * @brief Gets minimum duration for a sample over history.
     */
    double GetMinDuration(const std::string& name) const;

    /**
     * @brief Gets maximum duration for a sample over history.
     */
    double GetMaxDuration(const std::string& name) const;

    KL_BEGIN_FIELDS(PerformanceProfiler)
        KL_FIELD(PerformanceProfiler, enabled, "Enabled", 0, 1),
        KL_FIELD(PerformanceProfiler, currentFrame, "Current frame", 0, 0),
        KL_FIELD(PerformanceProfiler, historySize, "History size", 0, 0)
    KL_END_FIELDS

    KL_BEGIN_METHODS(PerformanceProfiler)
        KL_METHOD_AUTO(PerformanceProfiler, SetEnabled, "Set enabled"),
        KL_METHOD_AUTO(PerformanceProfiler, IsEnabled, "Is enabled"),
        KL_METHOD_AUTO(PerformanceProfiler, BeginFrame, "Begin frame"),
        KL_METHOD_AUTO(PerformanceProfiler, EndFrame, "End frame"),
        KL_METHOD_AUTO(PerformanceProfiler, GetFPS, "Get FPS"),
        KL_METHOD_AUTO(PerformanceProfiler, PrintReport, "Print report"),
        KL_METHOD_AUTO(PerformanceProfiler, Clear, "Clear")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(PerformanceProfiler)
        KL_CTOR0(PerformanceProfiler)
    KL_END_DESCRIBE(PerformanceProfiler)
};

/**
 * @class PerfProfileScope
 * @brief RAII helper for automatic timing.
 *
 * Create a PerfProfileScope at the beginning of a function to automatically
 * time its execution.
 */
class PerfProfileScope {
private:
    std::string name;
    bool active;

public:
    /**
     * @brief Constructor - starts timing.
     */
    PerfProfileScope(const std::string& name)
        : name(name), active(PerformanceProfiler::GetInstance().IsEnabled()) {
        if (active) {
            PerformanceProfiler::GetInstance().BeginSample(name);
        }
    }

    /**
     * @brief Destructor - ends timing.
     */
    ~PerfProfileScope() {
        if (active) {
            PerformanceProfiler::GetInstance().EndSample(name);
        }
    }

    KL_BEGIN_FIELDS(PerfProfileScope)
        /* No reflected fields. */
    KL_END_FIELDS

    KL_BEGIN_METHODS(PerfProfileScope)
        /* No reflected methods. */
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(PerfProfileScope)
        /* No reflected ctors - PerfProfileScope is engine-internal */
    KL_END_DESCRIBE(PerfProfileScope)

};

} // namespace koilo

// Convenience macro for profiling
#define KL_PERF_SCOPE(name) koilo::PerfProfileScope __perfProfileScope##__LINE__(name)
#define KL_PERF_FUNCTION() KL_PERF_SCOPE(__FUNCTION__)
