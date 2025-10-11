/**
 * @file testpixel.hpp
 * @brief Unit tests for the Pixel class.
 *
 * TODO: Add detailed description of what this test suite covers.
 *
 * @date 10/10/2025
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <ptx/systems/render/core/pixel.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestPixel
 * @brief Contains static test methods for the Pixel class.
 */
class TestPixel {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestGetPosition();
    static void TestSetUpPixel();
    static void TestSetDownPixel();
    static void TestSetLeftPixel();
    static void TestSetRightPixel();
    static void TestHasUpPixel();
    static void TestHasDownPixel();
    static void TestHasLeftPixel();
    static void TestHasRightPixel();
    static void TestGetUpPixel();
    // ... add tests for remaining 3 methods

    // Functionality tests

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void TestGetDownPixel();
    static void TestGetLeftPixel();
    static void TestGetRightPixel();
    static void RunAllTests();
};
