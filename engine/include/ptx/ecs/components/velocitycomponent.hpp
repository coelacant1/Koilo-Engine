/**
 * @file velocitycomponent.hpp
 * @brief Velocity component for ECS (linear and angular velocity).
 *
 * @date 11/10/2025
 * @author Coela
 */

#pragma once

#include "../../../core/math/vector3d.hpp"
#include "../../../registry/reflect_macros.hpp"

namespace ptx {

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

    PTX_BEGIN_FIELDS(VelocityComponent)
        PTX_FIELD(VelocityComponent, linear, "Linear", 0, 0),
        PTX_FIELD(VelocityComponent, angular, "Angular", 0, 0)
    PTX_END_FIELDS

    PTX_BEGIN_METHODS(VelocityComponent)
    PTX_END_METHODS

    PTX_BEGIN_DESCRIBE(VelocityComponent)
        PTX_CTOR0(VelocityComponent),
        PTX_CTOR(VelocityComponent, Vector3D),
        PTX_CTOR(VelocityComponent, Vector3D, Vector3D)
    PTX_END_DESCRIBE(VelocityComponent)
};

} // namespace ptx
