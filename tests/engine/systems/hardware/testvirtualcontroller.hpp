/**
 * @file testvirtualcontroller.hpp
 * @brief Unit tests for the VirtualController class.
 *
 * TODO: Add detailed description of what this test suite covers.
 *
 * @date 10/10/2025
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <ptx/systems/hardware/virtualcontroller.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestVirtualController
 * @brief Contains static test methods for the VirtualController class.
 */
class TestVirtualController {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestInitialize();
    static void TestDisplay();

    // Functionality tests

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
