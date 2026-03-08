// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file blendshapetransformcontroller.hpp
 * @brief Declares the runtime BlendshapeTransformController class for handling 3D transform animations.
 *
 * This file defines the BlendshapeTransformController class, which manages position, scale, and
 * rotation offsets for 3D transform animations using a dictionary-based approach and
 * an animation controller.
 *
 * @author Coela Can't
 * @date 22/12/2024
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>
#include <koilo/registry/reflect_macros.hpp>

#include <koilo/systems/scene/animation/easyeaseanimator.hpp> // Include for animation controller interface.
#include <koilo/core/math/vector3d.hpp> // Include for 3D vector operations.


namespace koilo {

/**
 * @class BlendshapeTransformController
 * @brief Runtime-managed transform animation controller with a fixed capacity supplied at construction.
 *
 * The BlendshapeTransformController class allows the definition of multiple transform animation targets 
 * with position, scale, and rotation offsets. It integrates with an animation controller to dynamically
 * calculate the resulting transformation based on animation values.
 * 
 * NOTE: Despite the name, this does NOT control Blendshape vertex morphing objects.
 * It animates Transform properties (position/scale/rotation offsets).
 */
class BlendshapeTransformController {
private:
    using Index = std::size_t;

public:
    // Sentinel value indicating no matching blendshape.
    static constexpr Index kInvalidIndex = ~Index(0);

    /**
     * @brief Constructs a BlendshapeTransformController object with an animation controller.
     *
     * @param eEA Pointer to the IEasyEaseAnimator instance.
     * @param maxBlendshapes Maximum number of transform animation targets that can be registered.
     */
    explicit BlendshapeTransformController(IEasyEaseAnimator* eEA, std::size_t maxBlendshapes = 16);

    /** @return Number of currently registered blendshape targets. */
    [[nodiscard]] std::size_t GetBlendshapeCount() const { return currentBlendshapes_; }

    /** @return Maximum number of blendshape targets supported. */
    [[nodiscard]] std::size_t GetCapacity() const { return capacity_; }

    /**
     * @brief Adds a blendshape target with a position offset.
     *
     * @param dictionaryValue The identifier for the blendshape target.
     * @param positionOffset The position offset for the blendshape target.
     */
    void AddBlendshape(uint16_t dictionaryValue, Vector3D positionOffset);

    /**
     * @brief Adds a blendshape target with position and scale offsets.
     *
     * @param dictionaryValue The identifier for the blendshape target.
     * @param positionOffset The position offset for the blendshape target.
     * @param scaleOffset The scale offset for the blendshape target.
     */
    void AddBlendshape(uint16_t dictionaryValue, Vector3D positionOffset, Vector3D scaleOffset);

    /**
     * @brief Adds a blendshape target with position, scale, and rotation offsets.
     *
     * @param dictionaryValue The identifier for the blendshape target.
     * @param positionOffset The position offset for the blendshape target.
     * @param scaleOffset The scale offset for the blendshape target.
     * @param rotationOffset The rotation offset for the blendshape target.
     */
    void AddBlendshape(uint16_t dictionaryValue, Vector3D positionOffset, Vector3D scaleOffset, Vector3D rotationOffset);

    /**
     * @brief Sets the position offset for a specific blendshape target.
     *
     * @param dictionaryValue The identifier for the blendshape target.
     * @param positionOffset The new position offset.
     */
    void SetBlendshapePositionOffset(uint16_t dictionaryValue, Vector3D positionOffset);

    /**
     * @brief Sets the scale offset for a specific blendshape target.
     *
     * @param dictionaryValue The identifier for the blendshape target.
     * @param scaleOffset The new scale offset.
     */
    void SetBlendshapeScaleOffset(uint16_t dictionaryValue, Vector3D scaleOffset);

    /**
     * @brief Sets the rotation offset for a specific blendshape target.
     *
     * @param dictionaryValue The identifier for the blendshape target.
     * @param rotationOffset The new rotation offset.
     */
    void SetBlendshapeRotationOffset(uint16_t dictionaryValue, Vector3D rotationOffset);

    /**
     * @brief Retrieves the combined position offset for all active blendshape targets.
     *
     * @return The cumulative position offset.
     */
    Vector3D GetPositionOffset();

    /**
     * @brief Retrieves the combined scale offset for all active blendshape targets.
     *
     * @return The cumulative scale offset.
     */
    Vector3D GetScaleOffset();

    /**
     * @brief Retrieves the combined rotation offset for all active blendshape targets.
     *
     * @return The cumulative rotation offset.
     */
    Vector3D GetRotationOffset();

private:
    [[nodiscard]] Index FindIndex(uint16_t dictionaryValue) const;

    IEasyEaseAnimator* eEA_; ///< Pointer to the animation controller.
    std::size_t        capacity_; ///< Maximum number of blendshape targets.
    std::size_t        currentBlendshapes_ = 0; ///< Current number of blendshape targets.
    std::vector<uint16_t> dictionary_; ///< Dictionary mapping blendshape targets to identifiers.
    std::vector<Vector3D> positionOffsets_; ///< Array of position offsets for blendshape targets.
    std::vector<Vector3D> scaleOffsets_; ///< Array of scale offsets for blendshape targets.
    std::vector<Vector3D> rotationOffsets_; ///< Array of rotation offsets for blendshape targets.

    KL_BEGIN_FIELDS(BlendshapeTransformController)
        /* No reflected fields. */
    KL_END_FIELDS

    KL_BEGIN_METHODS(BlendshapeTransformController)
        KL_METHOD_AUTO(BlendshapeTransformController, GetBlendshapeCount, "Get blendshape count"),
        KL_METHOD_AUTO(BlendshapeTransformController, GetCapacity, "Get capacity"),
        /* Add blendshape */ KL_METHOD_OVLD(BlendshapeTransformController, AddBlendshape, void, uint16_t, Vector3D),
        /* Add blendshape */ KL_METHOD_OVLD(BlendshapeTransformController, AddBlendshape, void, uint16_t, Vector3D, Vector3D),
        /* Add blendshape */ KL_METHOD_OVLD(BlendshapeTransformController, AddBlendshape, void, uint16_t, Vector3D, Vector3D, Vector3D),
        KL_METHOD_AUTO(BlendshapeTransformController, SetBlendshapePositionOffset, "Set blendshape position offset"),
        KL_METHOD_AUTO(BlendshapeTransformController, SetBlendshapeScaleOffset, "Set blendshape scale offset"),
        KL_METHOD_AUTO(BlendshapeTransformController, SetBlendshapeRotationOffset, "Set blendshape rotation offset"),
        KL_METHOD_AUTO(BlendshapeTransformController, GetPositionOffset, "Get position offset"),
        KL_METHOD_AUTO(BlendshapeTransformController, GetScaleOffset, "Get scale offset"),
        KL_METHOD_AUTO(BlendshapeTransformController, GetRotationOffset, "Get rotation offset")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(BlendshapeTransformController)
        KL_CTOR(BlendshapeTransformController, IEasyEaseAnimator *, std::size_t)
    KL_END_DESCRIBE(BlendshapeTransformController)

};

} // namespace koilo
