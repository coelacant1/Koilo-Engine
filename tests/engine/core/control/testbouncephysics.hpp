/**
 * @file testbouncephysics.hpp
 * @brief Unit tests for the BouncePhysics class.
 */

#pragma once

#include <unity.h>
#include <ptx/core/control/bouncephysics.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestBouncePhysics
 * @brief Contains static test methods for the BouncePhysics class.
 */
class TestBouncePhysics {
public:
    // Constructor tests
    static void TestParameterizedConstructor();

    // Calculate method tests

    // Physics behavior tests

    // Edge case tests

    /**
     * @brief Runs all test methods.
     */
    static void TestCalculate();
    static void TestDefaultConstructor();
    static void TestEdgeCases();
    static void RunAllTests();
};
