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
#include <memory>
#include "../../registry/reflect_macros.hpp"

namespace ptx {

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

    PTX_BEGIN_FIELDS(ProfileSample)
        PTX_FIELD(ProfileSample, name, "Name", 0, 0),
        PTX_FIELD(ProfileSample, duration, "Duration", 0, 0),
        PTX_FIELD(ProfileSample, callCount, "Call count", 0, 0)
    PTX_END_FIELDS

    PTX_BEGIN_METHODS(ProfileSample)
    PTX_END_METHODS

    PTX_BEGIN_DESCRIBE(ProfileSample)
        PTX_CTOR(ProfileSample, std::string)
    PTX_END_DESCRIBE(ProfileSample)
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

    /**
     * @brief Times a function call.
     */
    template<typename Func>
    double TimeFunction(const std::string& name, Func func);

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

    PTX_BEGIN_FIELDS(PerformanceProfiler)
        PTX_FIELD(PerformanceProfiler, enabled, "Enabled", 0, 1),
        PTX_FIELD(PerformanceProfiler, currentFrame, "Current frame", 0, 0),
        PTX_FIELD(PerformanceProfiler, historySize, "History size", 0, 0)
    PTX_END_FIELDS

    PTX_BEGIN_METHODS(PerformanceProfiler)
        PTX_METHOD_AUTO(PerformanceProfiler, SetEnabled, "Set enabled"),
        PTX_METHOD_AUTO(PerformanceProfiler, IsEnabled, "Is enabled"),
        PTX_METHOD_AUTO(PerformanceProfiler, BeginFrame, "Begin frame"),
        PTX_METHOD_AUTO(PerformanceProfiler, EndFrame, "End frame"),
        PTX_METHOD_AUTO(PerformanceProfiler, GetFPS, "Get FPS"),
        PTX_METHOD_AUTO(PerformanceProfiler, PrintReport, "Print report"),
        PTX_METHOD_AUTO(PerformanceProfiler, Clear, "Clear")
    PTX_END_METHODS

    PTX_BEGIN_DESCRIBE(PerformanceProfiler)
        PTX_CTOR0(PerformanceProfiler)
    PTX_END_DESCRIBE(PerformanceProfiler)
};

/**
 * @class ProfileScope
 * @brief RAII helper for automatic timing.
 *
 * Create a ProfileScope at the beginning of a function to automatically
 * time its execution.
 */
class ProfileScope {
private:
    std::string name;
    bool active;

public:
    /**
     * @brief Constructor - starts timing.
     */
    ProfileScope(const std::string& name)
        : name(name), active(PerformanceProfiler::GetInstance().IsEnabled()) {
        if (active) {
            PerformanceProfiler::GetInstance().BeginSample(name);
        }
    }

    /**
     * @brief Destructor - ends timing.
     */
    ~ProfileScope() {
        if (active) {
            PerformanceProfiler::GetInstance().EndSample(name);
        }
    }
};

// === Template Implementations ===

template<typename Func>
double PerformanceProfiler::TimeFunction(const std::string& name, Func func) {
    if (!enabled) {
        func();
        return 0.0;
    }

    auto start = std::chrono::high_resolution_clock::now();
    func();
    auto end = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double, std::milli> duration = end - start;
    return duration.count();
}

} // namespace ptx

// Convenience macro for profiling
#define PTX_PROFILE_SCOPE(name) ptx::ProfileScope __profileScope##__LINE__(name)
#define PTX_PROFILE_FUNCTION() PTX_PROFILE_SCOPE(__FUNCTION__)
