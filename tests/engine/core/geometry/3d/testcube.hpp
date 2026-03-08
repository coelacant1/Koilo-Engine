// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testcube.hpp
 * @brief Unit tests for the Cube geometry class.
 *
 * @date 10/10/2025
 * @version 2.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/core/geometry/3d/cube.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestCube
 * @brief Contains static test methods for the Cube class.
 */
class TestCube {
public:
    static void TestParameterizedConstructor();
    static void TestGetPosition();
    static void TestGetSize();
    static void TestGetMaximum();
    static void TestGetMinimum();
    static void TestDefaultConstructor();
    static void TestEdgeCases();
    static void RunAllTests();
};
