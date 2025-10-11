/**
 * @file profiler.hpp
 * @brief Performance profiling system for measuring CPU and GPU time.
 *
 * @date 11/10/2025
 * @author Coela
 */

#pragma once

#include <chrono>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <thread>
#include <mutex>
#include "../../registry/reflect_macros.hpp"

namespace ptx {

/**
 * @struct ProfileResult
 * @brief Result of a single profiling measurement.
 */
struct ProfileResult {
    std::string name;
    std::thread::id threadId;
    double startTime;   // Microseconds since profiler start
    double duration;    // Microseconds
    int depth;          // Nesting depth

    PTX_BEGIN_FIELDS(ProfileResult)
        PTX_FIELD(ProfileResult, name, "Name", 0, 0),
        PTX_FIELD(ProfileResult, threadId, "Thread id", 0, 0),
        PTX_FIELD(ProfileResult, startTime, "Start time", __DBL_MIN__, __DBL_MAX__),
        PTX_FIELD(ProfileResult, duration, "Duration", __DBL_MIN__, __DBL_MAX__),
        PTX_FIELD(ProfileResult, depth, "Depth", -2147483648, 2147483647)
    PTX_END_FIELDS

    PTX_BEGIN_METHODS(ProfileResult)
        /* No reflected methods. */
    PTX_END_METHODS

    PTX_BEGIN_DESCRIBE(ProfileResult)
        /* No reflected ctors. */
    PTX_END_DESCRIBE(ProfileResult)

};

/**
 * @struct ProfileStats
 * @brief Statistical analysis of a profiled scope.
 */
struct ProfileStats {
    std::string name;
    int callCount;
    double totalTime;
    double minTime;
    double maxTime;
    double avgTime;

    ProfileStats()
        : name(""), callCount(0), totalTime(0.0), minTime(1e9), maxTime(0.0), avgTime(0.0) {}

    PTX_BEGIN_FIELDS(ProfileStats)
        PTX_FIELD(ProfileStats, name, "Name", 0, 0),
        PTX_FIELD(ProfileStats, callCount, "Call count", -2147483648, 2147483647),
        PTX_FIELD(ProfileStats, totalTime, "Total time", __DBL_MIN__, __DBL_MAX__),
        PTX_FIELD(ProfileStats, minTime, "Min time", __DBL_MIN__, __DBL_MAX__),
        PTX_FIELD(ProfileStats, maxTime, "Max time", __DBL_MIN__, __DBL_MAX__),
        PTX_FIELD(ProfileStats, avgTime, "Avg time", __DBL_MIN__, __DBL_MAX__)
    PTX_END_FIELDS

    PTX_BEGIN_METHODS(ProfileStats)
        /* No reflected methods. */
    PTX_END_METHODS

    PTX_BEGIN_DESCRIBE(ProfileStats)
        PTX_CTOR0(ProfileStats)
    PTX_END_DESCRIBE(ProfileStats)

};

/**
 * @class Profiler
 * @brief Central profiling system for performance measurement.
 */
class Profiler {
private:
    // Singleton instance
    static Profiler* instance;

    // Profiling state
    bool enabled;
    std::chrono::high_resolution_clock::time_point startTime;

    // Current frame results
    std::vector<ProfileResult> results;

    // Statistics per scope name
    std::unordered_map<std::string, ProfileStats> stats;

    // Current depth (for hierarchical scopes)
    int currentDepth;

    // Thread safety
    std::mutex mutex;

    // Frame timing
    double frameStartTime;
    double lastFrameTime;
    double fps;
    int frameCount;

    Profiler();

public:
    /**
     * @brief Gets the singleton instance.
     */
    static Profiler& GetInstance();

    /**
     * @brief Enables profiling.
     */
    void Enable() { enabled = true; }

    /**
     * @brief Disables profiling.
     */
    void Disable() { enabled = false; }

    /**
     * @brief Checks if profiling is enabled.
     */
    bool IsEnabled() const { return enabled; }

    /**
     * @brief Begins a new frame.
     */
    void BeginFrame();

    /**
     * @brief Ends the current frame.
     */
    void EndFrame();

    /**
     * @brief Begins a profiling scope.
     * @param name The name of the scope.
     */
    void BeginScope(const std::string& name);

    /**
     * @brief Ends the current profiling scope.
     * @param name The name of the scope (for validation).
     */
    void EndScope(const std::string& name);

    /**
     * @brief Gets the results from the last frame.
     */
    const std::vector<ProfileResult>& GetResults() const { return results; }

    /**
     * @brief Gets statistics for a specific scope.
     * @param name The scope name.
     * @return The statistics, or nullptr if not found.
     */
    const ProfileStats* GetStats(const std::string& name) const;

    /**
     * @brief Gets all statistics.
     */
    const std::unordered_map<std::string, ProfileStats>& GetAllStats() const { return stats; }

    /**
     * @brief Clears all statistics.
     */
    void ClearStats();

    /**
     * @brief Gets the last frame time in milliseconds.
     */
    double GetLastFrameTime() const { return lastFrameTime; }

    /**
     * @brief Gets the current FPS.
     */
    double GetFPS() const { return fps; }

    /**
     * @brief Gets the frame count since profiler start.
     */
    int GetFrameCount() const { return frameCount; }

    /**
     * @brief Exports results to JSON format.
     * @param filepath Path to write the JSON file.
     * @return True if export succeeded.
     */
    bool ExportJSON(const std::string& filepath);

    /**
     * @brief Exports results to CSV format.
     * @param filepath Path to write the CSV file.
     * @return True if export succeeded.
     */
    bool ExportCSV(const std::string& filepath);

    /**
     * @brief Exports results to Chrome Tracing format.
     * @param filepath Path to write the trace file.
     * @return True if export succeeded.
     */
    bool ExportChromeTrace(const std::string& filepath);

    /**
     * @brief Prints statistics to console.
     */
    void PrintStats();

private:
    /**
     * @brief Gets the current time in microseconds since profiler start.
     */
    double GetCurrentTime() const;

    PTX_BEGIN_FIELDS(Profiler)
        PTX_FIELD(Profiler, enabled, "Enabled", 0, 1),
        PTX_FIELD(Profiler, fps, "FPS", 0.0, 1000.0),
        PTX_FIELD(Profiler, frameCount, "Frame count", 0, 1000000)
    PTX_END_FIELDS

    PTX_BEGIN_METHODS(Profiler)
        PTX_METHOD_AUTO(Profiler, Enable, "Enable"),
        PTX_METHOD_AUTO(Profiler, Disable, "Disable"),
        PTX_METHOD_AUTO(Profiler, IsEnabled, "Is enabled"),
        PTX_METHOD_AUTO(Profiler, BeginFrame, "Begin frame"),
        PTX_METHOD_AUTO(Profiler, EndFrame, "End frame"),
        PTX_METHOD_AUTO(Profiler, GetLastFrameTime, "Get last frame time"),
        PTX_METHOD_AUTO(Profiler, GetFPS, "Get FPS"),
        PTX_METHOD_AUTO(Profiler, ClearStats, "Clear stats"),
        PTX_METHOD_AUTO(Profiler, PrintStats, "Print stats")
    PTX_END_METHODS

    PTX_BEGIN_DESCRIBE(Profiler)
        // Singleton, no constructors exposed
    PTX_END_DESCRIBE(Profiler)
};

/**
 * @class ProfileScope
 * @brief RAII helper for profiling scopes.
 */
class ProfileScope {
private:
    std::string name;
    bool active;

public:
    /**
     * @brief Constructor that begins a scope.
     * @param name The scope name.
     */
    explicit ProfileScope(const std::string& name);

    /**
     * @brief Destructor that ends the scope.
     */
    ~ProfileScope();

    // Delete copy/move
    ProfileScope(const ProfileScope&) = delete;
    ProfileScope& operator=(const ProfileScope&) = delete;

    PTX_BEGIN_FIELDS(ProfileScope)
        /* No reflected fields. */
    PTX_END_FIELDS

    PTX_BEGIN_METHODS(ProfileScope)
        /* No reflected methods. */
    PTX_END_METHODS

    PTX_BEGIN_DESCRIBE(ProfileScope)
        PTX_CTOR(ProfileScope, const std::string &),
        PTX_CTOR(ProfileScope, const ProfileScope &)
    PTX_END_DESCRIBE(ProfileScope)

};

} // namespace ptx

// Profiling macros
#ifdef PTX_ENABLE_PROFILING
    #define PTX_PROFILE_SCOPE(name) ptx::ProfileScope __profile_scope##__LINE__(name)
    #define PTX_PROFILE_FUNCTION() PTX_PROFILE_SCOPE(__FUNCTION__)
    #define PTX_PROFILE_BEGIN_FRAME() ptx::Profiler::GetInstance().BeginFrame()
    #define PTX_PROFILE_END_FRAME() ptx::Profiler::GetInstance().EndFrame()
#else
    #define PTX_PROFILE_SCOPE(name)
    #define PTX_PROFILE_FUNCTION()
    #define PTX_PROFILE_BEGIN_FRAME()
    #define PTX_PROFILE_END_FRAME()
#endif
