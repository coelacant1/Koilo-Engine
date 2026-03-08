// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testray.hpp
 * @brief Unit tests for the Ray class.
 *
 * @date 11/10/2025
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/core/geometry/ray.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestRay
 * @brief Contains static test methods for the Ray class.
 */
class TestRay {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestGetPoint();
    static void TestNormalize();
    static void TestIsNormalized();
    static void TestTranslate();
    static void TestClosestPoint();
    static void TestClosestDistance();

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
