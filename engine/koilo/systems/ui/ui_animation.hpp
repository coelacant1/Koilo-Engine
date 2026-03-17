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
 * @author Coela Can't
 */

#pragma once

#include <cstdint>
#include <cstddef>
#include <cmath>
#include <functional>
#include <vector>
#include "../../registry/reflect_macros.hpp"

namespace koilo {
namespace ui {

// -- Easing functions ------------------------------------------------

/// Available easing curve types.
enum class EaseType : uint8_t {
    Linear = 0,     ///< Constant speed
    EaseInQuad,     ///< Quadratic acceleration
    EaseOutQuad,    ///< Quadratic deceleration
    EaseInOutQuad,  ///< Quadratic acceleration then deceleration
    EaseInCubic,    ///< Cubic acceleration
    EaseOutCubic,   ///< Cubic deceleration
    EaseInOutCubic, ///< Cubic acceleration then deceleration
    EaseInElastic,  ///< Elastic spring-in effect
    EaseOutElastic, ///< Elastic spring-out effect
    EaseOutBounce,  ///< Bouncing deceleration
    Count           ///< Number of easing types (sentinel)
};

/** @brief Evaluate easing function for t in [0, 1]. @return Mapped value in [0, 1]. */
float EaseEvaluate(EaseType type, float t);

// -- Tween -----------------------------------------------------------

/// What property to animate on the target widget.
enum class TweenProperty : uint8_t {
    PositionX = 0,  ///< Horizontal position
    PositionY,      ///< Vertical position
    Width,          ///< Widget width
    Height,         ///< Widget height
    Opacity,        ///< Alpha opacity
    SliderValue,    ///< Slider current value
    PaddingTop,     ///< Top padding
    PaddingRight,   ///< Right padding
    PaddingBottom,  ///< Bottom padding
    PaddingLeft,    ///< Left padding
    Count           ///< Number of properties (sentinel)
};

/** @brief Callback invoked when a tween completes. */
using TweenCallback = std::function<void(int widgetIdx)>;

/** @class Tween @brief A single property animation instance. */
struct Tween {
    bool active      = false;                          ///< Whether this tween is in use
    int  widgetIdx   = -1;                             ///< Target widget index
    TweenProperty prop = TweenProperty::PositionX;     ///< Property being animated
    EaseType ease    = EaseType::Linear;               ///< Easing curve type

    float startValue = 0.0f;  ///< Value at t=0
    float endValue   = 0.0f;  ///< Value at t=1
    float duration   = 0.0f;  ///< Duration in seconds
    float elapsed    = 0.0f;  ///< Elapsed time in seconds
    float delay      = 0.0f;  ///< Delay before start in seconds

    bool  loop       = false;  ///< Restart on completion
    bool  pingPong   = false;  ///< Reverse direction on completion
    bool  forward    = true;   ///< Current direction for ping-pong

    TweenCallback onComplete;  ///< Completion callback

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

    /**
     * @brief Start a new tween.
     * @return Tween index, or -1 if the pool is full.
     */
    int Start(int widgetIdx, TweenProperty prop, float from, float to,
              float duration, EaseType ease = EaseType::Linear,
              float delay = 0.0f);

    /** @brief Cancel a tween by index. */
    void Cancel(int tweenIdx);

    /** @brief Cancel all tweens targeting a specific widget. */
    void CancelAll(int widgetIdx);

    /**
     * @brief Update all active tweens.
     *
     * The apply callback is invoked for each tween that produces a new value.
     * @param dt        Delta time in seconds.
     * @param apply     Callback: apply(widgetIdx, property, currentValue, userData).
     * @param userData  Opaque pointer forwarded to the apply callback.
     * @return Number of tweens that completed this frame.
     */
    using ApplyFn = void(*)(int widgetIdx, TweenProperty prop, float value, void* userData);
    int Update(float dt, ApplyFn apply, void* userData = nullptr);

    /** @brief Set loop and ping-pong behaviour on a tween. */
    void SetLoop(int tweenIdx, bool loop, bool pingPong = false);

    /** @brief Set the completion callback on a tween. */
    void SetOnComplete(int tweenIdx, TweenCallback cb);

    /** @brief Get the number of currently active tweens. */
    size_t ActiveCount() const { return activeCount_; }

    /** @brief Get the pool capacity. */
    size_t Capacity() const { return capacity_; }

    /** @brief Direct access to a tween slot (for testing). */
    const Tween* At(int idx) const;

private:
    std::vector<Tween> tweens_;  ///< Pre-allocated tween array
    size_t capacity_ = 0;        ///< Total pool capacity
    size_t activeCount_ = 0;     ///< Number of currently active tweens

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
