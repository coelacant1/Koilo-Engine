// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testsphere.hpp
 * @brief Unit tests for the Sphere geometry class.
 *
 * @date 10/10/2025
 * @version 2.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/core/geometry/3d/sphere.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestSphere
 * @brief Contains static test methods for the Sphere class.
 */
class TestSphere {
public:
    static void TestParameterizedConstructor();
    static void TestGetRadius();
    static void TestDefaultConstructor();
    static void TestEdgeCases();
    static void TestCollide();
    static void TestIsIntersecting();
    static void TestUpdate();
    static void RunAllTests();
};
