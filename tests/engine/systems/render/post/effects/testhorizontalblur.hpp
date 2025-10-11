/**
 * @file testhorizontalblur.hpp
 * @brief Unit tests for the HorizontalBlur class.
 *
 * TODO: Add detailed description of what this test suite covers.
 *
 * @date 10/10/2025
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <ptx/systems/render/post/effects/horizontalblur.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestHorizontalBlur
 * @brief Contains static test methods for the HorizontalBlur class.
 */
class TestHorizontalBlur {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestApply();

    // Functionality tests

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
