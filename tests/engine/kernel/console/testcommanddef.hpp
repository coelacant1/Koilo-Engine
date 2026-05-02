/**
 * @file testcommanddef.hpp
 * @brief Unit tests for the CommandDef class.
 *
 * @date 02/05/2026
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/kernel/console/command_registry.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestCommandDef
 * @brief Contains static test methods for the CommandDef class.
 */
class TestCommandDef {
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
