#include <ptx/debug/profiler.hpp>
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iomanip>

namespace ptx {

// Static member initialization
Profiler* Profiler::instance = nullptr;

// === Profiler Implementation ===

Profiler::Profiler()
    : enabled(false), currentDepth(0), frameStartTime(0.0), lastFrameTime(0.0),
      fps(0.0), frameCount(0) {
    startTime = std::chrono::high_resolution_clock::now();
}

Profiler& Profiler::GetInstance() {
    if (instance == nullptr) {
        instance = new Profiler();
    }
    return *instance;
}

void Profiler::BeginFrame() {
    if (!enabled) return;

    std::lock_guard<std::mutex> lock(mutex);

    frameStartTime = GetCurrentTime();
    currentDepth = 0;
    results.clear();
}

void Profiler::EndFrame() {
    if (!enabled) return;

    std::lock_guard<std::mutex> lock(mutex);

    double frameEndTime = GetCurrentTime();
    lastFrameTime = (frameEndTime - frameStartTime) / 1000.0;  // Convert to milliseconds

    // Calculate FPS (exponential moving average)
    if (lastFrameTime > 0.0) {
        double instantFPS = 1000.0 / lastFrameTime;
        fps = (fps * 0.9) + (instantFPS * 0.1);  // Smooth FPS
    }

    frameCount++;
}

void Profiler::BeginScope(const std::string& name) {
    if (!enabled) return;

    std::lock_guard<std::mutex> lock(mutex);

    // Create a temporary result to track start time
    ProfileResult result;
    result.name = name;
    result.threadId = std::this_thread::get_id();
    result.startTime = GetCurrentTime();
    result.depth = currentDepth;
    result.duration = 0.0;

    results.push_back(result);
    currentDepth++;
}

void Profiler::EndScope(const std::string& name) {
    if (!enabled) return;

    std::lock_guard<std::mutex> lock(mutex);

    double endTime = GetCurrentTime();

    // Find the most recent scope with matching name
    for (auto it = results.rbegin(); it != results.rend(); ++it) {
        if (it->name == name && it->duration == 0.0) {
            it->duration = endTime - it->startTime;

            // Update statistics
            auto& stat = stats[name];
            stat.name = name;
            stat.callCount++;
            stat.totalTime += it->duration;
            stat.minTime = std::min(stat.minTime, it->duration);
            stat.maxTime = std::max(stat.maxTime, it->duration);
            stat.avgTime = stat.totalTime / stat.callCount;

            currentDepth--;
            break;
        }
    }
}

const ProfileStats* Profiler::GetStats(const std::string& name) const {
    auto it = stats.find(name);
    if (it != stats.end()) {
        return &it->second;
    }
    return nullptr;
}

void Profiler::ClearStats() {
    std::lock_guard<std::mutex> lock(mutex);
    stats.clear();
    results.clear();
    frameCount = 0;
}

double Profiler::GetCurrentTime() const {
    auto now = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(now - startTime);
    return static_cast<double>(duration.count());
}

bool Profiler::ExportJSON(const std::string& filepath) {
    std::lock_guard<std::mutex> lock(mutex);

    std::ofstream file(filepath);
    if (!file.is_open()) {
        return false;
    }

    file << "{\n";
    file << "  \"frames\": [\n";

    // Write frame results
    for (size_t i = 0; i < results.size(); ++i) {
        const auto& result = results[i];
        file << "    {\n";
        file << "      \"name\": \"" << result.name << "\",\n";
        file << "      \"startTime\": " << result.startTime << ",\n";
        file << "      \"duration\": " << result.duration << ",\n";
        file << "      \"depth\": " << result.depth << "\n";
        file << "    }";
        if (i < results.size() - 1) file << ",";
        file << "\n";
    }

    file << "  ],\n";
    file << "  \"stats\": [\n";

    // Write statistics
    size_t statCount = 0;
    for (const auto& pair : stats) {
        const auto& stat = pair.second;
        file << "    {\n";
        file << "      \"name\": \"" << stat.name << "\",\n";
        file << "      \"callCount\": " << stat.callCount << ",\n";
        file << "      \"totalTime\": " << stat.totalTime << ",\n";
        file << "      \"minTime\": " << stat.minTime << ",\n";
        file << "      \"maxTime\": " << stat.maxTime << ",\n";
        file << "      \"avgTime\": " << stat.avgTime << "\n";
        file << "    }";
        if (++statCount < stats.size()) file << ",";
        file << "\n";
    }

    file << "  ]\n";
    file << "}\n";

    file.close();
    return true;
}

bool Profiler::ExportCSV(const std::string& filepath) {
    std::lock_guard<std::mutex> lock(mutex);

    std::ofstream file(filepath);
    if (!file.is_open()) {
        return false;
    }

    // Write header
    file << "Name,CallCount,TotalTime(us),MinTime(us),MaxTime(us),AvgTime(us)\n";

    // Write statistics
    for (const auto& pair : stats) {
        const auto& stat = pair.second;
        file << stat.name << ","
             << stat.callCount << ","
             << stat.totalTime << ","
             << stat.minTime << ","
             << stat.maxTime << ","
             << stat.avgTime << "\n";
    }

    file.close();
    return true;
}

bool Profiler::ExportChromeTrace(const std::string& filepath) {
    std::lock_guard<std::mutex> lock(mutex);

    std::ofstream file(filepath);
    if (!file.is_open()) {
        return false;
    }

    file << "[\n";

    // Write trace events in Chrome Tracing format
    for (size_t i = 0; i < results.size(); ++i) {
        const auto& result = results[i];

        // Begin event
        file << "  {\"name\": \"" << result.name << "\", "
             << "\"cat\": \"function\", "
             << "\"ph\": \"B\", "
             << "\"ts\": " << result.startTime << ", "
             << "\"pid\": 1, "
             << "\"tid\": " << std::hash<std::thread::id>{}(result.threadId)
             << "},\n";

        // End event
        file << "  {\"name\": \"" << result.name << "\", "
             << "\"cat\": \"function\", "
             << "\"ph\": \"E\", "
             << "\"ts\": " << (result.startTime + result.duration) << ", "
             << "\"pid\": 1, "
             << "\"tid\": " << std::hash<std::thread::id>{}(result.threadId)
             << "}";

        if (i < results.size() - 1) file << ",";
        file << "\n";
    }

    file << "]\n";

    file.close();
    return true;
}

void Profiler::PrintStats() {
    std::lock_guard<std::mutex> lock(mutex);

    std::cout << "\n=== Profiler Statistics ===\n";
    std::cout << "Frame Count: " << frameCount << "\n";
    std::cout << "Last Frame Time: " << std::fixed << std::setprecision(2) << lastFrameTime << " ms\n";
    std::cout << "FPS: " << std::fixed << std::setprecision(1) << fps << "\n";
    std::cout << "\n";

    // Sort stats by total time (descending)
    std::vector<ProfileStats> sortedStats;
    for (const auto& pair : stats) {
        sortedStats.push_back(pair.second);
    }
    std::sort(sortedStats.begin(), sortedStats.end(),
        [](const ProfileStats& a, const ProfileStats& b) {
            return a.totalTime > b.totalTime;
        });

    // Print table header
    std::cout << std::left << std::setw(40) << "Scope Name"
              << std::right << std::setw(10) << "Calls"
              << std::setw(12) << "Total(ms)"
              << std::setw(12) << "Avg(us)"
              << std::setw(12) << "Min(us)"
              << std::setw(12) << "Max(us)" << "\n";
    std::cout << std::string(98, '-') << "\n";

    // Print stats
    for (const auto& stat : sortedStats) {
        std::cout << std::left << std::setw(40) << stat.name
                  << std::right << std::setw(10) << stat.callCount
                  << std::setw(12) << std::fixed << std::setprecision(3) << (stat.totalTime / 1000.0)
                  << std::setw(12) << std::fixed << std::setprecision(2) << stat.avgTime
                  << std::setw(12) << std::fixed << std::setprecision(2) << stat.minTime
                  << std::setw(12) << std::fixed << std::setprecision(2) << stat.maxTime << "\n";
    }

    std::cout << "\n";
}

// === ProfileScope Implementation ===

ProfileScope::ProfileScope(const std::string& name)
    : name(name), active(Profiler::GetInstance().IsEnabled()) {
    if (active) {
        Profiler::GetInstance().BeginScope(name);
    }
}

ProfileScope::~ProfileScope() {
    if (active) {
        Profiler::GetInstance().EndScope(name);
    }
}

} // namespace ptx
