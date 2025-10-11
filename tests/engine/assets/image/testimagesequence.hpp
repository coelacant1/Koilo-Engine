/**
 * @file testimagesequence.hpp
 * @brief Unit tests for the ImageSequence class.
 *
 * TODO: Add detailed description of what this test suite covers.
 *
 * @date 10/10/2025
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <ptx/assets/image/imagesequence.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestImageSequence
 * @brief Contains static test methods for the ImageSequence class.
 */
class TestImageSequence {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestSetFPS();
    static void TestSetSize();
    static void TestSetPosition();
    static void TestSetRotation();
    static void TestReset();
    static void TestUpdate();
    static void TestGetColorAtCoordinate();

    // Functionality tests

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
