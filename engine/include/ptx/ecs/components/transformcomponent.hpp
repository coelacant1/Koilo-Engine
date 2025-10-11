/**
 * @file transformcomponent.hpp
 * @brief Transform component for ECS (wraps existing Transform class).
 *
 * @date 11/10/2025
 * @author Coela
 */

#pragma once

#include "../../../core/math/transform.hpp"
#include "../../../registry/reflect_macros.hpp"

namespace ptx {

/**
 * @struct TransformComponent
 * @brief Wraps the existing Transform class for use in ECS.
 *
 * This component uses the existing Transform class from ptx::core::math
 * to avoid duplicating functionality.
 */
struct TransformComponent {
    Transform transform;  ///< The transform (position, rotation, scale)

    /**
     * @brief Default constructor.
     */
    TransformComponent()
        : transform() {}

    /**
     * @brief Constructor with position.
     */
    TransformComponent(const Vector3D& position)
        : transform(Vector3D(0, 0, 0), position, Vector3D(1, 1, 1)) {}

    /**
     * @brief Constructor with Transform object.
     */
    TransformComponent(const Transform& t)
        : transform(t) {}

    /**
     * @brief Constructor with rotation, position, scale.
     */
    TransformComponent(const Quaternion& rotation, const Vector3D& position, const Vector3D& scale)
        : transform(rotation, position, scale) {}

    // === Convenience Accessors ===

    /**
     * @brief Gets the position.
     */
    Vector3D GetPosition() const { return transform.GetPosition(); }

    /**
     * @brief Sets the position.
     */
    void SetPosition(const Vector3D& pos) { transform.SetPosition(pos); }

    /**
     * @brief Gets the rotation.
     */
    Quaternion GetRotation() const { return transform.GetRotation(); }

    /**
     * @brief Sets the rotation.
     */
    void SetRotation(const Quaternion& rot) { transform.SetRotation(rot); }

    /**
     * @brief Gets the scale.
     */
    Vector3D GetScale() const { return transform.GetScale(); }

    /**
     * @brief Sets the scale.
     */
    void SetScale(const Vector3D& scl) { transform.SetScale(scl); }

    PTX_BEGIN_FIELDS(TransformComponent)
        PTX_FIELD(TransformComponent, transform, "Transform", 0, 0)
    PTX_END_FIELDS

    PTX_BEGIN_METHODS(TransformComponent)
        PTX_METHOD_AUTO(TransformComponent, GetPosition, "Get position"),
        PTX_METHOD_AUTO(TransformComponent, SetPosition, "Set position"),
        PTX_METHOD_AUTO(TransformComponent, GetRotation, "Get rotation"),
        PTX_METHOD_AUTO(TransformComponent, SetRotation, "Set rotation"),
        PTX_METHOD_AUTO(TransformComponent, GetScale, "Get scale"),
        PTX_METHOD_AUTO(TransformComponent, SetScale, "Set scale")
    PTX_END_METHODS

    PTX_BEGIN_DESCRIBE(TransformComponent)
        PTX_CTOR0(TransformComponent),
        PTX_CTOR(TransformComponent, Vector3D),
        PTX_CTOR(TransformComponent, Transform),
        PTX_CTOR(TransformComponent, Quaternion, Vector3D, Vector3D)
    PTX_END_DESCRIBE(TransformComponent)
};

} // namespace ptx
