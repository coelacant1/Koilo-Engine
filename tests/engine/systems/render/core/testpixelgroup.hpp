/**
 * @file testpixelgroup.hpp
 * @brief Unit tests for the PixelGroup class.
 *
 * TODO: Add detailed description of what this test suite covers.
 *
 * @date 10/10/2025
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <ptx/systems/render/core/pixelgroup.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestPixelGroup
 * @brief Contains static test methods for the PixelGroup class.
 */
class TestPixelGroup {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestGetCenterCoordinate();
    static void TestGetSize();
    static void TestGetCoordinate();
    static void TestGetPixelIndex();
    static void TestGetColor();
    static void TestGetColors();
    static void TestGetColorBuffer();
    static void TestGetPixelCount();
    static void TestOverlaps();
    static void TestContainsVector2D();
    // ... add tests for remaining 11 methods

    // Functionality tests

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void TestGetAlternateXIndex();
    static void TestGetAlternateYIndex();
    static void TestGetDownIndex();
    static void TestGetLeftIndex();
    static void TestGetOffsetXIndex();
    static void TestGetOffsetXYIndex();
    static void TestGetOffsetYIndex();
    static void TestGetRadialIndex();
    static void TestGetRightIndex();
    static void TestGetUpIndex();
    static void TestGridSort();
    static void RunAllTests();
};
