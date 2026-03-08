// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file physicsmaterial.hpp
 * @brief Physics material properties for collision response.
 *
 * @date 11/10/2025
 * @author Coela
 */

#pragma once

#include <koilo/registry/reflect_macros.hpp>

namespace koilo {

/**
 * @struct PhysicsMaterial
 * @brief Describes physical properties of a collider for collision response.
 */
struct PhysicsMaterial {
    float friction;    ///< Coefficient of friction (0 = no friction, 1 = high friction)
    float bounciness;  ///< Coefficient of restitution (0 = no bounce, 1 = perfect bounce)
    float density;     ///< Density in kg/m³

    /**
     * @brief Default constructor.
     */
    PhysicsMaterial()
        : friction(0.5f), bounciness(0.3f), density(1.0f) {}

    /**
     * @brief Parameterized constructor.
     * @param friction Friction coefficient.
     * @param bounciness Bounciness coefficient.
     * @param density Density in kg/m³.
     */
    PhysicsMaterial(float friction, float bounciness, float density)
        : friction(friction), bounciness(bounciness), density(density) {}

    KL_BEGIN_FIELDS(PhysicsMaterial)
        KL_FIELD(PhysicsMaterial, friction, "Friction", 0, 0),
        KL_FIELD(PhysicsMaterial, bounciness, "Bounciness", 0, 0),
        KL_FIELD(PhysicsMaterial, density, "Density", 0, 0)
    KL_END_FIELDS

    KL_BEGIN_METHODS(PhysicsMaterial)
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(PhysicsMaterial)
        KL_CTOR0(PhysicsMaterial),
        KL_CTOR(PhysicsMaterial, float, float, float)
    KL_END_DESCRIBE(PhysicsMaterial)
};

} // namespace koilo
