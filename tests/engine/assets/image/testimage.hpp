/**
 * @file testimage.hpp
 * @brief Unit tests for the Image class.
 *
 * TODO: Add detailed description of what this test suite covers.
 *
 * @date 10/10/2025
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <ptx/assets/image/image.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestImage
 * @brief Contains static test methods for the Image class.
 */
class TestImage {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestSetData();
    static void TestSetColorPalette();
    static void TestSetSize();
    static void TestSetPosition();
    static void TestSetRotation();
    static void TestGetColorAtCoordinate();

    // Functionality tests

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
