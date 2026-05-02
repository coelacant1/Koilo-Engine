/**
 * @file testkoilokernel.hpp
 * @brief Unit tests for the KoiloKernel class.
 *
 * @date 02/05/2026
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/kernel/kernel.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestKoiloKernel
 * @brief Contains static test methods for the KoiloKernel class.
 */
class TestKoiloKernel {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestScratch();
    static void TestMessages();
    static void TestServices();
    static void TestModules();
    static void TestRegisterModule();
    static void TestIsRunning();

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
