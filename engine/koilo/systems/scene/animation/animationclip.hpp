// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <string>
#include <vector>
#include <functional>
#include "keyframe.hpp"
#include <koilo/core/math/mathematics.hpp>
#include <koilo/registry/reflect_macros.hpp>

namespace koilo {

// Interpolation methods for animation channels.
enum class ChannelInterp { Linear, Cosine, Step };

// A single animation channel targeting one float property.
// Stores keyframes and evaluates the value at a given time.
struct AnimationChannel {
    std::string targetNode;     ///< Scene node name (empty = self)
    std::string targetProperty; ///< Property path: "position.x", "morphWeight.Blink", "opacity"
    std::vector<KeyFrame> keyframes;
    ChannelInterp interpolation = ChannelInterp::Cosine;

    // Add a keyframe at the given time with the given value.
    void AddKey(float time, float value);

    // Evaluate the channel at a given time, returning the interpolated value.
    float Evaluate(float time) const;

    KL_BEGIN_FIELDS(AnimationChannel)
    KL_END_FIELDS

    KL_BEGIN_METHODS(AnimationChannel)
        KL_METHOD_AUTO(AnimationChannel, AddKey, "Add keyframe"),
        KL_METHOD_AUTO(AnimationChannel, Evaluate, "Evaluate at time"),
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(AnimationChannel)
        KL_CTOR0(AnimationChannel)
    KL_END_DESCRIBE(AnimationChannel)
};

// A collection of channels that together define one animation.
// Can target multiple properties on multiple nodes.
class AnimationClip {
public:
    AnimationClip();
    AnimationClip(const std::string& name, float duration);

    const std::string& GetName() const { return name_; }
    void SetName(const std::string& name) { name_ = name; }

    float GetDuration() const { return duration_; }
    void SetDuration(float duration) { duration_ = duration; }

    bool GetLooping() const { return looping_; }
    void SetLooping(bool loop) { looping_ = loop; }

    // Add a channel to this clip. Returns channel index.
    std::size_t AddChannel(const std::string& property);
    std::size_t AddChannelForNode(const std::string& node, const std::string& property);

    // Get a channel by index for adding keyframes.
    AnimationChannel* GetChannel(std::size_t index);
    std::size_t GetChannelCount() const { return channels_.size(); }

    // Evaluate all channels at a given time. Calls the applicator for each result.
    // applicator(targetNode, targetProperty, value)
    void Evaluate(float time, const std::function<void(const std::string&, const std::string&, float)>& applicator) const;

private:
    std::string name_;
    float duration_ = 0.0f;
    bool looping_ = false;
    std::vector<AnimationChannel> channels_;

public:
    KL_BEGIN_FIELDS(AnimationClip)
    KL_END_FIELDS

    KL_BEGIN_METHODS(AnimationClip)
        KL_METHOD_AUTO(AnimationClip, GetName, "Get clip name"),
        KL_METHOD_AUTO(AnimationClip, SetName, "Set clip name"),
        KL_METHOD_AUTO(AnimationClip, GetDuration, "Get duration"),
        KL_METHOD_AUTO(AnimationClip, SetDuration, "Set duration"),
        KL_METHOD_AUTO(AnimationClip, GetLooping, "Get looping"),
        KL_METHOD_AUTO(AnimationClip, SetLooping, "Set looping"),
        KL_METHOD_AUTO(AnimationClip, AddChannel, "Add channel for property"),
        KL_METHOD_AUTO(AnimationClip, GetChannel, "Get channel by index"),
        KL_METHOD_AUTO(AnimationClip, GetChannelCount, "Get channel count"),
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(AnimationClip)
        KL_CTOR0(AnimationClip),
        KL_CTOR(AnimationClip, const std::string&, float)
    KL_END_DESCRIBE(AnimationClip)
};

} // namespace koilo
