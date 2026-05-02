/**
 * @file testdebuglinevertex.hpp
 * @brief Unit tests for the DebugLineVertex class.
 *
 * @date 02/05/2026
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/systems/render/pipeline/render_pipeline.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestDebugLineVertex
 * @brief Contains static test methods for the DebugLineVertex class.
 */
class TestDebugLineVertex {
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
