/**
 * @file blendshapecontroller.hpp
 * @brief Declares the runtime BlendshapeController class for handling 3D transformations.
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
#include <cstdint>
#include <limits>
#include <vector>
#include "../../../registry/reflect_macros.hpp"

#include "../animation/easyeaseanimator.hpp" // Include for animation controller interface.
#include "../../../core/math/vector3d.hpp" // Include for 3D vector operations.

/**
 * @class BlendshapeController
 * @brief Runtime-managed blendshape controller with a fixed capacity supplied at construction.

 * The BlendshapeController class allows the definition of multiple blendshape targets with position,
 * scale, and rotation offsets. It integrates with an animation controller to dynamically
 * calculate the resulting transformation based on animation values.
 */
class BlendshapeController {
private:
    using Index = std::size_t;

public:
    /// Sentinel value indicating no matching blendshape.
    static constexpr Index kInvalidIndex = std::numeric_limits<Index>::max();

    /**
     * @brief Constructs a BlendshapeController object with an animation controller.
     *
     * @param eEA Pointer to the IEasyEaseAnimator instance.
     * @param maxBlendshapes Maximum number of blendshape targets that can be registered.
     */
    explicit BlendshapeController(IEasyEaseAnimator* eEA, std::size_t maxBlendshapes = 16);

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

    PTX_BEGIN_FIELDS(BlendshapeController)
        /* No reflected fields. */
    PTX_END_FIELDS

    PTX_BEGIN_METHODS(BlendshapeController)
        PTX_METHOD_AUTO(BlendshapeController, GetBlendshapeCount, "Get blendshape count"),
        PTX_METHOD_AUTO(BlendshapeController, GetCapacity, "Get capacity"),
        /* Add blendshape */ PTX_METHOD_OVLD(BlendshapeController, AddBlendshape, void, uint16_t, Vector3D),
        /* Add blendshape */ PTX_METHOD_OVLD(BlendshapeController, AddBlendshape, void, uint16_t, Vector3D, Vector3D),
        /* Add blendshape */ PTX_METHOD_OVLD(BlendshapeController, AddBlendshape, void, uint16_t, Vector3D, Vector3D, Vector3D),
        PTX_METHOD_AUTO(BlendshapeController, SetBlendshapePositionOffset, "Set blendshape position offset"),
        PTX_METHOD_AUTO(BlendshapeController, SetBlendshapeScaleOffset, "Set blendshape scale offset"),
        PTX_METHOD_AUTO(BlendshapeController, SetBlendshapeRotationOffset, "Set blendshape rotation offset"),
        PTX_METHOD_AUTO(BlendshapeController, GetPositionOffset, "Get position offset"),
        PTX_METHOD_AUTO(BlendshapeController, GetScaleOffset, "Get scale offset"),
        PTX_METHOD_AUTO(BlendshapeController, GetRotationOffset, "Get rotation offset")
    PTX_END_METHODS

    PTX_BEGIN_DESCRIBE(BlendshapeController)
        PTX_CTOR(BlendshapeController, IEasyEaseAnimator *, std::size_t)
    PTX_END_DESCRIBE(BlendshapeController)

};
