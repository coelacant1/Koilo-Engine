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

        if (component == "position") {
            if (axis == "x") bone->localPosition.X = value;
            else if (axis == "y") bone->localPosition.Y = value;
            else if (axis == "z") bone->localPosition.Z = value;
        } else if (component == "rotation") {
            // Value is angle in degrees around the specified axis
            float rad = value * 3.14159265f / 180.0f;
            float halfAngle = rad * 0.5f;
            float s = Mathematics::Sin(halfAngle);
            float c = Mathematics::Cos(halfAngle);
            if (axis == "x")      bone->localRotation = Quaternion(c, s, 0, 0);
            else if (axis == "y") bone->localRotation = Quaternion(c, 0, s, 0);
            else if (axis == "z") bone->localRotation = Quaternion(c, 0, 0, s);
        } else if (component == "scale") {
            if (axis == "x") bone->localScale.X = value;
            else if (axis == "y") bone->localScale.Y = value;
            else if (axis == "z") bone->localScale.Z = value;
        }
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
        clip->Evaluate(time, [this](const std::string& node,
                                     const std::string& prop,
                                     float val) {
            ApplyChannel(node, prop, val);
        });
    }

    void SetSkeleton(Skeleton* skeleton) { skeleton_ = skeleton; }
    Skeleton* GetSkeleton() const { return skeleton_; }

private:
    Skeleton* skeleton_ = nullptr;

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
