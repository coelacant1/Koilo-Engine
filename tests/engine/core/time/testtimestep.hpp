/**
 * @file testtimestep.hpp
 * @brief Unit tests for the TimeStep class.
 *
 * Tests timing utilities and frequency-based triggering.
 *
 * @date 10/10/2025
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <ptx/core/time/timestep.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestTimeStep
 * @brief Contains static test methods for the TimeStep class.
 */
class TestTimeStep {
public:
    // Constructor tests

    // SetFrequency tests
    static void TestSetFrequency();

    // IsReady tests

    // Edge case tests

    /**
     * @brief Runs all test methods.
     */
    static void TestDefaultConstructor();
    static void TestEdgeCases();
    static void TestIsReady();
    static void TestParameterizedConstructor();
    static void RunAllTests();
};
