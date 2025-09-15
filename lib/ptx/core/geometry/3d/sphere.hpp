/**
 * @file Sphere.h
 * @brief Defines the Sphere class for simulating spherical boundary objects.
 *
 * The Sphere class provides functionality to represent a 3D sphere's position, velocity,
 * and radius, along with methods to handle motion, detect intersections, and resolve collisions.
 *
 * @date 22/12/2024
 * @version 1.0
 * @author Coela Can't
 */

#pragma once

#include "../../math/rotation.hpp"
#include "../../math/vector3d.hpp"
#include "../../../registry/reflect_macros.hpp"

/**
 * @class Sphere
 * @brief Represents a spherical boundary object for motion and collision simulations.
 */
class Sphere {
private:
    Vector3D centerPosition; ///< Center position of the sphere.
    Quaternion previousRotation; ///< Previous rotation of the sphere.
    float radius = 1.0f; ///< Radius of the sphere.

public:
    Vector3D velocity = Vector3D(0, 0, 0); ///< Velocity vector of the sphere.
    Vector3D position = Vector3D(0, 0, 0); ///< Current position of the sphere.

    /**
     * @brief Constructs a Sphere with a given object, position, and radius.
     * @param position Initial position of the sphere.
     * @param radius Radius of the sphere.
     */
    Sphere(Vector3D position, float radius);

    /**
     * @brief Gets the radius of the sphere.
     * @return Radius of the sphere.
     */
    float GetRadius();

    /**
     * @brief Updates the sphere's position and velocity based on acceleration and rotation.
     * @param dT Time step in seconds.
     * @param acceleration Acceleration vector applied to the sphere.
     * @param rotation Rotation quaternion applied to the sphere.
     */
    void Update(float dT, Vector3D acceleration, Quaternion rotation);

    /**
     * @brief Checks if the sphere intersects with another Sphere.
     * @param bO Pointer to the other Sphere.
     * @return True if the spheres intersect, false otherwise.
     */
    bool IsIntersecting(Sphere* bO);

    /**
     * @brief Resolves collision between this sphere and another Sphere.
     * @param elasticity Elasticity coefficient for the collision.
     * @param bO Pointer to the other Sphere.
     */
    void Collide(float elasticity, Sphere* bO);

    PTX_BEGIN_FIELDS(Sphere)
        PTX_FIELD(Sphere, velocity, "Velocity", 0, 0),
        PTX_FIELD(Sphere, position, "Position", 0, 0)
    PTX_END_FIELDS
    
    PTX_BEGIN_METHODS(Sphere)
        PTX_METHOD_AUTO(Sphere, GetRadius, "Get radius"),
        PTX_METHOD_AUTO(Sphere, Update, "Update"),
        PTX_METHOD_AUTO(Sphere, IsIntersecting, "Is intersecting"),
        PTX_METHOD_AUTO(Sphere, Collide, "Collide")
    PTX_END_METHODS
    
    PTX_BEGIN_DESCRIBE(Sphere)
        PTX_CTOR(Sphere, Vector3D, float)
    PTX_END_DESCRIBE(Sphere)
    
};
