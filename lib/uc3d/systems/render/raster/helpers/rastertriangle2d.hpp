/**
 * @file rastertriangle2d.hpp
 * @brief Extends Triangle2D with rendering-specific data for rasterization.
 * @date  26/06/2025
 * @author Coela Can't
 */
#pragma once

#include "../../../../core/geometry/2d/triangle.hpp"
#include "../../../../core/math/transform.hpp"
#include "../../../../core/math/vector3d.hpp"
#include "../../../render/material/imaterial.hpp"
#include "../../../../core/geometry/2d/rectangle.hpp"
#include "rastertriangle3d.hpp"

/**
 * @class RasterTriangle2D
 * @brief A 2D triangle with data needed for rasterization, like depth, UVs, and materials.
 *
 * This class inherits the basic geometry from Triangle2D and adds all the
 * properties required to render it, including pointers to the original 3D data,
 * material information, and pre-calculated values for efficient intersection tests.
 */
class RasterTriangle2D : public Triangle2D {
public:
    // --- Rendering & 3D Link Data ---
    const Vector3D* t3p1;   ///< Pointer to the original first vertex in 3D space.
    const Vector3D* t3p2;   ///< Pointer to the original second vertex in 3D space.
    const Vector3D* t3p3;   ///< Pointer to the original third vertex in 3D space.
    const Vector3D* normal; ///< Pointer to the original normal vector of the 3D triangle.
    IMaterial* material;     ///< Material assigned to the triangle for shading.

    // --- UV Mapping Data ---
    const Vector2D* p1UV;   ///< UV coordinates of the first vertex.
    const Vector2D* p2UV;   ///< UV coordinates of the second vertex.
    const Vector2D* p3UV;   ///< UV coordinates of the third vertex.
    bool hasUV;             ///< Flag indicating if the triangle has UV coordinates.

    // --- Pre-calculated values for efficiency ---
    float averageDepth;     ///< Average depth of the triangle's vertices for z-buffering.
    float denominator;      ///< Precomputed denominator for barycentric coordinate calculations.
    Vector2D v0, v1;        ///< Edge vectors for barycentric calculations.
    Rectangle2D bounds;     ///< Axis-aligned bounding box for fast spatial queries (e.g., QuadTree).

    /**
     * @brief Default constructor. Initializes all pointers to nullptr.
     */
    RasterTriangle2D();

    /**
     * @brief Projects a 3D triangle to a 2D raster triangle using a camera transform.
     *
     * This is the primary constructor for creating a renderable 2D triangle from 3D scene data.
     * It handles the projection, calculates depth, copies material/UV data, and pre-computes
     * values for efficient rasterization.
     *
     * @param camTransform The transform of the camera.
     * @param lookDirection The look direction of the camera.
     * @param sourceTriangle The source 3D triangle.
     * @param mat The material to assign.
     */
    RasterTriangle2D(const Transform& camTransform, const Quaternion& lookDirection,
                     const RasterTriangle3D& sourceTriangle, IMaterial* mat);

    /**
     * @brief Checks for intersection with a point using efficient barycentric coordinates.
     *
     * @param x The x-coordinate of the point.
     * @param y The y-coordinate of the point.
     * @param u [out] The first barycentric coordinate.
     * @param v [out] The second barycentric coordinate.
     * @param w [out] The third barycentric coordinate.
     * @return true if the point is inside the triangle, false otherwise.
     */
    bool GetBarycentricCoords(float x, float y, float& u, float& v, float& w) const;

    /**
     * @brief Checks if the triangle's bounding box overlaps with another rectangle.
     * @param otherBounds The rectangle to test against.
     * @return true if the bounds overlap, false otherwise.
     */
    bool Overlaps(const Rectangle2D& otherBounds) const;

    /**
     * @brief Gets the assigned material.
     */
    IMaterial* GetMaterial() const;

    /**
     * @brief Provides a string representation of the 2D vertices.
     */
    uc3d::UString ToString() const;

private:
    /**
     * @brief Private helper to calculate the bounding box and barycentric denominator.
     */
    void CalculateBoundsAndDenominator();
};
