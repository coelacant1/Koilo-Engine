// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testeulerconstants.hpp
 * @brief Unit tests for the EulerConstants source file (eulerconstants.cpp).
 *
 * @date 24/10/2025
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/core/math/eulerconstants.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestEulerConstants
 * @brief Contains static test methods for eulerconstants.cpp implementation.
 */
class TestEulerConstants {
public:
    // Static frame tests
    static void TestStaticFrameConstants();

    // Rotating frame tests
    static void TestRotatingFrameConstants();

    // Wrapper tests
    static void TestEulerConstantsWrapper();

    // Edge case tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
