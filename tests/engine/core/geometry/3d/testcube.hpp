/**
 * @file testcube.hpp
 * @brief Unit tests for the Cube class.
 *
 * This test suite validates the Cube class functionality including:
 * - Constructor initialization with center position and size
 * - Position, size, min/max coordinate getters
 * - Physics update with acceleration and rotation
 * - Intersection detection between cubes
 * - Edge cases with zero/extreme values
 *
 * @date 10/10/2025
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <ptx/core/geometry/3d/cube.hpp>
#include <ptx/core/math/axisangle.hpp>
#include <ptx/core/math/rotation.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestCube
 * @brief Contains static test methods for the Cube class.
 */
class TestCube {
public:
    // Constructor & lifecycle tests
    static void TestParameterizedConstructor();

    // Getter method tests
    static void TestGetPosition();
    static void TestGetSize();
    static void TestGetMaximum();
    static void TestGetMinimum();

    // Physics update tests

    // Intersection tests

    // Field access tests

    // Edge case tests

    /**
     * @brief Runs all test methods.
     */
    static void TestDefaultConstructor();
    static void TestEdgeCases();
    static void TestIsIntersecting();
    static void TestUpdate();
    static void RunAllTests();
};
