/**
 * @file testindexgroup.hpp
 * @brief Unit tests for the IndexGroup class.
 *
 * TODO: Add detailed description of what this test suite covers.
 *
 * @date 10/10/2025
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <ptx/assets/model/indexgroup.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestIndexGroup
 * @brief Contains static test methods for the IndexGroup class.
 */
class TestIndexGroup {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestAdd();
    static void TestSubtract();
    static void TestMultiply();
    static void TestDivide();
    static void TestToString();

    // Functionality tests

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
