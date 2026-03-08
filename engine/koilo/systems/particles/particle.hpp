// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file particle.hpp
 * @brief Individual particle data structure.
 *
 * @date 11/10/2025
 * @author Coela
 */

#pragma once

#include <koilo/core/math/vector3d.hpp>
#include <koilo/core/math/vector2d.hpp>
#include <koilo/registry/reflect_macros.hpp>

namespace koilo {

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

    KL_BEGIN_FIELDS(Particle)
        KL_FIELD(Particle, position, "Position", 0, 0),
        KL_FIELD(Particle, velocity, "Velocity", 0, 0),
        KL_FIELD(Particle, lifetime, "Lifetime", 0, 0),
        KL_FIELD(Particle, age, "Age", 0, 0),
        KL_FIELD(Particle, size, "Size", 0, 0),
        KL_FIELD(Particle, color, "Color", 0, 0),
        KL_FIELD(Particle, alpha, "Alpha", 0, 0),
        KL_FIELD(Particle, active, "Active", 0, 1)
    KL_END_FIELDS

    KL_BEGIN_METHODS(Particle)
        KL_METHOD_AUTO(Particle, IsAlive, "Is alive"),
        KL_METHOD_AUTO(Particle, GetLifetimeProgress, "Get lifetime progress")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(Particle)
        KL_CTOR0(Particle)
    KL_END_DESCRIBE(Particle)
};

} // namespace koilo
