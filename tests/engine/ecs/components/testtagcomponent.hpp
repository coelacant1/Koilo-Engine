// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testtagcomponent.hpp
 * @brief Unit tests for the TagComponent class.
 *
 * @date 16/02/2026
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/ecs/components/tagcomponent.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestTagComponent
 * @brief Contains static test methods for the TagComponent class.
 */
class TestTagComponent {
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
