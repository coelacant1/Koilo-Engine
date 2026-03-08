// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <string>
#include <vector>
#include <functional>
#include "animationclip.hpp"
#include <koilo/registry/reflect_macros.hpp>

namespace koilo {

// Blend mode for animation layers.
enum class AnimBlendMode { Override, Additive };

// A single active animation layer in the mixer.
struct AnimationLayer {
    AnimationClip* clip = nullptr;
    float time = 0.0f;
    float weight = 1.0f;
    float speed = 1.0f;
    AnimBlendMode blendMode = AnimBlendMode::Override;
    bool playing = true;
    bool finished = false;

    KL_BEGIN_FIELDS(AnimationLayer)
        KL_FIELD(AnimationLayer, time, "Time", 0, 0),
        KL_FIELD(AnimationLayer, weight, "Weight", 0, 1),
        KL_FIELD(AnimationLayer, speed, "Speed", 0, 0),
        KL_FIELD(AnimationLayer, playing, "Playing", 0, 0),
        KL_FIELD(AnimationLayer, finished, "Finished", 0, 0)
    KL_END_FIELDS

    KL_BEGIN_METHODS(AnimationLayer)
        /* No reflected methods. */
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(AnimationLayer)
        /* No reflected ctors. */
    KL_END_DESCRIBE(AnimationLayer)

};

// Plays multiple AnimationClips simultaneously with weighted blending.
// Supports crossfade transitions between clips.
class AnimationMixer {
public:
    // Property applicator callback: (node, property, value)
    using Applicator = std::function<void(const std::string&, const std::string&, float)>;

    AnimationMixer();
    AnimationMixer(std::size_t maxLayers);

    // Play a clip on the given layer (0-based). Replaces existing clip on that layer.
    void Play(std::size_t layer, AnimationClip* clip, float weight = 1.0f, float speed = 1.0f);

    // Crossfade from the current clip on layer 0 to a new clip over fadeDuration seconds.
    void CrossfadeTo(AnimationClip* clip, float fadeDuration, float speed = 1.0f);

    // Stop playback on a layer.
    void Stop(std::size_t layer);

    // Stop all layers.
    void StopAll();

    // Set weight of a layer (0.0 - 1.0).
    void SetWeight(std::size_t layer, float weight);

    // Set speed of a layer.
    void SetSpeed(std::size_t layer, float speed);

    // Set blend mode of a layer.
    void SetBlendMode(std::size_t layer, AnimBlendMode mode);

    // Update all active layers and apply blended results via the applicator.
    void Update(const Applicator& applicator);

    // Update without applying (advances time only).
    void Update();

    // Get number of active layers.
    std::size_t GetActiveLayerCount() const;
    std::size_t GetMaxLayers() const { return maxLayers_; }

    // Check if a specific layer has finished playing (non-looping clips).
    bool IsLayerFinished(std::size_t layer) const;

private:
    std::size_t maxLayers_;
    std::vector<AnimationLayer> layers_;

    // Crossfade state
    bool crossfading_ = false;
    float crossfadeDuration_ = 0.0f;
    float crossfadeElapsed_ = 0.0f;

    void EnsureLayer(std::size_t layer);

public:
    KL_BEGIN_FIELDS(AnimationMixer)
    KL_END_FIELDS

    KL_BEGIN_METHODS(AnimationMixer)
        KL_METHOD_AUTO(AnimationMixer, StopAll, "Stop all layers"),
        KL_METHOD_AUTO(AnimationMixer, GetActiveLayerCount, "Get active layer count"),
        KL_METHOD_AUTO(AnimationMixer, GetMaxLayers, "Get max layers"),
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(AnimationMixer)
        KL_CTOR0(AnimationMixer),
        KL_CTOR(AnimationMixer, std::size_t)
    KL_END_DESCRIBE(AnimationMixer)
};

} // namespace koilo
