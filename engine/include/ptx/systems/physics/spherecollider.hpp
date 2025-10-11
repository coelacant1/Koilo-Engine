/**
 * @file spherecollider.hpp
 * @brief Sphere collider extending the geometry Sphere class.
 *
 * @date 11/10/2025
 * @author Coela
 */

#pragma once

#include "collider.hpp"
#include "../../core/geometry/3d/sphere.hpp"

namespace ptx {

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
     * @brief Virtual destructor.
     */
    virtual ~SphereCollider();

    // === Collider Interface Implementation ===

    /**
     * @brief Performs raycast against sphere.
     */
    virtual bool Raycast(const Vector3D& origin, const Vector3D& direction,
                        RaycastHit& hit, float maxDistance) override;

    /**
     * @brief Checks if point is inside sphere.
     */
    virtual bool ContainsPoint(const Vector3D& point) override;

    /**
     * @brief Gets closest point on sphere surface.
     */
    virtual Vector3D ClosestPoint(const Vector3D& point) override;

    /**
     * @brief Gets sphere center position.
     */
    virtual Vector3D GetPosition() const override;

    /**
     * @brief Sets sphere center position.
     */
    virtual void SetPosition(const Vector3D& pos) override;

    PTX_BEGIN_FIELDS(SphereCollider)
        // Inherits fields from Collider and Sphere
    PTX_END_FIELDS

    PTX_BEGIN_METHODS(SphereCollider)
        PTX_METHOD_AUTO(SphereCollider, Raycast, "Raycast"),
        PTX_METHOD_AUTO(SphereCollider, ContainsPoint, "Contains point"),
        PTX_METHOD_AUTO(SphereCollider, ClosestPoint, "Closest point")
    PTX_END_METHODS

    PTX_BEGIN_DESCRIBE(SphereCollider)
        PTX_CTOR0(SphereCollider),
        PTX_CTOR(SphereCollider, Vector3D, float)
    PTX_END_DESCRIBE(SphereCollider)
};

} // namespace ptx
