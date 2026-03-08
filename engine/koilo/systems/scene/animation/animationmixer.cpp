// SPDX-License-Identifier: GPL-3.0-or-later
#include <koilo/systems/scene/animation/animationmixer.hpp>
#include <koilo/core/time/timemanager.hpp>
#include <map>
#include <string>

namespace koilo {

AnimationMixer::AnimationMixer() : maxLayers_(4) {
    layers_.resize(maxLayers_);
}

AnimationMixer::AnimationMixer(std::size_t maxLayers) : maxLayers_(maxLayers) {
    layers_.resize(maxLayers_);
}

void AnimationMixer::EnsureLayer(std::size_t layer) {
    if (layer >= layers_.size()) {
        layers_.resize(layer + 1);
    }
}

void AnimationMixer::Play(std::size_t layer, AnimationClip* clip, float weight, float speed) {
    EnsureLayer(layer);
    auto& l = layers_[layer];
    l.clip = clip;
    l.time = 0.0f;
    l.weight = weight;
    l.speed = speed;
    l.playing = true;
    l.finished = false;
    l.blendMode = AnimBlendMode::Override;
}

void AnimationMixer::CrossfadeTo(AnimationClip* clip, float fadeDuration, float speed) {
    // Move current layer 0 clip to layer 1 (fade-out layer)
    if (layers_.size() >= 2 && layers_[0].clip && layers_[0].playing) {
        layers_[1] = layers_[0];
        layers_[1].blendMode = AnimBlendMode::Override;
    }
    // Start new clip on layer 0
    Play(0, clip, 0.0f, speed);  // starts at weight 0, will ramp up
    crossfading_ = true;
    crossfadeDuration_ = fadeDuration > 0.001f ? fadeDuration : 0.001f;
    crossfadeElapsed_ = 0.0f;
}

void AnimationMixer::Stop(std::size_t layer) {
    if (layer < layers_.size()) {
        layers_[layer].playing = false;
        layers_[layer].clip = nullptr;
        layers_[layer].finished = true;
    }
}

void AnimationMixer::StopAll() {
    for (auto& l : layers_) {
        l.playing = false;
        l.clip = nullptr;
        l.finished = true;
    }
    crossfading_ = false;
}

void AnimationMixer::SetWeight(std::size_t layer, float weight) {
    EnsureLayer(layer);
    layers_[layer].weight = weight;
}

void AnimationMixer::SetSpeed(std::size_t layer, float speed) {
    EnsureLayer(layer);
    layers_[layer].speed = speed;
}

void AnimationMixer::SetBlendMode(std::size_t layer, AnimBlendMode mode) {
    EnsureLayer(layer);
    layers_[layer].blendMode = mode;
}

void AnimationMixer::Update() {
    Update(nullptr);
}

void AnimationMixer::Update(const Applicator& applicator) {
    float dt = TimeManager::GetInstance().GetDeltaTime();
    // Handle crossfade weight ramping
    if (crossfading_) {
        crossfadeElapsed_ += dt;
        float t = crossfadeElapsed_ / crossfadeDuration_;
        if (t >= 1.0f) {
            t = 1.0f;
            crossfading_ = false;
            // Crossfade complete: stop the old layer
            if (layers_.size() >= 2) {
                layers_[1].playing = false;
                layers_[1].clip = nullptr;
            }
        }
        // Ramp layer 0 up, layer 1 down
        layers_[0].weight = t;
        if (layers_.size() >= 2 && layers_[1].clip) {
            layers_[1].weight = 1.0f - t;
        }
    }

    // Accumulate blended property values: map<"node:property", value>
    std::map<std::string, float> blended;

    for (auto& layer : layers_) {
        if (!layer.playing || !layer.clip || layer.weight <= 0.0f) continue;

        // Advance time
        layer.time += dt * layer.speed;

        float duration = layer.clip->GetDuration();
        if (duration > 0.0f && !layer.clip->GetLooping() && layer.time >= duration) {
            layer.time = duration;
            layer.finished = true;
            layer.playing = false;
        }

        // Evaluate clip and blend
        float w = layer.weight;
        AnimBlendMode mode = layer.blendMode;

        layer.clip->Evaluate(layer.time, [&](const std::string& node, const std::string& prop, float value) {
            std::string key = node + ":" + prop;
            if (mode == AnimBlendMode::Additive) {
                blended[key] += value * w;
            } else {
                // Override: weighted replacement
                auto it = blended.find(key);
                if (it == blended.end()) {
                    blended[key] = value * w;
                } else {
                    it->second = it->second * (1.0f - w) + value * w;
                }
            }
        });
    }

    // Apply blended values
    if (applicator) {
        for (auto& kv : blended) {
            auto sep = kv.first.find(':');
            std::string node = kv.first.substr(0, sep);
            std::string prop = (sep != std::string::npos) ? kv.first.substr(sep + 1) : "";
            applicator(node, prop, kv.second);
        }
    }
}

std::size_t AnimationMixer::GetActiveLayerCount() const {
    std::size_t count = 0;
    for (auto& l : layers_) {
        if (l.playing && l.clip) ++count;
    }
    return count;
}

bool AnimationMixer::IsLayerFinished(std::size_t layer) const {
    if (layer >= layers_.size()) return true;
    return layers_[layer].finished;
}

} // namespace koilo
