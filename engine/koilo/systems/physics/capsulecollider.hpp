// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file capsulecollider.hpp
 * @brief Capsule collider for physics collision detection.
 *
 * @date 11/10/2025
 * @author Coela
 */

#pragma once

#include "collider.hpp"
#include "../../registry/reflect_macros.hpp"

namespace koilo {

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
     * @brief Destructor.
     */
    ~CapsuleCollider() override;

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
    bool Raycast(const Vector3D& origin, const Vector3D& direction,
                  RaycastHit& hit, float maxDistance);

    bool Raycast(const Ray& ray, RaycastHit& hit, float maxDistance) override;

    /**
     * @brief Checks if point is inside capsule.
     */
    bool ContainsPoint(const Vector3D& point) override;

    /**
     * @brief Gets closest point on capsule surface.
     */
    Vector3D ClosestPoint(const Vector3D& point) override;

    /**
     * @brief Gets capsule center position.
     */
    Vector3D GetPosition() const override;

    /**
     * @brief Sets capsule center position.
     */
    void SetPosition(const Vector3D& pos) override;

    // Script-friendly raycast: returns hit/miss without out-param
    bool ScriptRaycast(const Vector3D& origin, const Vector3D& direction, float maxDistance) {
        RaycastHit hit;
        return Raycast(Ray(origin, direction), hit, maxDistance);
    }

    KL_BEGIN_FIELDS(CapsuleCollider)
        KL_FIELD(CapsuleCollider, radius, "Radius", 0, 0),
        KL_FIELD(CapsuleCollider, height, "Height", 0, 0)
    KL_END_FIELDS

    KL_BEGIN_METHODS(CapsuleCollider)
        KL_METHOD_AUTO(CapsuleCollider, GetRadius, "Get radius"),
        KL_METHOD_AUTO(CapsuleCollider, SetRadius, "Set radius"),
        KL_METHOD_AUTO(CapsuleCollider, GetHeight, "Get height"),
        KL_METHOD_AUTO(CapsuleCollider, SetHeight, "Set height"),
        KL_METHOD_AUTO(CapsuleCollider, ContainsPoint, "Contains point"),
        KL_METHOD_AUTO(CapsuleCollider, ClosestPoint, "Closest point"),
        KL_METHOD_AUTO(CapsuleCollider, GetPosition, "Get position"),
        KL_METHOD_AUTO(CapsuleCollider, SetPosition, "Set position"),
        KL_METHOD_AUTO(CapsuleCollider, ScriptRaycast, "Script-friendly raycast")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(CapsuleCollider)
        KL_CTOR0(CapsuleCollider),
        KL_CTOR(CapsuleCollider, Vector3D, float, float)
    KL_END_DESCRIBE(CapsuleCollider)
};

} // namespace koilo
