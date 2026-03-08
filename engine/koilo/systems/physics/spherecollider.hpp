// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file spherecollider.hpp
 * @brief Sphere collider extending the geometry Sphere class.
 *
 * @date 11/10/2025
 * @author Coela
 */

#pragma once

#include "collider.hpp"
#include <koilo/core/geometry/3d/sphere.hpp>
#include "../../registry/reflect_macros.hpp"

namespace koilo {

/**
 * @class SphereCollider
 * @brief Sphere-shaped collider that extends the Sphere geometry class with physics functionality.
 *
 * This class inherits from both Collider (for physics interface) and Sphere (for geometry).
 */
class SphereCollider : public Collider, public Sphere {
public:
    /**
     * @brief Default constructor.
     */
    SphereCollider();

    /**
     * @brief Constructor with position and radius.
     * @param position Center position.
     * @param radius Sphere radius.
     */
    SphereCollider(const Vector3D& position, float radius);

    /**
     * @brief Destructor.
     */
    ~SphereCollider() override;

    // === Collider Interface Implementation ===

    /**
     * @brief Performs raycast against sphere using Ray object.
     */
    bool Raycast(const Ray& ray, RaycastHit& hit, float maxDistance) override;

    /**
     * @brief Performs raycast against sphere (legacy interface).
     */
    bool Raycast(const Vector3D& origin, const Vector3D& direction,
                 RaycastHit& hit, float maxDistance) {
        return Raycast(Ray(origin, direction), hit, maxDistance);
    }

    /**
     * @brief Checks if point is inside sphere.
     */
    bool ContainsPoint(const Vector3D& point) override;

    /**
     * @brief Gets closest point on sphere surface.
     */
    Vector3D ClosestPoint(const Vector3D& point) override;

    /**
     * @brief Gets sphere center position.
     */
    Vector3D GetPosition() const override;

    /**
     * @brief Sets sphere center position.
     */
    void SetPosition(const Vector3D& pos) override;

    // Script-friendly raycast: returns hit/miss without out-param
    bool ScriptRaycast(const Vector3D& origin, const Vector3D& direction, float maxDistance) {
        RaycastHit hit;
        return Raycast(Ray(origin, direction), hit, maxDistance);
    }

    KL_BEGIN_FIELDS(SphereCollider)
        // Inherits fields from Collider and Sphere
    KL_END_FIELDS

    KL_BEGIN_METHODS(SphereCollider)
        KL_METHOD_AUTO(SphereCollider, ContainsPoint, "Contains point"),
        KL_METHOD_AUTO(SphereCollider, ClosestPoint, "Closest point"),
        KL_METHOD_AUTO(SphereCollider, GetPosition, "Get position"),
        KL_METHOD_AUTO(SphereCollider, SetPosition, "Set position"),
        KL_METHOD_AUTO(SphereCollider, ScriptRaycast, "Script-friendly raycast")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(SphereCollider)
        KL_CTOR0(SphereCollider),
        KL_CTOR(SphereCollider, Vector3D, float)
    KL_END_DESCRIBE(SphereCollider)
};

} // namespace koilo
