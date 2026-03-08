// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testskinvertex.hpp
 * @brief Unit tests for the SkinVertex class.
 *
 * @date 23/02/2026
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/systems/scene/animation/skeleton.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestSkinVertex
 * @brief Contains static test methods for the SkinVertex class.
 */
class TestSkinVertex {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void TestNormalize();
    static void RunAllTests();
};
