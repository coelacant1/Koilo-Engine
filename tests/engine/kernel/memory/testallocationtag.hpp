/**
 * @file testallocationtag.hpp
 * @brief Unit tests for the AllocationTag class.
 *
 * @date 02/05/2026
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/kernel/memory/allocation_tag.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestAllocationTag
 * @brief Contains static test methods for the AllocationTag class.
 */
class TestAllocationTag {
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
