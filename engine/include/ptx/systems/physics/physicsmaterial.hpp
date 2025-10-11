/**
 * @file physicsmaterial.hpp
 * @brief Physics material properties for collision response.
 *
 * @date 11/10/2025
 * @author Coela
 */

#pragma once

#include "../../registry/reflect_macros.hpp"

namespace ptx {

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

    PTX_BEGIN_FIELDS(PhysicsMaterial)
        PTX_FIELD(PhysicsMaterial, friction, "Friction", 0, 0),
        PTX_FIELD(PhysicsMaterial, bounciness, "Bounciness", 0, 0),
        PTX_FIELD(PhysicsMaterial, density, "Density", 0, 0)
    PTX_END_FIELDS

    PTX_BEGIN_METHODS(PhysicsMaterial)
    PTX_END_METHODS

    PTX_BEGIN_DESCRIBE(PhysicsMaterial)
        PTX_CTOR0(PhysicsMaterial),
        PTX_CTOR(PhysicsMaterial, float, float, float)
    PTX_END_DESCRIBE(PhysicsMaterial)
};

} // namespace ptx
