// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file boxcollider.hpp
 * @brief Box collider extending the geometry Cube class.
 *
 * @date 11/10/2025
 * @author Coela
 */

#pragma once

#include "collider.hpp"
#include <koilo/core/geometry/3d/cube.hpp>
#include "../../registry/reflect_macros.hpp"

namespace koilo {

/**
 * @class BoxCollider
 * @brief Axis-aligned box collider that extends the Cube geometry class with physics functionality.
 *
 * This class inherits from both Collider (for physics interface) and Cube (for geometry).
 */
class BoxCollider : public Collider, public Cube {
public:
    /**
     * @brief Default constructor.
     */
    BoxCollider();

    /**
     * @brief Constructor with center and size.
     * @param center Center position.
     * @param size Size (extents) of the box.
     */
    BoxCollider(const Vector3D& center, const Vector3D& size);

    /**
     * @brief Destructor.
     */
    ~BoxCollider() override;

    // === Collider Interface Implementation ===

    /**
     * @brief Performs raycast against box (AABB) using origin/direction.
     */
    bool Raycast(const Vector3D& origin, const Vector3D& direction,
                 RaycastHit& hit, float maxDistance);

    /**
     * @brief Performs raycast against box (AABB) using a Ray object.
     */
    bool Raycast(const Ray& ray, RaycastHit& hit, float maxDistance) override;

    /**
     * @brief Checks if point is inside box.
     */
    bool ContainsPoint(const Vector3D& point) override;

    /**
     * @brief Gets closest point on box surface.
     */
    Vector3D ClosestPoint(const Vector3D& point) override;

    /**
     * @brief Gets box center position.
     */
    Vector3D GetPosition() const override;

    /**
     * @brief Sets box center position.
     */
    void SetPosition(const Vector3D& pos) override;

    // Script-friendly raycast: returns hit/miss without out-param
    bool ScriptRaycast(const Vector3D& origin, const Vector3D& direction, float maxDistance) {
        RaycastHit hit;
        return Raycast(Ray(origin, direction), hit, maxDistance);
    }

    KL_BEGIN_FIELDS(BoxCollider)
        // Inherits fields from Collider and Cube
    KL_END_FIELDS

    KL_BEGIN_METHODS(BoxCollider)
        KL_METHOD_AUTO(BoxCollider, ContainsPoint, "Contains point"),
        KL_METHOD_AUTO(BoxCollider, ClosestPoint, "Closest point"),
        KL_METHOD_AUTO(BoxCollider, GetPosition, "Get position"),
        KL_METHOD_AUTO(BoxCollider, SetPosition, "Set position"),
        KL_METHOD_AUTO(BoxCollider, ScriptRaycast, "Script-friendly raycast")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(BoxCollider)
        KL_CTOR0(BoxCollider),
        KL_CTOR(BoxCollider, Vector3D, Vector3D)
    KL_END_DESCRIBE(BoxCollider)
};

} // namespace koilo
