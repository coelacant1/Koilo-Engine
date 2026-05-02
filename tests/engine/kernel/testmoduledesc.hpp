/**
 * @file testmoduledesc.hpp
 * @brief Unit tests for the ModuleDesc class.
 *
 * @date 02/05/2026
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/kernel/module.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestModuleDesc
 * @brief Contains static test methods for the ModuleDesc class.
 */
class TestModuleDesc {
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
