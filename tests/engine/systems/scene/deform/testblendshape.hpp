/**
 * @file testblendshape.hpp
 * @brief Unit tests for the Blendshape class.
 *
 * TODO: Add detailed description of what this test suite covers.
 *
 * @date 10/10/2025
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <ptx/systems/scene/deform/blendshape.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestBlendshape
 * @brief Contains static test methods for the Blendshape class.
 */
class TestBlendshape {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestBlendObject3D();

    // Functionality tests

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
