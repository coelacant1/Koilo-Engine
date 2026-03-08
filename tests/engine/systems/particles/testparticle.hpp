// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testparticle.hpp
 * @brief Unit tests for the Particle class.
 *
 * @date 11/10/2025
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/systems/particles/particle.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestParticle
 * @brief Contains static test methods for the Particle class.
 */
class TestParticle {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestIsAlive();
    static void TestGetLifetimeProgress();

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
