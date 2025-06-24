/**
 * @file PhysicsSimulator.h
 * @brief Defines the PhysicsSimulator class for managing physics simulations within a scene.
 *
 * The PhysicsSimulator class integrates boundary motion, object dynamics, and lighting
 * to provide a cohesive simulation environment. It manages a scene with objects, boundary
 * conditions, and physical interactions.
 *
 * @date 22/12/2024
 * @version 1.0
 * @author Coela Can't
 */

#pragma once

#include <cstdint>
#include "../../scene/lighting/light.hpp"
#include "boundarymotionsimulator.hpp"
#include "../../scene/mesh.hpp"
#include "../../math/quaternion.hpp"
#include "../../scene/scene.hpp"

/**
 * @class PhysicsSimulator
 * @brief Manages physics simulations for objects within a scene.
 */
class PhysicsSimulator {
private:
    BoundaryCube bC; ///< Boundary cube to contain and interact with objects.
    BoundaryMotionSimulator* bMS; ///< Simulator handling boundary object interactions.
    Scene* scene; ///< Scene containing objects and their dynamics.
    Light lights[6]; ///< Array of lights illuminating the scene.
    Object3D* objects[12]; ///< Array of objects in the simulation.
    
    int32_t previousTime; ///< Time of the previous simulation update.
    bool startedSim; ///< Indicates if the simulation has started.

public:
    /**
     * @brief Default constructor initializes the PhysicsSimulator.
     */
    PhysicsSimulator();

    /**
     * @brief Retrieves the scene managed by the simulator.
     * @return Pointer to the Scene object.
     */
    Scene* GetScene();

    /**
     * @brief Updates the simulation with new acceleration and rotation values.
     * @param acceleration The acceleration vector applied to objects.
     * @param rotation The rotation quaternion applied to objects.
     */
    void Update(Vector3D acceleration, Quaternion rotation);
};
