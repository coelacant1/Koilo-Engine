// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testblendshapetransformcontroller.hpp
 * @brief Unit tests for the BlendshapeTransformController class.
 *
 * TODO: Add detailed description of what this test suite covers.
 *
 * @date 10/10/2025
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/systems/scene/deform/blendshapetransformcontroller.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestBlendshapeTransformController
 * @brief Contains static test methods for the BlendshapeTransformController class.
 */
class TestBlendshapeTransformController {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestGetBlendshapeCount();
    static void TestGetCapacity();
    static void TestAddBlendshape();
    static void TestSetBlendshapePositionOffset();
    static void TestSetBlendshapeScaleOffset();
    static void TestSetBlendshapeRotationOffset();
    static void TestGetPositionOffset();
    static void TestGetScaleOffset();
    static void TestGetRotationOffset();

    // Functionality tests

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
