/**
 * @file testpid.hpp
 * @brief Unit tests for the PID class.
 *
 * Comprehensive test suite covering PID controller functionality including:
 * - Constructor initialization with various gain configurations
 * - Proportional-only control (P controller)
 * - Integral accumulation and windup behavior
 * - Derivative response and derivative kick
 * - Full PID response to step inputs
 * - Output saturation and limiting
 * - Edge cases with zero and extreme gain values
 * - Time-based calculation variants
 *
 * @date 10/10/2025
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <ptx/core/control/pid.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestPID
 * @brief Contains static test methods for the PID class.
 */
class TestPID {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Proportional control tests

    // Integral control tests

    // Derivative control tests

    // Full PID tests

    // Time-based calculation tests

    // Output behavior tests

    // Edge cases and robustness

    /**
     * @brief Runs all test methods.
     */
    static void TestCalculate();
    static void TestEdgeCases();
    static void RunAllTests();
};
