/**
 * @file testcamera.hpp
 * @brief Unit tests for the Camera class.
 *
 * TODO: Add detailed description of what this test suite covers.
 *
 * @date 10/10/2025
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <ptx/systems/render/core/camera.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestCamera
 * @brief Contains static test methods for the Camera class.
 */
class TestCamera {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestGetPixelGroup();
    static void TestGetCameraMinCoordinate();
    static void TestGetCameraMaxCoordinate();
    static void TestGetCameraCenterCoordinate();
    static void TestGetCameraTransformMin();
    static void TestGetCameraTransformMax();
    static void TestGetCameraTransformCenter();

    // Functionality tests

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
