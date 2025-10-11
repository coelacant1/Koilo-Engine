/**
 * @file testimagesequencematerial.hpp
 * @brief Unit tests for the ImageSequenceMaterial class.
 *
 * TODO: Add detailed description of what this test suite covers.
 *
 * @date 10/10/2025
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <ptx/systems/render/material/implementations/imagesequencematerial.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestImageSequenceMaterial
 * @brief Contains static test methods for the ImageSequenceMaterial class.
 */
class TestImageSequenceMaterial {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestSetHueAngle();
    static void TestUseUV();
    static void TestSetSequence();
    static void TestUpdate();

    // Functionality tests

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
