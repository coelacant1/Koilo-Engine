// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testraytracesettings.hpp
 * @brief Unit tests for the RayTraceSettings class.
 *
 * @date 11/10/2025
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/systems/render/ray/raytracer.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestRayTraceSettings
 * @brief Contains static test methods for the RayTraceSettings class.
 */
class TestRayTraceSettings {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
