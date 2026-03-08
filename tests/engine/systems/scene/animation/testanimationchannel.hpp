// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testanimationchannel.hpp
 * @brief Unit tests for the AnimationChannel class.
 *
 * @date 22/02/2026
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/systems/scene/animation/animationclip.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestAnimationChannel
 * @brief Contains static test methods for the AnimationChannel class.
 */
class TestAnimationChannel {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestAddKey();
    static void TestEvaluate();

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
