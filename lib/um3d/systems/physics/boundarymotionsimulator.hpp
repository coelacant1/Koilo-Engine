/**
 * @file BoundaryMotionSimulator.h
 * @brief Defines the BoundaryMotionSimulator class for simulating motion and collisions of boundary objects.
 *
 * The BoundaryMotionSimulator class manages the simulation of motion, collisions, and interactions
 * between a boundary cube and multiple boundary spheres in 3D space. Includes randomized velocity and acceleration.
 *
 * @date 22/12/2024
 * @version 1.0
 * @author Coela Can't
 */

#pragma once

#include "../../core/math/rotation.hpp"
#include "../../core/math/vector3d.hpp"
#include "../../core/math/rotation.hpp"
#include "../scene/mesh.hpp"
#include "../../core/geometry/3d/cube.hpp"
#include "../../core/geometry/3d/sphere.hpp"
#include "../../core/platform/random.hpp"

/**
 * @class BoundaryMotionSimulator
 * @brief Simulates motion and collision interactions between a BoundaryCube and multiple BoundarySpheres.
 */
class BoundaryMotionSimulator {
private:
    Cube* bC; ///< Pointer to the boundary cube.
    Sphere** bS; ///< Array of pointers to the boundary spheres.
    int sphereCount; ///< Number of boundary spheres.
    float elasticity = 0.8f; ///< Elasticity coefficient for collisions.

public:
    /**
     * @brief Constructs a BoundaryMotionSimulator.
     * @param objects Array of Object3D pointers representing the objects.
     * @param objectCount Number of objects in the simulation.
     * @param bC Pointer to the boundary cube.
     * @param radius Radius of the boundary spheres.
     * @param elasticity Elasticity coefficient for collisions.
     */
    BoundaryMotionSimulator(Mesh** objects, int objectCount, Cube* bC, float radius, float elasticity);

    /**
     * @brief Randomizes the velocities of all boundary spheres within a specified range.
     * @param range Maximum velocity range (mm/s).
     */
    void Randomize(float range);

    /**
     * @brief Generates a randomized vector with ratios for X, Y, and Z axes.
     * @param range Maximum range for each axis ratio.
     * @return A randomized Vector3D.
     */
    Vector3D RandomRatio(float range);

    /**
     * @brief Updates the simulation by applying physics and resolving collisions.
     * @param dT Time step in seconds.
     * @param acceleration Acceleration vector applied to the spheres.
     * @param rotation Rotation quaternion applied to the spheres.
     */
    void Update(float dT, Vector3D acceleration, Quaternion rotation);
};
