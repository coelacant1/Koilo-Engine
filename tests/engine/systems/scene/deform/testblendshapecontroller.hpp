// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testblendshapecontroller.hpp
 * @brief Unit tests for the BlendshapeController class.
 *
 * @date 11/10/2025
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/systems/scene/deform/blendshapecontroller.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestBlendshapeController
 * @brief Contains static test methods for the BlendshapeController class.
 */
class TestBlendshapeController {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestSetAnimator();
    static void TestAddBlendshape();
    static void TestRemoveBlendshape();
    static void TestGetBlendshapeCount();
    static void TestGetCapacity();
    static void TestSetWeight();
    static void TestGetWeight();
    static void TestResetWeights();
    static void TestUpdate();
    static void TestApplyTo();

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
