// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file ui_animation.cpp
 * @brief UI animation system implementation.
 * @date 03/09/2026
 * @author Coela
 */

#include "ui_animation.hpp"
#include <algorithm>

namespace koilo {
namespace ui {

// -- Easing functions ------------------------------------------------

static float EaseLinear(float t) { return t; }

static float EaseInQuad(float t) { return t * t; }
static float EaseOutQuad(float t) { return t * (2.0f - t); }
static float EaseInOutQuad(float t) {
    return t < 0.5f ? 2.0f * t * t : -1.0f + (4.0f - 2.0f * t) * t;
}

static float EaseInCubic(float t) { return t * t * t; }
static float EaseOutCubic(float t) { float u = t - 1.0f; return u * u * u + 1.0f; }
static float EaseInOutCubic(float t) {
    return t < 0.5f ? 4.0f * t * t * t
                    : (t - 1.0f) * (2.0f * t - 2.0f) * (2.0f * t - 2.0f) + 1.0f;
}

static float EaseInElastic(float t) {
    if (t <= 0.0f) return 0.0f;
    if (t >= 1.0f) return 1.0f;
    return -std::pow(2.0f, 10.0f * (t - 1.0f)) *
           std::sin((t - 1.1f) * 5.0f * 3.14159265f);
}

static float EaseOutElastic(float t) {
    if (t <= 0.0f) return 0.0f;
    if (t >= 1.0f) return 1.0f;
    return std::pow(2.0f, -10.0f * t) *
           std::sin((t - 0.1f) * 5.0f * 3.14159265f) + 1.0f;
}

static float EaseOutBounce(float t) {
    if (t < 1.0f / 2.75f) {
        return 7.5625f * t * t;
    } else if (t < 2.0f / 2.75f) {
        t -= 1.5f / 2.75f;
        return 7.5625f * t * t + 0.75f;
    } else if (t < 2.5f / 2.75f) {
        t -= 2.25f / 2.75f;
        return 7.5625f * t * t + 0.9375f;
    } else {
        t -= 2.625f / 2.75f;
        return 7.5625f * t * t + 0.984375f;
    }
}

using EaseFn = float(*)(float);
static const EaseFn kEaseFns[static_cast<int>(EaseType::Count)] = {
    EaseLinear,
    EaseInQuad, EaseOutQuad, EaseInOutQuad,
    EaseInCubic, EaseOutCubic, EaseInOutCubic,
    EaseInElastic, EaseOutElastic,
    EaseOutBounce
};

float EaseEvaluate(EaseType type, float t) {
    int idx = static_cast<int>(type);
    if (idx < 0 || idx >= static_cast<int>(EaseType::Count)) return t;
    t = std::max(0.0f, std::min(1.0f, t));
    return kEaseFns[idx](t);
}

// -- TweenPool -------------------------------------------------------

TweenPool::TweenPool(size_t capacity) : capacity_(capacity) {
    tweens_ = new Tween[capacity_];
}

TweenPool::~TweenPool() {
    delete[] tweens_;
}

int TweenPool::Start(int widgetIdx, TweenProperty prop, float from, float to,
                     float duration, EaseType ease, float delay) {
    // Find free slot
    for (size_t i = 0; i < capacity_; ++i) {
        if (!tweens_[i].active) {
            Tween& tw = tweens_[i];
            tw.active = true;
            tw.widgetIdx = widgetIdx;
            tw.prop = prop;
            tw.startValue = from;
            tw.endValue = to;
            tw.duration = duration > 0.0f ? duration : 0.001f;
            tw.elapsed = 0.0f;
            tw.ease = ease;
            tw.delay = delay;
            tw.loop = false;
            tw.pingPong = false;
            tw.forward = true;
            tw.onComplete = nullptr;
            ++activeCount_;
            return static_cast<int>(i);
        }
    }
    return -1; // pool full
}

void TweenPool::Cancel(int tweenIdx) {
    if (tweenIdx < 0 || tweenIdx >= static_cast<int>(capacity_)) return;
    if (tweens_[tweenIdx].active) {
        tweens_[tweenIdx].active = false;
        tweens_[tweenIdx].onComplete = nullptr;
        --activeCount_;
    }
}

void TweenPool::CancelAll(int widgetIdx) {
    for (size_t i = 0; i < capacity_; ++i) {
        if (tweens_[i].active && tweens_[i].widgetIdx == widgetIdx) {
            tweens_[i].active = false;
            tweens_[i].onComplete = nullptr;
            --activeCount_;
        }
    }
}

int TweenPool::Update(float dt, ApplyFn apply, void* userData) {
    int completed = 0;

    for (size_t i = 0; i < capacity_; ++i) {
        Tween& tw = tweens_[i];
        if (!tw.active) continue;

        // Handle delay
        if (tw.delay > 0.0f) {
            tw.delay -= dt;
            if (tw.delay > 0.0f) continue;
            dt = -tw.delay; // overflow into elapsed
            tw.delay = 0.0f;
        }

        tw.elapsed += dt;

        float t = tw.elapsed / tw.duration;
        bool done = t >= 1.0f;
        t = std::min(t, 1.0f);

        // Apply easing
        float eased = EaseEvaluate(tw.ease, tw.forward ? t : (1.0f - t));
        float value = tw.startValue + (tw.endValue - tw.startValue) * eased;

        if (apply) {
            apply(tw.widgetIdx, tw.prop, value, userData);
        }

        if (done) {
            if (tw.pingPong) {
                tw.forward = !tw.forward;
                tw.elapsed = 0.0f;
                if (!tw.forward) continue; // still bouncing back
                // Completed full cycle
                if (tw.loop) {
                    tw.elapsed = 0.0f;
                    continue;
                }
            } else if (tw.loop) {
                tw.elapsed = 0.0f;
                continue;
            }

            // Tween complete
            TweenCallback cb = std::move(tw.onComplete);
            int wIdx = tw.widgetIdx;
            tw.active = false;
            tw.onComplete = nullptr;
            --activeCount_;
            ++completed;

            if (cb) cb(wIdx);
        }
    }

    return completed;
}

void TweenPool::SetLoop(int tweenIdx, bool loop, bool pingPong) {
    if (tweenIdx < 0 || tweenIdx >= static_cast<int>(capacity_)) return;
    if (!tweens_[tweenIdx].active) return;
    tweens_[tweenIdx].loop = loop;
    tweens_[tweenIdx].pingPong = pingPong;
}

void TweenPool::SetOnComplete(int tweenIdx, TweenCallback cb) {
    if (tweenIdx < 0 || tweenIdx >= static_cast<int>(capacity_)) return;
    if (!tweens_[tweenIdx].active) return;
    tweens_[tweenIdx].onComplete = std::move(cb);
}

const Tween* TweenPool::At(int idx) const {
    if (idx < 0 || idx >= static_cast<int>(capacity_)) return nullptr;
    return &tweens_[idx];
}

} // namespace ui
} // namespace koilo
