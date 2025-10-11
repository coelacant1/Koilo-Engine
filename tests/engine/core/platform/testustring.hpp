/**
 * @file testustring.hpp
 * @brief Unit tests for the UString class.
 *
 * Comprehensive tests for platform-agnostic string operations.
 *
 * @date 10/10/2025
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <ptx/core/platform/ustring.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestUString
 * @brief Contains static test methods for the UString class.
 */
class TestUString {
public:
    // Constructor tests
    static void TestDefaultConstructor();

    // Assignment operator tests

    // Append tests

    // Concatenation operator tests

    // Length and empty tests
    static void TestLength();
    static void TestIsEmpty();

    // Clear tests
    static void TestClear();

    // CStr tests
    static void TestCStr();

    // FromFloat tests

    // Edge case tests

    /**
     * @brief Runs all test methods.
     */
    static void TestAppend();
    static void TestEdgeCases();
    static void TestParameterizedConstructor();
    static void RunAllTests();
};
