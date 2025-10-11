/**
 * @file testblendshapecontroller.hpp
 * @brief Unit tests for the BlendshapeController class.
 *
 * TODO: Add detailed description of what this test suite covers.
 *
 * @date 10/10/2025
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <ptx/systems/scene/deform/blendshapecontroller.hpp>
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
