#pragma once

#include <cstdint> // For std::uint32_t

#include "../../registry/reflect_macros.hpp"

/**
 * @file Random.hpp
 * @brief A platform-agnostic API for pseudo-random number generation.
 * @date 29/06/2025
 * @author Coela Can't
 */

/**
 * @namespace ptx::Random
 * @brief Unifies random number generation between Arduino and native C++.
 */
namespace ptx {
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
        static void Seed(std::uint32_t seed) { ::ptx::Random::Seed(seed); }
        static int Int(int min, int max) { return ::ptx::Random::Int(min, max); }
        static float Float(float min, float max) { return ::ptx::Random::Float(min, max); }

        PTX_BEGIN_FIELDS(Reflection)
            /* No reflected fields. */
        PTX_END_FIELDS

        PTX_BEGIN_METHODS(Reflection)
            PTX_SMETHOD_OVLD(Reflection, Seed, void, std::uint32_t),
            PTX_SMETHOD_OVLD(Reflection, Int, int, int, int),
            PTX_SMETHOD_OVLD(Reflection, Float, float, float, float)
        PTX_END_METHODS

        PTX_BEGIN_DESCRIBE(Reflection)
        PTX_END_DESCRIBE(Reflection)
    };
} // namespace Random
} // namespace ptx
