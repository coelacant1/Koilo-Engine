// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <cstdint> // For std::uint32_t

#include <koilo/registry/reflect_macros.hpp>

/**
 * @file Random.hpp
 * @brief A platform-agnostic API for pseudo-random number generation.
 * @date 29/06/2025
 * @author Coela Can't
 */

/**
 * @namespace koilo::Random
 * @brief Unifies random number generation between Arduino and native C++.
 */
namespace koilo {
namespace Random {

    /**
     * @brief Seeds the underlying random number generator.
     * @param seed 32-bit unsigned seed value.
     */
    void Seed(std::uint32_t seed);

    /**
     * @brief Generates a pseudo-random integer in an inclusive range.
     * @param min The minimum value of the range (inclusive).
     * @param max The maximum value of the range (inclusive).
     * @return A random integer within [min, max].
     */
    int Int(int min, int max);

    /**
     * @brief Generates a pseudo-random float in a half-open range.
     * @param min The minimum value of the range (inclusive).
     * @param max The maximum value of the range (exclusive).
     * @return A random float within [min, max).
     */
    float Float(float min, float max);

    struct Reflection {
        static void Seed(std::uint32_t seed) { ::koilo::Random::Seed(seed); }
        static int Int(int min, int max) { return ::koilo::Random::Int(min, max); }
        static float Float(float min, float max) { return ::koilo::Random::Float(min, max); }

        KL_BEGIN_FIELDS(Reflection)
            /* No reflected fields. */
        KL_END_FIELDS

        KL_BEGIN_METHODS(Reflection)
            KL_SMETHOD_OVLD(Reflection, Seed, void, std::uint32_t),
            KL_SMETHOD_OVLD(Reflection, Int, int, int, int),
            KL_SMETHOD_OVLD(Reflection, Float, float, float, float)
        KL_END_METHODS

        KL_BEGIN_DESCRIBE(Reflection)
        KL_END_DESCRIBE(Reflection)
    };
} // namespace Random
} // namespace koilo
