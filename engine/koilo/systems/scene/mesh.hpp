// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file mesh.h
 * @brief Defines the `Mesh` class, representing a 3D object with geometry, material, and transformation data.
 *
 * This class provides methods for managing 3D objects, including transformations,
 * material assignments, and geometric modifications.
 *
 * @date 22/12/2024
 * @version 1.0
 * @author Coela Can't
 */

#pragma once

#include <koilo/systems/render/material/imaterial.hpp>
#include <koilo/core/math/transform.hpp>
#include <koilo/assets/model/trianglegroup.hpp>
#include <koilo/assets/model/statictrianglegroup.hpp>
#include <koilo/registry/reflect_macros.hpp>


namespace koilo {

// Forward declaration
class BlendshapeController;
class Skeleton;
struct SkinData;

/**
 * @class Mesh
 * @brief Represents a 3D object with geometry, material, and transformation data.
 *
 * The `Mesh` class manages the geometric representation, transformation, and material
 * properties of a 3D object. It provides methods to enable or disable the object, modify
 * its transformations, reset its geometry, and retrieve its material or geometry data.
 */
class Mesh {
private:
    Transform transform;                     ///< Transform object representing the object's position, rotation, and scale.
    IStaticTriangleGroup* originalTriangles; ///< Pointer to the static representation of the object's geometry.
    ITriangleGroup* modifiedTriangles;       ///< Pointer to the modifiable representation of the object's geometry.
    IMaterial* material;                      ///< Pointer to the material assigned to the object.
    BlendshapeController* blendshapeController = nullptr; ///< Optional blendshape controller for vertex morphing.
    Skeleton* skeleton_ = nullptr;            ///< Optional skeleton for bone-driven deformation.
    SkinData* skinData_ = nullptr;            ///< Optional per-vertex bone weights.
    bool enabled = true;                     ///< Indicates whether the object is currently enabled.
    bool transformDirty_ = true;             ///< True when transform needs reapplication to vertices.
    uint32_t gpuVersion_ = 0;                ///< Monotonic version counter for GPU upload caching.

public:
    /**
     * @brief Constructs an `Mesh` instance.
     *
     * @param originalTriangles Pointer to the static representation of the object's geometry.
     * @param modifiedTriangles Pointer to the modifiable representation of the object's geometry.
     * @param material Pointer to the material assigned to the object.
     */
    Mesh(IStaticTriangleGroup* originalTriangles, ITriangleGroup* modifiedTriangles, IMaterial* material);

    /**
     * @brief Destructor for `Mesh`.
     */
    ~Mesh();

    /**
     * @brief Enables the object, making it visible and active.
     */
    void Enable();

    /**
     * @brief Disables the object, making it invisible and inactive.
     */
    void Disable();

    /**
     * @brief Checks if the object is enabled.
     * @return True if the object is enabled, otherwise false.
     */
    bool IsEnabled();

    /**
     * @brief Checks if the attached StaticTriangleGroup has a UV.
     * @return True if the StaticTriangleGroup has a UV.
     */
    bool HasUV();

    /**
     * @brief Retrieves the array of UV vertices in the triangle group.
     * @return A pointer to the array of Vector2D UV vertices.
     */
    const Vector2D* GetUVVertices();

    /**
     * @brief Retrieves the index group for the UV vertices.
     * @return A pointer to the IndexGroup array for UV vertices.
     */
    const IndexGroup* GetUVIndexGroup();

    /**
     * @brief Retrieves the object's center offset.
     * @return A `Vector3D` representing the object's center offset.
     */
    Vector3D GetCenterOffset();

    /**
     * @brief Retrieves the minimum and maximum dimensions of the object.
     *
     * @param minimum Reference to a `Vector3D` to store the minimum dimensions.
     * @param maximum Reference to a `Vector3D` to store the maximum dimensions.
     */
    void GetMinMaxDimensions(Vector3D& minimum, Vector3D& maximum);

    /**
     * @brief Retrieves the size of the object.
     * @return A `Vector3D` representing the object's size.
     */
    Vector3D GetSize();

    /**
     * @brief Retrieves the object's transformation data.
     * @return Pointer to the `Transform` object.
     */
    Transform* GetTransform();

    /**
     * @brief Sets the object's transformation data.
     * 
     * @param t Reference to the new `Transform`.
     */
    void SetTransform(Transform& t);

    /**
     * @brief Resets the object's vertices to their original positions.
     */
    void ResetVertices();

    /**
     * @brief Updates the object's geometry based on its transformation data.
     * Skips recomputation if the transform has not changed since last call.
     */
    void UpdateTransform();

    /**
     * @brief Marks the transform as dirty so UpdateTransform will recompute vertices.
     * Call this after modifying the transform via GetTransform()->Set*() directly.
     */
    void MarkTransformDirty();

    /**
     * @brief Returns the GPU version counter (incremented on vertex changes).
     */
    uint32_t GetGPUVersion() const { return gpuVersion_; }

    /**
     * @brief Retrieves the modifiable geometry of the object.
     * @return Pointer to the `ITriangleGroup` representing the object's modifiable geometry.
     */
    ITriangleGroup* GetTriangleGroup();

    /**
     * @brief Retrieves the material assigned to the object.
     * @return Pointer to the `Material` assigned to the object.
     */
    IMaterial* GetMaterial();

    /**
     * @brief Sets the material for the object.
     *
     * @param material Pointer to the new `Material` to be assigned.
     */
    void SetMaterial(IMaterial* material);

    /**
     * @brief Sets the blendshape controller for vertex morphing.
     *
     * @param controller Pointer to the BlendshapeController (can be nullptr to disable)
     */
    void SetBlendshapeController(BlendshapeController* controller);

    /**
     * @brief Gets the current blendshape controller.
     *
     * @return Pointer to the BlendshapeController, or nullptr if not set
     */
    BlendshapeController* GetBlendshapeController();

    /**
     * @brief Updates blendshapes if a controller is set.
     * 
     * This calls the controller's Update() method which will reset vertices
     * and apply all weighted blendshapes.
     */
    void UpdateBlendshapes();

    /**
     * @brief Sets the skeleton for bone-driven vertex deformation.
     * @param skeleton Pointer to a Skeleton (can be nullptr to disable).
     */
    void SetSkeleton(Skeleton* skeleton);

    /**
     * @brief Gets the current skeleton.
     * @return Pointer to the Skeleton, or nullptr if not set.
     */
    Skeleton* GetSkeleton();

    /**
     * @brief Sets the skin data (per-vertex bone weights).
     * @param skinData Pointer to a SkinData (can be nullptr to disable).
     */
    void SetSkinData(SkinData* skinData);

    /**
     * @brief Gets the current skin data.
     * @return Pointer to the SkinData, or nullptr if not set.
     */
    SkinData* GetSkinData();

    /**
     * @brief Applies skeletal skinning to vertex positions.
     *
     * Resets vertices to bind pose, applies blendshapes if any,
     * then applies bone-weighted deformation.
     * Call after updating bone transforms and before rendering.
     */
    void UpdateSkinning();

    KL_BEGIN_FIELDS(Mesh)
        /* No reflected fields. */
    KL_END_FIELDS

    KL_BEGIN_METHODS(Mesh)
        KL_METHOD_AUTO(Mesh, Enable, "Enable"),
        KL_METHOD_AUTO(Mesh, Disable, "Disable"),
        KL_METHOD_AUTO(Mesh, IsEnabled, "Is enabled"),
        KL_METHOD_AUTO(Mesh, HasUV, "Has uv"),
        KL_METHOD_AUTO(Mesh, GetUVVertices, "Get uvvertices"),
        KL_METHOD_AUTO(Mesh, GetUVIndexGroup, "Get uvindex group"),
        KL_METHOD_AUTO(Mesh, GetCenterOffset, "Get center offset"),
        KL_METHOD_AUTO(Mesh, GetMinMaxDimensions, "Get min max dimensions"),
        KL_METHOD_AUTO(Mesh, GetSize, "Get size"),
        KL_METHOD_AUTO(Mesh, GetTransform, "Get transform"),
        KL_METHOD_AUTO(Mesh, SetTransform, "Set transform"),
        KL_METHOD_AUTO(Mesh, ResetVertices, "Reset vertices"),
        KL_METHOD_AUTO(Mesh, UpdateTransform, "Update transform"),
        KL_METHOD_AUTO(Mesh, MarkTransformDirty, "Mark transform dirty"),
        KL_METHOD_AUTO(Mesh, GetTriangleGroup, "Get triangle group"),
        KL_METHOD_AUTO(Mesh, GetMaterial, "Get material"),
        KL_METHOD_AUTO(Mesh, SetMaterial, "Set material"),
        KL_METHOD_AUTO(Mesh, SetBlendshapeController, "Set blendshape controller"),
        KL_METHOD_AUTO(Mesh, GetBlendshapeController, "Get blendshape controller"),
        KL_METHOD_AUTO(Mesh, UpdateBlendshapes, "Update blendshapes"),
        KL_METHOD_AUTO(Mesh, SetSkeleton, "Set skeleton"),
        KL_METHOD_AUTO(Mesh, GetSkeleton, "Get skeleton"),
        KL_METHOD_AUTO(Mesh, UpdateSkinning, "Update skinning")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(Mesh)
        KL_CTOR(Mesh, IStaticTriangleGroup *, ITriangleGroup *, IMaterial *)
    KL_END_DESCRIBE(Mesh)

};

} // namespace koilo
