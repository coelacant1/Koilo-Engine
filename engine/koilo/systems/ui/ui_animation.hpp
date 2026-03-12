// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file ui_animation.hpp
 * @brief Property animation system for UI widgets.
 *
 * Tweens numeric widget properties over time with easing functions.
 * Pre-allocated tween pool - zero per-frame heap allocation.
 * Target: < 0.02ms for 100 active tweens.
 *
 * @date 03/09/2026
 * @author Coela
 */

#pragma once

#include <cstdint>
#include <cstddef>
#include <cmath>
#include <functional>
#include "../../registry/reflect_macros.hpp"

namespace koilo {
namespace ui {

// -- Easing functions ------------------------------------------------

enum class EaseType : uint8_t {
    Linear = 0,
    EaseInQuad,
    EaseOutQuad,
    EaseInOutQuad,
    EaseInCubic,
    EaseOutCubic,
    EaseInOutCubic,
    EaseInElastic,
    EaseOutElastic,
    EaseOutBounce,
    Count
};

/// Evaluate easing function for t in [0, 1]. Returns mapped value in [0, 1].
float EaseEvaluate(EaseType type, float t);

// -- Tween -----------------------------------------------------------

/// What property to animate on the target widget.
enum class TweenProperty : uint8_t {
    PositionX = 0,
    PositionY,
    Width,
    Height,
    Opacity,
    SliderValue,
    PaddingTop,
    PaddingRight,
    PaddingBottom,
    PaddingLeft,
    Count
};

/// Callback when a tween completes.
using TweenCallback = std::function<void(int widgetIdx)>;

struct Tween {
    bool active      = false;
    int  widgetIdx   = -1;
    TweenProperty prop = TweenProperty::PositionX;
    EaseType ease    = EaseType::Linear;

    float startValue = 0.0f;
    float endValue   = 0.0f;
    float duration   = 0.0f;  // seconds
    float elapsed    = 0.0f;
    float delay      = 0.0f;  // seconds before start

    bool  loop       = false;
    bool  pingPong   = false;
    bool  forward    = true;   // current direction for ping-pong

    TweenCallback onComplete;

    KL_BEGIN_FIELDS(Tween)
        KL_FIELD(Tween, active, "Active", 0, 1),
        KL_FIELD(Tween, widgetIdx, "Widget index", -1, 65535),
        KL_FIELD(Tween, startValue, "Start value", 0, 0),
        KL_FIELD(Tween, endValue, "End value", 0, 0),
        KL_FIELD(Tween, duration, "Duration", 0, 0),
        KL_FIELD(Tween, elapsed, "Elapsed", 0, 0),
        KL_FIELD(Tween, delay, "Delay", 0, 0),
        KL_FIELD(Tween, loop, "Loop", 0, 1),
        KL_FIELD(Tween, pingPong, "Ping pong", 0, 1),
        KL_FIELD(Tween, forward, "Forward", 0, 1)
    KL_END_FIELDS

    KL_BEGIN_METHODS(Tween)
        /* No reflected methods. */
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(Tween)
        /* No reflected ctors. */
    KL_END_DESCRIBE(Tween)

};

// -- TweenPool -------------------------------------------------------

/**
 * @class TweenPool
 * @brief Pre-allocated pool of tweens with O(1) allocation.
 */
class TweenPool {
public:
    static constexpr size_t DEFAULT_CAPACITY = 256;

    explicit TweenPool(size_t capacity = DEFAULT_CAPACITY);
    ~TweenPool();

    /// Start a new tween. Returns tween index, or -1 if pool full.
    int Start(int widgetIdx, TweenProperty prop, float from, float to,
              float duration, EaseType ease = EaseType::Linear,
              float delay = 0.0f);

    /// Cancel a tween by index.
    void Cancel(int tweenIdx);

    /// Cancel all tweens on a widget.
    void CancelAll(int widgetIdx);

    /// Update all active tweens. Returns the number of completed tweens.
    /// The apply callback is invoked for each tween that produces a new value.
    /// Signature: apply(widgetIdx, property, currentValue)
    using ApplyFn = void(*)(int widgetIdx, TweenProperty prop, float value, void* userData);
    int Update(float dt, ApplyFn apply, void* userData = nullptr);

    /// Set loop/ping-pong on a tween.
    void SetLoop(int tweenIdx, bool loop, bool pingPong = false);

    /// Set completion callback on a tween.
    void SetOnComplete(int tweenIdx, TweenCallback cb);

    /// Number of active tweens.
    size_t ActiveCount() const { return activeCount_; }

    /// Pool capacity.
    size_t Capacity() const { return capacity_; }

    /// Direct access for testing.
    const Tween* At(int idx) const;

private:
    Tween* tweens_   = nullptr;
    size_t capacity_ = 0;
    size_t activeCount_ = 0;

    KL_BEGIN_FIELDS(TweenPool)
        /* No reflected fields. */
    KL_END_FIELDS

    KL_BEGIN_METHODS(TweenPool)
        KL_METHOD_AUTO(TweenPool, Update, "Update"),
        KL_METHOD_AUTO(TweenPool, Capacity, "Capacity"),
        KL_METHOD_AUTO(TweenPool, At, "At")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(TweenPool)
        KL_CTOR0(TweenPool)
    KL_END_DESCRIBE(TweenPool)

};

} // namespace ui
} // namespace koilo
