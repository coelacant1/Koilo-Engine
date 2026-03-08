// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testeasyeaseanimator.hpp
 * @brief Unit tests for the EasyEaseAnimator class.
 *
 * @date 16/02/2026
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/systems/scene/animation/easyeaseanimator.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestEasyEaseAnimator
 * @brief Contains static test methods for the EasyEaseAnimator class.
 */
class TestEasyEaseAnimator {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestSetConstants();
    static void TestGetValue();
    static void TestGetTarget();

    static void TestAddParameterFrame();
    static void TestSetInterpolationMethod();
    static void TestReset();
    static void TestSetParameters();
    static void TestUpdate();
    static void TestGetCapacity();
    static void TestGetParameterCount();
    static void TestIsActive();
    static void TestSetActive();

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */

    static void RunAllTests();
};
