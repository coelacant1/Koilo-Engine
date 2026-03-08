// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file velocitycomponent.hpp
 * @brief Velocity component for ECS (linear and angular velocity).
 *
 * @date 11/10/2025
 * @author Coela
 */

#pragma once

#include <koilo/core/math/vector3d.hpp>
#include <koilo/registry/reflect_macros.hpp>

namespace koilo {

/**
 * @struct VelocityComponent
 * @brief Linear and angular velocity for an entity.
 */
struct VelocityComponent {
    Vector3D linear;   ///< Linear velocity (m/s)
    Vector3D angular;  ///< Angular velocity (rad/s)

    /**
     * @brief Default constructor.
     */
    VelocityComponent()
        : linear(0, 0, 0), angular(0, 0, 0) {}

    /**
     * @brief Constructor with linear velocity.
     */
    VelocityComponent(const Vector3D& lin)
        : linear(lin), angular(0, 0, 0) {}

    /**
     * @brief Constructor with linear and angular velocity.
     */
    VelocityComponent(const Vector3D& lin, const Vector3D& ang)
        : linear(lin), angular(ang) {}

    KL_BEGIN_FIELDS(VelocityComponent)
        KL_FIELD(VelocityComponent, linear, "Linear", 0, 0),
        KL_FIELD(VelocityComponent, angular, "Angular", 0, 0)
    KL_END_FIELDS

    KL_BEGIN_METHODS(VelocityComponent)
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(VelocityComponent)
        KL_CTOR0(VelocityComponent),
        KL_CTOR(VelocityComponent, Vector3D),
        KL_CTOR(VelocityComponent, Vector3D, Vector3D)
    KL_END_DESCRIBE(VelocityComponent)
};

} // namespace koilo
