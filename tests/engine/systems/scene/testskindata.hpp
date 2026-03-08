// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testskindata.hpp
 * @brief Unit tests for the SkinData class.
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
 * @class TestSkinData
 * @brief Contains static test methods for the SkinData class.
 */
class TestSkinData {
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
