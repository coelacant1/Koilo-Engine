/**
 * @file testglitchx.hpp
 * @brief Unit tests for the GlitchX class.
 *
 * TODO: Add detailed description of what this test suite covers.
 *
 * @date 10/10/2025
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <ptx/systems/render/post/effects/glitchx.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestGlitchX
 * @brief Contains static test methods for the GlitchX class.
 */
class TestGlitchX {
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
