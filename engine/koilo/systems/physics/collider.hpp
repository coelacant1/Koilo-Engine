/**
 * @file collider.hpp
 * @brief Base collider class for physics collision detection.
 *
 * @date 11/10/2025
 * @author Coela
 */

#pragma once

#include <string>
#include "../../core/math/vector3d.hpp"
#include "../../core/math/rotation.hpp"
#include "physicsmaterial.hpp"
#include "raycasthit.hpp"
#include "../../registry/reflect_macros.hpp"

namespace ptx {

/**
 * @enum ColliderType
 * @brief Type of collider shape.
 */
enum class ColliderType {
    Sphere,
    Box,
    Capsule,
    Plane,
    Custom
};

/**
 * @class Collider
 * @brief Base class for all physics colliders.
 *
 * This class provides a common interface for collision detection and extends
 * the existing geometry classes with physics-specific functionality.
 */
class Collider {
protected:
    ColliderType type;           ///< Type of collider
    bool isTrigger;              ///< If true, collider is a trigger (no physics response)
    bool isEnabled;              ///< If true, collider is active
    int layer;                   ///< Collision layer (0-31)
    std::string tag;             ///< User-defined tag for identification
    PhysicsMaterial material;    ///< Physics material properties
    void* owner;                 ///< Pointer to owning game object (optional)

public:
    /**
     * @brief Constructor.
     * @param type Type of collider.
     */
    explicit Collider(ColliderType type);

    /**
     * @brief Virtual destructor.
     */
    virtual ~Collider();

    // === Type and State ===

    /**
     * @brief Gets the collider type.
     */
    ColliderType GetType() const { return type; }

    /**
     * @brief Checks if collider is a trigger.
     */
    bool IsTrigger() const { return isTrigger; }

    /**
     * @brief Sets trigger state.
     */
    void SetTrigger(bool trigger) { isTrigger = trigger; }

    /**
     * @brief Checks if collider is enabled.
     */
    bool IsEnabled() const { return isEnabled; }

    /**
     * @brief Sets enabled state.
     */
    void SetEnabled(bool enabled) { isEnabled = enabled; }

    // === Layer ===

    /**
     * @brief Gets the collision layer.
     */
    int GetLayer() const { return layer; }

    /**
     * @brief Sets the collision layer.
     * @param layer Layer index (0-31).
     */
    void SetLayer(int layer);

    // === Tag ===

    /**
     * @brief Gets the collider tag.
     */
    std::string GetTag() const { return tag; }

    /**
     * @brief Sets the collider tag.
     */
    void SetTag(const std::string& t) { tag = t; }

    // === Material ===

    /**
     * @brief Gets the physics material.
     */
    PhysicsMaterial GetMaterial() const { return material; }

    /**
     * @brief Sets the physics material.
     */
    void SetMaterial(const PhysicsMaterial& mat) { material = mat; }

    // === Owner ===

    /**
     * @brief Gets the owner pointer.
     */
    void* GetOwner() const { return owner; }

    /**
     * @brief Sets the owner pointer.
     */
    void SetOwner(void* o) { owner = o; }

    // === Virtual Interface (Pure Virtual) ===

    /**
     * @brief Performs raycast against this collider.
     * @param origin Ray origin.
     * @param direction Ray direction (must be normalized).
     * @param hit Output hit information.
     * @param maxDistance Maximum ray distance.
     * @return True if ray hits collider.
     */
    virtual bool Raycast(const Vector3D& origin, const Vector3D& direction,
                         RaycastHit& hit, float maxDistance) = 0;

    /**
     * @brief Checks if a point is inside the collider.
     * @param point Point in world space.
     * @return True if point is inside.
     */
    virtual bool ContainsPoint(const Vector3D& point) = 0;

    /**
     * @brief Gets the closest point on the collider surface to a given point.
     * @param point Point in world space.
     * @return Closest point on collider surface.
     */
    virtual Vector3D ClosestPoint(const Vector3D& point) = 0;

    /**
     * @brief Gets the center position of the collider.
     */
    virtual Vector3D GetPosition() const = 0;

    /**
     * @brief Sets the center position of the collider.
     */
    virtual void SetPosition(const Vector3D& pos) = 0;

    PTX_BEGIN_FIELDS(Collider)
        PTX_FIELD(Collider, isTrigger, "Is trigger", 0, 1),
        PTX_FIELD(Collider, isEnabled, "Is enabled", 0, 1),
        PTX_FIELD(Collider, layer, "Layer", 0, 0),
        PTX_FIELD(Collider, tag, "Tag", 0, 0),
        PTX_FIELD(Collider, material, "Material", 0, 0)
    PTX_END_FIELDS

    PTX_BEGIN_METHODS(Collider)
        PTX_METHOD_AUTO(Collider, GetType, "Get type"),
        PTX_METHOD_AUTO(Collider, IsTrigger, "Is trigger"),
        PTX_METHOD_AUTO(Collider, SetTrigger, "Set trigger"),
        PTX_METHOD_AUTO(Collider, IsEnabled, "Is enabled"),
        PTX_METHOD_AUTO(Collider, SetEnabled, "Set enabled"),
        PTX_METHOD_AUTO(Collider, GetLayer, "Get layer"),
        PTX_METHOD_AUTO(Collider, SetLayer, "Set layer"),
        PTX_METHOD_AUTO(Collider, GetTag, "Get tag"),
        PTX_METHOD_AUTO(Collider, SetTag, "Set tag"),
        PTX_METHOD_AUTO(Collider, GetMaterial, "Get material"),
        PTX_METHOD_AUTO(Collider, SetMaterial, "Set material"),
        PTX_METHOD_AUTO(Collider, ContainsPoint, "Contains point")
    PTX_END_METHODS

    PTX_BEGIN_DESCRIBE(Collider)
        // Abstract class, no constructors exposed
    PTX_END_DESCRIBE(Collider)
};

} // namespace ptx
