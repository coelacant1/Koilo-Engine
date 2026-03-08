// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testanimationlayer.hpp
 * @brief Unit tests for the AnimationLayer class.
 *
 * @date 22/02/2026
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/systems/scene/animation/animationmixer.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestAnimationLayer
 * @brief Contains static test methods for the AnimationLayer class.
 */
class TestAnimationLayer {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
