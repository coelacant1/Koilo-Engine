#pragma once

#include <cstdint> // For std::uint32_t

/**
 * @file Random.hpp
 * @brief A platform-agnostic API for pseudo-random number generation.
 * @date 29/06/2025
 * @author Coela Can't
 */

/**
 * @namespace um3d::Random
 * @brief Unifies random number generation between Arduino and native C++.
 */
namespace um3d::Random {

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

} // namespace um3d::Random
