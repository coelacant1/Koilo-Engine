/**
 * @file blendshapecontroller.hpp
 * @brief Declares the BlendshapeController template class for handling 3D transformations.
 *
 * This file defines the BlendshapeController class, which manages position, scale, and
 * rotation offsets for 3D transformations using a dictionary-based approach and
 * an animation controller.
 *
 * @author Coela Can't
 * @date 22/12/2024
 */

#pragma once

#include <cstddef>
#include "../animation/easyeaseanimator.hpp" // Include for animation controller interface.
#include "../../../core/math/vector3d.hpp" // Include for 3D vector operations.

/**
 * @class BlendshapeController
 * @brief A template class for managing 3D transformations using blendshape targets.
 *
 * The BlendshapeController class allows the definition of multiple blendshape targets with position,
 * scale, and rotation offsets. It integrates with an animation controller to dynamically
 * calculate the resulting transformation based on animation values.
 *
 * @tparam maxBlendshapes The maximum number of blendshape targets this class can handle.
 */
template<size_t maxBlendshapes>
class BlendshapeController {
private:
    IEasyEaseAnimator* eEA; ///< Pointer to the animation controller.
    uint16_t dictionary[maxBlendshapes]; ///< Dictionary mapping blendshape targets to identifiers.
    uint16_t currentBlendshapes = 0; ///< Current number of blendshape targets.
    Vector3D positionOffsets[maxBlendshapes]; ///< Array of position offsets for blendshape targets.
    Vector3D scaleOffsets[maxBlendshapes]; ///< Array of scale offsets for blendshape targets.
    Vector3D rotationOffsets[maxBlendshapes]; ///< Array of rotation offsets for blendshape targets.

public:
    /**
     * @brief Constructs a BlendshapeController object with an animation controller.
     *
     * @param eEA Pointer to the IEasyEaseAnimator instance.
     */
    BlendshapeController(IEasyEaseAnimator* eEA);

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
};

#include "blendshapecontroller.tpp" // Include the template implementation.
