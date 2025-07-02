/**
 * @file Cube.h
 * @brief Defines the Cube class for representing and managing axis-aligned bounding cubes.
 *
 * The Cube class provides functionality for collision detection and physics calculations
 * for axis-aligned bounding cubes.
 *
 * @date 22/12/2024
 * @version 1.0
 * @author Coela Can't
 */

#pragma once

#include "../../math/rotation.hpp"
#include "../../math/vector3d.hpp"

/**
 * @class Cube
 * @brief Represents an axis-aligned bounding cube for collision detection and physics.
 */
class Cube {
private:
    Quaternion previousRotation; ///< Previous rotation of the object.
    Vector3D centerPosition; ///< Center position of the bounding cube.
    Vector3D maximum; ///< Maximum coordinates of the bounding cube.
    Vector3D minimum; ///< Minimum coordinates of the bounding cube.

public:
    Vector3D velocity = Vector3D(0, 0, 0); ///< Velocity of the cube.
    Vector3D position = Vector3D(0, 0, 0); ///< Current position of the cube.

    /**
     * @brief Constructs a Cube object with a specified center and size.
     * @param centerPosition Center position of the cube.
     * @param objectSize Size of the cube.
     */
    Cube(Vector3D centerPosition, Vector3D objectSize);

    /**
     * @brief Retrieves the current position of the cube.
     * @return The current position as a Vector3D.
     */
    Vector3D GetPosition();

    /**
     * @brief Retrieves the size of the cube.
     * @return The size as a Vector3D.
     */
    Vector3D GetSize();

    /**
     * @brief Retrieves the maximum coordinates of the cube.
     * @return The maximum coordinates as a Vector3D.
     */
    Vector3D GetMaximum();

    /**
     * @brief Retrieves the minimum coordinates of the cube.
     * @return The minimum coordinates as a Vector3D.
     */
    Vector3D GetMinimum();

    /**
     * @brief Updates the cube's state based on time, acceleration, and rotation.
     * @param dT Time step in seconds.
     * @param acceleration Acceleration vector applied to the cube.
     * @param rotation Rotation quaternion applied to the cube.
     */
    void Update(float dT, Vector3D acceleration, Quaternion rotation);

    /**
     * @brief Checks for intersection with another Cube.
     * @param bO Pointer to the other Cube.
     * @return The intersection vector if intersecting, otherwise zero vector.
     */
    Vector3D IsIntersecting(Cube* bO);
};
