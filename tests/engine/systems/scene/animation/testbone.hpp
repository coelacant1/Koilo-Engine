// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testbone.hpp
 * @brief Unit tests for the Bone class.
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
 * @class TestBone
 * @brief Contains static test methods for the Bone class.
 */
class TestBone {
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
