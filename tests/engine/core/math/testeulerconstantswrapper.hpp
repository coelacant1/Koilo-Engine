// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testeulerconstantswrapper.hpp
 * @brief Unit tests for the EulerConstantsWrapper class.
 *
 * @date 11/10/2025
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/core/math/eulerconstants.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestEulerConstantsWrapper
 * @brief Contains static test methods for the EulerConstantsWrapper class.
 */
class TestEulerConstantsWrapper {
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
