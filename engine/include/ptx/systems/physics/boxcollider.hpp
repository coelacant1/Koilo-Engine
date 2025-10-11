/**
 * @file boxcollider.hpp
 * @brief Box collider extending the geometry Cube class.
 *
 * @date 11/10/2025
 * @author Coela
 */

#pragma once

#include "collider.hpp"
#include "../../core/geometry/3d/cube.hpp"

namespace ptx {

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
     * @brief Virtual destructor.
     */
    virtual ~BoxCollider();

    // === Collider Interface Implementation ===

    /**
     * @brief Performs raycast against box (AABB).
     */
    virtual bool Raycast(const Vector3D& origin, const Vector3D& direction,
                        RaycastHit& hit, float maxDistance) override;

    /**
     * @brief Checks if point is inside box.
     */
    virtual bool ContainsPoint(const Vector3D& point) override;

    /**
     * @brief Gets closest point on box surface.
     */
    virtual Vector3D ClosestPoint(const Vector3D& point) override;

    /**
     * @brief Gets box center position.
     */
    virtual Vector3D GetPosition() const override;

    /**
     * @brief Sets box center position.
     */
    virtual void SetPosition(const Vector3D& pos) override;

    PTX_BEGIN_FIELDS(BoxCollider)
        // Inherits fields from Collider and Cube
    PTX_END_FIELDS

    PTX_BEGIN_METHODS(BoxCollider)
        PTX_METHOD_AUTO(BoxCollider, Raycast, "Raycast"),
        PTX_METHOD_AUTO(BoxCollider, ContainsPoint, "Contains point"),
        PTX_METHOD_AUTO(BoxCollider, ClosestPoint, "Closest point")
    PTX_END_METHODS

    PTX_BEGIN_DESCRIBE(BoxCollider)
        PTX_CTOR0(BoxCollider),
        PTX_CTOR(BoxCollider, Vector3D, Vector3D)
    PTX_END_DESCRIBE(BoxCollider)
};

} // namespace ptx
