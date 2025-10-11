/**
 * @file testcameramanager.hpp
 * @brief Unit tests for the CameraManager class.
 *
 * TODO: Add detailed description of what this test suite covers.
 *
 * @date 10/10/2025
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <ptx/systems/render/core/cameramanager.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestCameraManager
 * @brief Contains static test methods for the CameraManager class.
 */
class TestCameraManager {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestGetCameras();
    static void TestGetCameraCount();

    // Functionality tests

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
