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

#include "../render/material/imaterial.hpp"
#include "../../core/math/transform.hpp"
#include "../../assets/model/trianglegroup.hpp"
#include "../../assets/model/statictrianglegroup.hpp"

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
    bool enabled = true;                     ///< Indicates whether the object is currently enabled.

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
     */
    void UpdateTransform();

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
};
