/**
 * @file testcompositor.hpp
 * @brief Unit tests for the Compositor class.
 *
 * TODO: Add detailed description of what this test suite covers.
 *
 * @date 10/10/2025
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <ptx/systems/render/post/compositor.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestCompositor
 * @brief Contains static test methods for the Compositor class.
 */
class TestCompositor {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestAddEffect();
    static void TestSetEnabled();
    static void TestClear();
    static void TestApply();
    static void TestGetCount();
    static void TestGetCapacity();

    // Functionality tests

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
