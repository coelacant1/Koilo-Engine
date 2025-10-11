/**
 * @file testrenderingengine.hpp
 * @brief Unit tests for the RenderingEngine class.
 *
 * @date 10/10/2025
 * @version 1.0
 * @author Coela (Auto-Generated)
 */

#pragma once

#include <unity.h>
#include <ptx/systems/render/engine/renderer.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestRenderingEngine
 * @brief Contains static test methods for the RenderingEngine class.
 */
class TestRenderingEngine {
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
