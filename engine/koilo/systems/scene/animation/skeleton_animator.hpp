// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file skeleton_animator.hpp
 * @brief Bridges AnimationClip/Mixer -> Skeleton bone transforms.
 *
 * Applies animation channel values to bone local transforms.
 * Channels use the naming convention:
 *   targetNode = "bone.<boneName>"  (e.g., "bone.spine", "bone.arm_l")
 *   targetProperty = "position.x" | "rotation.x" | "scale.x" etc.
 *
 * Usage:
 *   SkeletonAnimator animator(&skeleton);
 *   animator.ApplyClip(&walkClip, currentTime);
 *   // or with mixer:
 *   mixer.Update([&](auto& node, auto& prop, float val) {
 *       animator.ApplyChannel(node, prop, val);
 *   });
 *   skeleton.ComputeWorldMatrices();
 *
 * @date 23/02/2026
 * @author Coela
 */

#pragma once

#include <koilo/systems/scene/animation/skeleton.hpp>
#include <koilo/systems/scene/animation/animationclip.hpp>
#include <koilo/core/math/mathematics.hpp>
#include <string>
#include <unordered_map>
#include <cstdint>
#include "../../../registry/reflect_macros.hpp"

namespace koilo {

/**
 * @class SkeletonAnimator
 * @brief Applies animation data to skeleton bone transforms.
 */
class SkeletonAnimator {
public:
    explicit SkeletonAnimator(Skeleton* skeleton) : skeleton_(skeleton) {}

    /**
     * @brief Apply a single channel value to the skeleton.
     *
     * Expected naming: targetNode = "bone.<name>", targetProperty = "<component>.<axis>"
     * Components: position, rotation, scale. Axes: x, y, z.
     *
     * @param targetNode Channel target node string.
     * @param targetProperty Channel target property string.
     * @param value The interpolated value.
     */
    void ApplyChannel(const std::string& targetNode,
                      const std::string& targetProperty,
                      float value) {
        if (!skeleton_) return;

        // Parse "bone.<name>" prefix
        if (targetNode.size() <= 5 || targetNode.substr(0, 5) != "bone.") return;
        std::string boneName = targetNode.substr(5);

        Bone* bone = skeleton_->GetBone(boneName);
        if (!bone) return;

        // Parse "<component>.<axis>"
        auto dot = targetProperty.find('.');
        if (dot == std::string::npos) return;

        std::string component = targetProperty.substr(0, dot);
        std::string axis = targetProperty.substr(dot + 1);

        Component comp;
        if      (component == "position") comp = Component::Position;
        else if (component == "rotation") comp = Component::Rotation;
        else if (component == "scale")    comp = Component::Scale;
        else return;

        Axis ax;
        if      (axis == "x") ax = Axis::X;
        else if (axis == "y") ax = Axis::Y;
        else if (axis == "z") ax = Axis::Z;
        else return;

        ApplyResolved(bone, comp, ax, value);
    }

    /**
     * @brief A5: apply a pre-resolved channel by reference.
     *
     * Caches (Bone*, Component, Axis) keyed on AnimationChannel::GetChannelId()
     * after first resolution; subsequent applies skip all string parsing /
     * skeleton bone-name lookups.
     */
    void ApplyChannelCached(const AnimationChannel& ch, float value) {
        if (!skeleton_) return;
        std::uint32_t id = ch.GetChannelId();
        auto it = resolveCache_.find(id);
        const ResolvedTarget* rt;
        if (it == resolveCache_.end()) {
            ResolvedTarget r = ResolveChannel(ch);
            it = resolveCache_.emplace(id, r).first;
        }
        rt = &it->second;
        if (!rt->bone) return;
        ApplyResolved(rt->bone, rt->component, rt->axis, value);
    }

    /**
     * @brief Evaluate an AnimationClip and apply all bone channels.
     *
     * Convenience method that evaluates the clip and routes each channel
     * through ApplyChannel.
     *
     * @param clip The animation clip to evaluate.
     * @param time Current playback time.
     */
    void ApplyClip(const AnimationClip* clip, float time) {
        if (!clip || !skeleton_) return;
        // A5: use the packed-channel callback so we can use cached resolution.
        clip->EvaluatePacked(time, [this](const AnimationChannel& ch, float val) {
            ApplyChannelCached(ch, val);
        });
    }

    void SetSkeleton(Skeleton* skeleton) {
        if (skeleton_ != skeleton) resolveCache_.clear();
        skeleton_ = skeleton;
    }
    Skeleton* GetSkeleton() const { return skeleton_; }

    // Drop the resolution cache (e.g. after re-binding channels in a clip).
    void ClearResolveCache() { resolveCache_.clear(); }

private:
    enum class Component : std::uint8_t { Position, Rotation, Scale };
    enum class Axis      : std::uint8_t { X, Y, Z };

    struct ResolvedTarget {
        Bone*     bone      = nullptr;
        Component component = Component::Position;
        Axis      axis      = Axis::X;
    };

    ResolvedTarget ResolveChannel(const AnimationChannel& ch) const {
        ResolvedTarget r{};
        const std::string& node = ch.targetNode;
        const std::string& prop = ch.targetProperty;
        if (node.size() <= 5 || node.compare(0, 5, "bone.") != 0) return r;
        Bone* bone = skeleton_->GetBone(node.substr(5));
        if (!bone) return r;

        auto dot = prop.find('.');
        if (dot == std::string::npos) return r;

        // component
        if      (prop.compare(0, dot, "position") == 0) r.component = Component::Position;
        else if (prop.compare(0, dot, "rotation") == 0) r.component = Component::Rotation;
        else if (prop.compare(0, dot, "scale")    == 0) r.component = Component::Scale;
        else return r;

        // axis (single char)
        if (dot + 1 >= prop.size()) return r;
        char axc = prop[dot + 1];
        if      (axc == 'x') r.axis = Axis::X;
        else if (axc == 'y') r.axis = Axis::Y;
        else if (axc == 'z') r.axis = Axis::Z;
        else return r;

        r.bone = bone;
        return r;
    }

    void ApplyResolved(Bone* bone, Component comp, Axis ax, float value) const {
        switch (comp) {
            case Component::Position:
                if      (ax == Axis::X) bone->localPosition.X = value;
                else if (ax == Axis::Y) bone->localPosition.Y = value;
                else                    bone->localPosition.Z = value;
                break;
            case Component::Rotation: {
                float rad = value * 3.14159265f / 180.0f;
                float halfAngle = rad * 0.5f;
                float s = Mathematics::Sin(halfAngle);
                float c = Mathematics::Cos(halfAngle);
                if      (ax == Axis::X) bone->localRotation = Quaternion(c, s, 0, 0);
                else if (ax == Axis::Y) bone->localRotation = Quaternion(c, 0, s, 0);
                else                    bone->localRotation = Quaternion(c, 0, 0, s);
                break;
            }
            case Component::Scale:
                if      (ax == Axis::X) bone->localScale.X = value;
                else if (ax == Axis::Y) bone->localScale.Y = value;
                else                    bone->localScale.Z = value;
                break;
        }
    }

    Skeleton* skeleton_ = nullptr;
    std::unordered_map<std::uint32_t, ResolvedTarget> resolveCache_;

    KL_BEGIN_FIELDS(SkeletonAnimator)
        /* No reflected fields. */
    KL_END_FIELDS

    KL_BEGIN_METHODS(SkeletonAnimator)
        KL_METHOD_AUTO(SkeletonAnimator, ApplyChannel, "Apply channel"),
        KL_METHOD_AUTO(SkeletonAnimator, ApplyClip, "Apply clip"),
        KL_METHOD_AUTO(SkeletonAnimator, SetSkeleton, "Set skeleton"),
        KL_METHOD_AUTO(SkeletonAnimator, GetSkeleton, "Get skeleton")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(SkeletonAnimator)
        KL_CTOR(SkeletonAnimator, Skeleton*)
    KL_END_DESCRIBE(SkeletonAnimator)

};

} // namespace koilo
