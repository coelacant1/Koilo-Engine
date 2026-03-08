/**
 * @file capsulecollider.hpp
 * @brief Capsule collider for physics collision detection.
 *
 * @date 11/10/2025
 * @author Coela
 */

#pragma once

#include "collider.hpp"

namespace ptx {

/**
 * @class CapsuleCollider
 * @brief Capsule-shaped collider (cylinder with hemispherical ends).
 */
class CapsuleCollider : public Collider {
private:
    Vector3D centerPosition;  ///< Center position of capsule
    float radius;             ///< Radius of capsule
    float height;             ///< Total height (including hemispherical ends)

public:
    /**
     * @brief Default constructor.
     */
    CapsuleCollider();

    /**
     * @brief Constructor with parameters.
     * @param position Center position.
     * @param radius Capsule radius.
     * @param height Total height.
     */
    CapsuleCollider(const Vector3D& position, float radius, float height);

    /**
     * @brief Virtual destructor.
     */
    virtual ~CapsuleCollider();

    // === Accessors ===

    /**
     * @brief Gets the radius.
     */
    float GetRadius() const { return radius; }

    /**
     * @brief Sets the radius.
     */
    void SetRadius(float r);

    /**
     * @brief Gets the height.
     */
    float GetHeight() const { return height; }

    /**
     * @brief Sets the height.
     */
    void SetHeight(float h);

    /**
     * @brief Gets the line segment endpoints (cylinder axis).
     * @param p1 Output: bottom hemisphere center.
     * @param p2 Output: top hemisphere center.
     */
    void GetSegment(Vector3D& p1, Vector3D& p2) const;

    // === Collider Interface Implementation ===

    /**
     * @brief Performs raycast against capsule.
     */
    virtual bool Raycast(const Vector3D& origin, const Vector3D& direction,
                        RaycastHit& hit, float maxDistance) override;

    /**
     * @brief Checks if point is inside capsule.
     */
    virtual bool ContainsPoint(const Vector3D& point) override;

    /**
     * @brief Gets closest point on capsule surface.
     */
    virtual Vector3D ClosestPoint(const Vector3D& point) override;

    /**
     * @brief Gets capsule center position.
     */
    virtual Vector3D GetPosition() const override;

    /**
     * @brief Sets capsule center position.
     */
    virtual void SetPosition(const Vector3D& pos) override;

    PTX_BEGIN_FIELDS(CapsuleCollider)
        PTX_FIELD(CapsuleCollider, radius, "Radius", 0, 0),
        PTX_FIELD(CapsuleCollider, height, "Height", 0, 0)
    PTX_END_FIELDS

    PTX_BEGIN_METHODS(CapsuleCollider)
        PTX_METHOD_AUTO(CapsuleCollider, GetRadius, "Get radius"),
        PTX_METHOD_AUTO(CapsuleCollider, SetRadius, "Set radius"),
        PTX_METHOD_AUTO(CapsuleCollider, GetHeight, "Get height"),
        PTX_METHOD_AUTO(CapsuleCollider, SetHeight, "Set height"),
        PTX_METHOD_AUTO(CapsuleCollider, Raycast, "Raycast"),
        PTX_METHOD_AUTO(CapsuleCollider, ContainsPoint, "Contains point"),
        PTX_METHOD_AUTO(CapsuleCollider, ClosestPoint, "Closest point")
    PTX_END_METHODS

    PTX_BEGIN_DESCRIBE(CapsuleCollider)
        PTX_CTOR0(CapsuleCollider),
        PTX_CTOR(CapsuleCollider, Vector3D, float, float)
    PTX_END_DESCRIBE(CapsuleCollider)
};

} // namespace ptx
