// SPDX-License-Identifier: GPL-3.0-or-later
#include <koilo/systems/scene/animation/animationclip.hpp>
#include <cmath>
#include <algorithm>

namespace koilo {

// --- AnimationChannel ---

void AnimationChannel::AddKey(float time, float value) {
    // Insert sorted by time
    KeyFrame kf(time, value);
    auto it = std::lower_bound(keyframes.begin(), keyframes.end(), kf,
        [](const KeyFrame& a, const KeyFrame& b) { return a.Time < b.Time; });
    keyframes.insert(it, kf);
}

float AnimationChannel::Evaluate(float time) const {
    if (keyframes.empty()) return 0.0f;
    if (keyframes.size() == 1) return keyframes[0].Value;

    // Clamp to range
    if (time <= keyframes.front().Time) return keyframes.front().Value;
    if (time >= keyframes.back().Time) return keyframes.back().Value;

    // Find surrounding keyframes
    for (std::size_t i = 0; i + 1 < keyframes.size(); ++i) {
        const KeyFrame& a = keyframes[i];
        const KeyFrame& b = keyframes[i + 1];
        if (time >= a.Time && time <= b.Time) {
            float span = b.Time - a.Time;
            if (span < 0.0001f) return a.Value;
            float t = (time - a.Time) / span;

            switch (interpolation) {
                case ChannelInterp::Step:
                    return a.Value;
                case ChannelInterp::Cosine: {
                    float ct = (1.0f - std::cos(t * 3.14159265f)) * 0.5f;
                    return a.Value + (b.Value - a.Value) * ct;
                }
                case ChannelInterp::Linear:
                default:
                    return a.Value + (b.Value - a.Value) * t;
            }
        }
    }
    return keyframes.back().Value;
}

// --- AnimationClip ---

AnimationClip::AnimationClip() {}

AnimationClip::AnimationClip(const std::string& name, float duration)
    : name_(name), duration_(duration) {}

std::size_t AnimationClip::AddChannel(const std::string& property) {
    AnimationChannel ch;
    ch.targetProperty = property;
    channels_.push_back(std::move(ch));
    return channels_.size() - 1;
}

std::size_t AnimationClip::AddChannelForNode(const std::string& node, const std::string& property) {
    AnimationChannel ch;
    ch.targetNode = node;
    ch.targetProperty = property;
    channels_.push_back(std::move(ch));
    return channels_.size() - 1;
}

AnimationChannel* AnimationClip::GetChannel(std::size_t index) {
    if (index >= channels_.size()) return nullptr;
    return &channels_[index];
}

void AnimationClip::Evaluate(float time,
    const std::function<void(const std::string&, const std::string&, float)>& applicator) const {
    EvaluatePacked(time, [&](const AnimationChannel& ch, float value) {
        applicator(ch.targetNode, ch.targetProperty, value);
    });
}

void AnimationClip::EvaluatePacked(float time,
    const std::function<void(const AnimationChannel&, float)>& cb) const {
    float evalTime = time;
    if (looping_ && duration_ > 0.0f) {
        evalTime = std::fmod(time, duration_);
        if (evalTime < 0.0f) evalTime += duration_;
    }
    for (auto& ch : channels_) {
        float value = ch.Evaluate(evalTime);
        cb(ch, value);
    }
}

// FNV-1a 32-bit hash of "node:prop" - gives a stable per-channel id without
// having to materialise the concatenated string.
std::uint32_t AnimationChannel::GetChannelId() const {
    if (cachedId_ != 0) return cachedId_;
    std::uint32_t h = 2166136261u;
    auto mix = [&](const std::string& s) {
        for (unsigned char c : s) {
            h ^= c;
            h *= 16777619u;
        }
    };
    mix(targetNode);
    h ^= ':';
    h *= 16777619u;
    mix(targetProperty);
    if (h == 0) h = 1;  // reserve 0 as "uncomputed" sentinel
    cachedId_ = h;
    return h;
}

} // namespace koilo
