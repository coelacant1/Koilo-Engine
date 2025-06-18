/**
 * @file triangle3d.hpp
 * @brief Immutable 3-D triangle, normal + ray hit test.
 * @date  18/06/2025
 * @author Coela Can't
 */
#pragma once

#include "../../math/vector2d.hpp"
#include "../../math/vector3d.hpp"

/**
 * @class Triangle3D
 * @brief Pure geometry: three vertices, optional UV pointers, normal math.
 */
class Triangle3D
{
public:
    Vector3D p1;            ///< first vertex
    Vector3D p2;            ///< second vertex
    Vector3D p3;            ///< third vertex

    const Vector2D* uv1;    ///< UV for p1, may be nullptr
    const Vector2D* uv2;    ///< UV for p2, may be nullptr
    const Vector2D* uv3;    ///< UV for p3, may be nullptr

    /** @brief Default ctor (degenerate at origin, no UV). */
    Triangle3D();

    /** @brief Construct from explicit vertices. */
    Triangle3D(const Vector3D& p1In,
               const Vector3D& p2In,
               const Vector3D& p3In,
               const Vector2D* uv1In = nullptr,
               const Vector2D* uv2In = nullptr,
               const Vector2D* uv3In = nullptr);

    /** @brief Face normal (counter-clockwise). */
    Vector3D GetNormal() const;

    /**
     * @brief Ray-triangle intersection (Möller-Trumbore).
     * @param rayOrigin  ray start
     * @param rayDir     ray direction (unit length recommended)
     * @param tOut       out param: distance along ray (optional)
     * @param uvOut      out param: barycentric UV (optional, size 2)
     * @return true if hit, else false
     */
    bool IntersectRay(const Vector3D& rayOrigin,
                      const Vector3D& rayDir,
                      float*          tOut = nullptr,
                      float*          uvOut = nullptr) const;
};
