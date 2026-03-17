// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file transformcomponent.hpp
 * @brief Transform component for ECS (wraps existing Transform class).
 *
 * @date 11/10/2025
 * @author Coela
 */

#pragma once

#include <koilo/core/math/transform.hpp>
#include <koilo/registry/reflect_macros.hpp>

namespace koilo {

/**
 * @struct TransformComponent
 * @brief Wraps the existing Transform class for use in ECS.
 *
 * This component uses the existing Transform class from koilo::core::math
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

    KL_BEGIN_FIELDS(TransformComponent)
        KL_FIELD(TransformComponent, transform, "Transform", 0, 0)
    KL_END_FIELDS

    KL_BEGIN_METHODS(TransformComponent)
        KL_METHOD_AUTO(TransformComponent, GetPosition, "Get position"),
        KL_METHOD_AUTO(TransformComponent, SetPosition, "Set position"),
        KL_METHOD_AUTO(TransformComponent, GetRotation, "Get rotation"),
        KL_METHOD_AUTO(TransformComponent, SetRotation, "Set rotation"),
        KL_METHOD_AUTO(TransformComponent, GetScale, "Get scale"),
        KL_METHOD_AUTO(TransformComponent, SetScale, "Set scale")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(TransformComponent)
        KL_CTOR0(TransformComponent),
        KL_CTOR(TransformComponent, Vector3D),
        KL_CTOR(TransformComponent, Transform),
        KL_CTOR(TransformComponent, Quaternion, Vector3D, Vector3D)
    KL_END_DESCRIBE(TransformComponent)
};

} // namespace koilo
