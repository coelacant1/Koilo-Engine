/**
 * @file particle.hpp
 * @brief Individual particle data structure.
 *
 * @date 11/10/2025
 * @author Coela
 */

#pragma once

#include "../../core/math/vector3d.hpp"
#include "../../core/math/vector2d.hpp"
#include "../../registry/reflect_macros.hpp"

namespace ptx {

/**
 * @struct Particle
 * @brief Represents a single particle in a particle system.
 */
struct Particle {
    Vector3D position;        ///< Current position
    Vector3D velocity;        ///< Current velocity
    Vector3D acceleration;    ///< Current acceleration (e.g., gravity)

    float lifetime;           ///< Total lifetime (seconds)
    float age;                ///< Current age (seconds)

    float size;               ///< Current size
    float sizeStart;          ///< Initial size
    float sizeEnd;            ///< Final size

    Vector3D color;           ///< Current color (RGB 0-1)
    Vector3D colorStart;      ///< Initial color
    Vector3D colorEnd;        ///< Final color

    float alpha;              ///< Current alpha (0-1)
    float alphaStart;         ///< Initial alpha
    float alphaEnd;           ///< Final alpha

    float rotation;           ///< Current rotation (radians)
    float rotationSpeed;      ///< Rotation speed (radians/second)

    bool active;              ///< Is particle active?

    /**
     * @brief Default constructor.
     */
    Particle()
        : position(0, 0, 0), velocity(0, 0, 0), acceleration(0, 0, 0),
          lifetime(1.0f), age(0.0f),
          size(1.0f), sizeStart(1.0f), sizeEnd(1.0f),
          color(1, 1, 1), colorStart(1, 1, 1), colorEnd(1, 1, 1),
          alpha(1.0f), alphaStart(1.0f), alphaEnd(1.0f),
          rotation(0.0f), rotationSpeed(0.0f),
          active(false) {}

    /**
     * @brief Checks if particle is alive.
     */
    bool IsAlive() const {
        return active && age < lifetime;
    }

    /**
     * @brief Gets the normalized lifetime (0-1).
     */
    float GetLifetimeProgress() const {
        if (lifetime <= 0.0f) return 1.0f;
        return age / lifetime;
    }

    PTX_BEGIN_FIELDS(Particle)
        PTX_FIELD(Particle, position, "Position", 0, 0),
        PTX_FIELD(Particle, velocity, "Velocity", 0, 0),
        PTX_FIELD(Particle, lifetime, "Lifetime", 0, 0),
        PTX_FIELD(Particle, age, "Age", 0, 0),
        PTX_FIELD(Particle, size, "Size", 0, 0),
        PTX_FIELD(Particle, color, "Color", 0, 0),
        PTX_FIELD(Particle, alpha, "Alpha", 0, 0),
        PTX_FIELD(Particle, active, "Active", 0, 1)
    PTX_END_FIELDS

    PTX_BEGIN_METHODS(Particle)
        PTX_METHOD_AUTO(Particle, IsAlive, "Is alive"),
        PTX_METHOD_AUTO(Particle, GetLifetimeProgress, "Get lifetime progress")
    PTX_END_METHODS

    PTX_BEGIN_DESCRIBE(Particle)
        PTX_CTOR0(Particle)
    PTX_END_DESCRIBE(Particle)
};

} // namespace ptx
