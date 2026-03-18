// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file timemanager.hpp
 * @brief Central frame timing singleton - all subsystems pull dt from here.
 *
 * Follows the Unity Time.deltaTime pattern: one authoritative source of
 * frame timing that any class can query without needing dt passed through
 * method parameters.
 *
 * @date 22/02/2026
 * @author Coela Can't
 */

#pragma once

#include <cstdint>
#include <koilo/registry/reflect_macros.hpp>

namespace koilo {

/**
 * @class TimeManager
 * @brief Singleton providing frame-level timing to all engine subsystems.
 *
 * Call Tick(dt) once per frame from the engine loop. All other code
 * retrieves timing via GetDeltaTime() / GetTotalTime() / GetFrameCount().
 */
class TimeManager {
public:
    /// Return the active instance. If the kernel has installed one via
    /// SetInstance(), that is returned; otherwise falls back to a
    /// process-wide Meyer's singleton (useful in tests without a kernel).
    static TimeManager& GetInstance() {
        if (s_instance) return *s_instance;
        static TimeManager fallback;
        return fallback;
    }

    /// Install a kernel-owned instance as the active singleton.
    static void SetInstance(TimeManager* inst) { s_instance = inst; }
    static void ClearInstance()                { s_instance = nullptr; }

    TimeManager() = default;
    TimeManager(const TimeManager&) = delete;
    TimeManager& operator=(const TimeManager&) = delete;

    // Called once per frame by the engine to advance time.
    void Tick(float dt) {
        deltaTime_  = dt;
        totalTime_ += dt;
        ++frameCount_;
    }

    // Delta time for the current frame (seconds).
    float GetDeltaTime()  const { return deltaTime_; }

    // Frames per second derived from the current frame's delta time.
    float GetFPS() const { return deltaTime_ > 0.0f ? 1.0f / deltaTime_ : 0.0f; }

    // Total elapsed time since engine start (seconds).
    float GetTotalTime()  const { return totalTime_; }

    // Number of frames since engine start.
    uint64_t GetFrameCount() const { return frameCount_; }

    // Reset all counters (e.g. on script reload).
    void Reset() {
        deltaTime_  = 0.0f;
        totalTime_  = 0.0f;
        frameCount_ = 0;
    }

    KL_BEGIN_FIELDS(TimeManager)
    KL_END_FIELDS

    KL_BEGIN_METHODS(TimeManager)
        KL_METHOD_AUTO(TimeManager, GetDeltaTime,  "Current frame delta time"),
        KL_METHOD_AUTO(TimeManager, GetTotalTime,  "Total elapsed time"),
        KL_METHOD_AUTO(TimeManager, GetFrameCount, "Total frame count"),
        KL_METHOD_AUTO(TimeManager, GetFPS,        "Frames per second (1/deltaTime)")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(TimeManager)
    KL_END_DESCRIBE(TimeManager)

private:
    static inline TimeManager* s_instance = nullptr;

    float    deltaTime_  = 0.0f;
    float    totalTime_  = 0.0f;
    uint64_t frameCount_ = 0;
};

} // namespace koilo
