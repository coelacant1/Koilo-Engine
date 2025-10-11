/**
 * @file testyawpitchroll.hpp
 * @brief Unit tests for the YawPitchRoll class.
 *
 * Tests for yaw, pitch, and roll angle representation.
 *
 * @date 10/10/2025
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <ptx/core/math/yawpitchroll.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestYawPitchRoll
 * @brief Contains static test methods for the YawPitchRoll class.
 */
class TestYawPitchRoll {
public:
    // Constructor tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Field access tests

    // String conversion tests
    static void TestToString();

    // Edge case tests

    /**
     * @brief Runs all test methods.
     */
    static void TestEdgeCases();
    static void RunAllTests();
};
