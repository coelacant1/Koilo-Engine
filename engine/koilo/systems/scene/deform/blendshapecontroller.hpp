// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file blendshapecontroller.hpp
 * @brief Controller for managing and applying multiple vertex-based blendshapes (morph targets).
 *
 * This file defines the BlendshapeController class, which manages a collection of Blendshape
 * objects and their weights, integrating with the animation system to apply vertex morphing
 * to meshes.
 *
 * @author Coela
 * @date TBD
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>
#include <koilo/registry/reflect_macros.hpp>

#include "blendshape.hpp"
#include <koilo/systems/scene/animation/ieasyeaseanimator.hpp>
#include <koilo/systems/scene/mesh.hpp>


namespace koilo {

/**
 * @class BlendshapeController
 * @brief Manages multiple vertex-based blendshapes (morph targets) for a mesh.
 *
 * The BlendshapeController class provides centralized management of Blendshape objects,
 * handling their weights through an animation system and applying all active morphs
 * to mesh geometry in a single update call.
 *
 * Typical workflow:
 * 1. Create controller with animator
 * 2. Add blendshapes with IDs
 * 3. Animator drives weights via dictionary IDs
 * 4. Call Update() to apply all weighted blendshapes to mesh
 */
class BlendshapeController {
private:
    IEasyEaseAnimator* animator_; ///< Animation system for driving weights
    std::vector<Blendshape*> blendshapes_; ///< Collection of blendshape objects (non-owning)
    std::vector<uint16_t> dictionaryIds_; ///< Animation dictionary IDs for each blendshape
    std::size_t capacity_; ///< Maximum number of blendshapes

public:
    /**
     * @brief Constructs a BlendshapeController.
     *
     * @param animator Pointer to the IEasyEaseAnimator for weight management (can be nullptr)
     * @param capacity Maximum number of blendshapes to support
     */
    explicit BlendshapeController(IEasyEaseAnimator* animator = nullptr, std::size_t capacity = 16);

    /**
     * @brief Destructor.
     */
    ~BlendshapeController() = default;

    /**
     * @brief Sets the animation controller.
     *
     * @param animator Pointer to the IEasyEaseAnimator instance
     */
    void SetAnimator(IEasyEaseAnimator* animator);

    /**
     * @brief Adds a blendshape to the controller.
     *
     * @param dictionaryId Animation dictionary ID for weight control
     * @param blendshape Pointer to the Blendshape object (non-owning)
     * @return True if added successfully, false if capacity reached or nullptr
     */
    bool AddBlendshape(uint16_t dictionaryId, Blendshape* blendshape);

    /**
     * @brief Removes a blendshape by dictionary ID.
     *
     * @param dictionaryId The dictionary ID of the blendshape to remove
     * @return True if removed successfully, false if not found
     */
    bool RemoveBlendshape(uint16_t dictionaryId);

    /**
     * @brief Gets the number of registered blendshapes.
     *
     * @return Number of blendshapes currently registered
     */
    std::size_t GetBlendshapeCount() const;

    /**
     * @brief Gets the maximum capacity.
     *
     * @return Maximum number of blendshapes supported
     */
    std::size_t GetCapacity() const;

    /**
     * @brief Manually sets the weight for a blendshape.
     *
     * @param dictionaryId The dictionary ID of the blendshape
     * @param weight The weight value to set (typically 0.0 to 1.0)
     */
    void SetWeight(uint16_t dictionaryId, float weight);

    /**
     * @brief Gets the current weight for a blendshape.
     *
     * @param dictionaryId The dictionary ID of the blendshape
     * @return The current weight (from animator if available, else from Blendshape.Weight)
     */
    float GetWeight(uint16_t dictionaryId) const;

    /**
     * @brief Resets all blendshape weights to 0.
     */
    void ResetWeights();

    /**
     * @brief Updates and applies all blendshapes to a mesh.
     *
     * This method:
     * 1. Resets the mesh vertices to original state
     * 2. Gets weights from animator (if set) or uses Blendshape.Weight
     * 3. Applies each blendshape with its weight to the mesh geometry
     *
     * @param mesh Pointer to the Mesh to apply blendshapes to
     */
    void Update(Mesh* mesh);

    /**
     * @brief Applies all blendshapes to a triangle group directly.
     *
     * @param triangleGroup Pointer to the ITriangleGroup to apply blendshapes to
     */
    void ApplyTo(ITriangleGroup* triangleGroup);

private:
    /**
     * @brief Finds the index of a blendshape by dictionary ID.
     *
     * @param dictionaryId The dictionary ID to search for
     * @return Index if found, -1 if not found
     */
    int FindIndex(uint16_t dictionaryId) const;

    KL_BEGIN_FIELDS(BlendshapeController)
        /* No reflected fields. */
    KL_END_FIELDS

    KL_BEGIN_METHODS(BlendshapeController)
        KL_METHOD_AUTO(BlendshapeController, SetAnimator, "Set animator"),
        KL_METHOD_AUTO(BlendshapeController, AddBlendshape, "Add blendshape"),
        KL_METHOD_AUTO(BlendshapeController, RemoveBlendshape, "Remove blendshape"),
        KL_METHOD_AUTO(BlendshapeController, GetBlendshapeCount, "Get blendshape count"),
        KL_METHOD_AUTO(BlendshapeController, GetCapacity, "Get capacity"),
        KL_METHOD_AUTO(BlendshapeController, SetWeight, "Set weight"),
        KL_METHOD_AUTO(BlendshapeController, GetWeight, "Get weight"),
        KL_METHOD_AUTO(BlendshapeController, ResetWeights, "Reset weights"),
        KL_METHOD_AUTO(BlendshapeController, Update, "Update"),
        KL_METHOD_AUTO(BlendshapeController, ApplyTo, "Apply to")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(BlendshapeController)
        KL_CTOR(BlendshapeController, IEasyEaseAnimator *, std::size_t)
    KL_END_DESCRIBE(BlendshapeController)
};

} // namespace koilo
