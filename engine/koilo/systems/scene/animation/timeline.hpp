// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file timeline.hpp
 * @brief Timeline controller for sequencing events and animations.
 *
 * @date TBD
 * @author Coela
 */

#pragma once

#include <koilo/registry/reflect_macros.hpp>
#include <koilo/core/time/timemanager.hpp>

namespace koilo {

/**
 * @class Timeline
 * @brief Controls time-based event sequencing.
 */
class Timeline {
protected:
    float currentTime = 0.0f;
    float duration = 1.0f;
    bool playing = false;
    bool looping = false;

public:
    Timeline() = default;
    Timeline(float duration) : duration(duration) {}
    virtual ~Timeline() = default;

    void Play() { playing = true; }
    void Stop() { playing = false; currentTime = 0.0f; }
    void Pause() { playing = false; }
    void Resume() { playing = true; }
    
    void Update() { if (playing) currentTime += TimeManager::GetInstance().GetDeltaTime(); }
    
    bool IsPlaying() const { return playing; }
    void SetLooping(bool loop) { looping = loop; }
    bool IsLooping() const { return looping; }
    
    void SetDuration(float dur) { duration = dur; }
    float GetDuration() const { return duration; }
    
    void SetCurrentTime(float time) { currentTime = time; }
    float GetCurrentTime() const { return currentTime; }

    KL_BEGIN_FIELDS(Timeline)
        KL_FIELD(Timeline, currentTime, "Current time", 0.0f, __FLT_MAX__),
        KL_FIELD(Timeline, duration, "Duration", 0.0f, __FLT_MAX__),
        KL_FIELD(Timeline, playing, "Playing", 0, 1),
        KL_FIELD(Timeline, looping, "Looping", 0, 1)
    KL_END_FIELDS

    KL_BEGIN_METHODS(Timeline)
        KL_METHOD_AUTO(Timeline, Play, "Play"),
        KL_METHOD_AUTO(Timeline, Stop, "Stop"),
        KL_METHOD_AUTO(Timeline, Pause, "Pause"),
        KL_METHOD_AUTO(Timeline, Resume, "Resume"),
        KL_METHOD_AUTO(Timeline, Update, "Update"),
        KL_METHOD_AUTO(Timeline, IsPlaying, "Is playing"),
        KL_METHOD_AUTO(Timeline, SetLooping, "Set looping"),
        KL_METHOD_AUTO(Timeline, IsLooping, "Is looping"),
        KL_METHOD_AUTO(Timeline, SetDuration, "Set duration"),
        KL_METHOD_AUTO(Timeline, GetDuration, "Get duration"),
        KL_METHOD_AUTO(Timeline, SetCurrentTime, "Set current time"),
        KL_METHOD_AUTO(Timeline, GetCurrentTime, "Get current time")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(Timeline)
        KL_CTOR0(Timeline),
        KL_CTOR(Timeline, float)
    KL_END_DESCRIBE(Timeline)
};

} // namespace koilo
