/**
 * @file testcameralayout.hpp
 * @brief Unit tests for the CameraLayout class.
 *
 * TODO: Add detailed description of what this test suite covers.
 *
 * @date 10/10/2025
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <ptx/systems/render/core/cameralayout.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestCameraLayout
 * @brief Contains static test methods for the CameraLayout class.
 */
class TestCameraLayout {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestGetForwardAxis();
    static void TestGetUpAxis();
    static void TestGetForwardVector();
    static void TestGetUpVector();
    static void TestGetRotation();

    // Functionality tests

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
