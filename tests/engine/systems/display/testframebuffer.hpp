// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testframebuffer.hpp
 * @brief Unit tests for the Framebuffer class.
 *
 * @date 11/10/2025
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/systems/display/framebuffer.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestFramebuffer
 * @brief Contains static test methods for the Framebuffer class.
 */
class TestFramebuffer {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestGetSizeBytes();
    static void TestIsValid();
    static void TestClear();

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
