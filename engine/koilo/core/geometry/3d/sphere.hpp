// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file sphere.hpp
 * @brief Defines the Sphere geometry primitive.
 *
 * Pure geometric representation of a sphere (position + radius).
 *
 * @date 22/12/2024
 * @version 2.0
 * @author Coela Can't
 */

#pragma once

#include <koilo/core/math/vector3d.hpp>
#include <koilo/core/math/quaternion.hpp>
#include <koilo/registry/reflect_macros.hpp>


namespace koilo {

/**
 * @class Sphere
 * @brief Sphere with position, radius, and optional physics (velocity, collision).
 */
class Sphere {
private:
    Vector3D centerPosition;
    Quaternion previousRotation;
    float radius = 1.0f; ///< Radius of the sphere.

public:
    Vector3D velocity = Vector3D(0, 0, 0); ///< Velocity vector.
    Vector3D position = Vector3D(0, 0, 0); ///< Center position of the sphere.

    /**
     * @brief Constructs a Sphere with a given position and radius.
     * @param position Center position of the sphere.
     * @param radius Radius of the sphere.
     */
    Sphere(Vector3D position, float radius);

    /**
     * @brief Gets the radius of the sphere.
     * @return Radius of the sphere.
     */
    float GetRadius();

    void Update(float dT, Vector3D acceleration, Quaternion rotation);
    bool IsIntersecting(Sphere* bO);
    void Collide(float elasticity, Sphere* bO);

    KL_BEGIN_FIELDS(Sphere)
        KL_FIELD(Sphere, velocity, "Velocity", 0, 0),
        KL_FIELD(Sphere, position, "Position", 0, 0)
    KL_END_FIELDS

    KL_BEGIN_METHODS(Sphere)
        KL_METHOD_AUTO(Sphere, GetRadius, "Get radius"),
        KL_METHOD_AUTO(Sphere, Update, "Update"),
        KL_METHOD_AUTO(Sphere, IsIntersecting, "Is intersecting"),
        KL_METHOD_AUTO(Sphere, Collide, "Collide")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(Sphere)
        KL_CTOR(Sphere, Vector3D, float)
    KL_END_DESCRIBE(Sphere)

};

} // namespace koilo
