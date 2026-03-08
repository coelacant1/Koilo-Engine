// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testgamepad.hpp
 * @brief Unit tests for the Gamepad class.
 *
 * @date 11/10/2025
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/systems/input/gamepad.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestGamepad
 * @brief Contains static test methods for the Gamepad class.
 */
class TestGamepad {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestUpdate();
    static void TestIsConnected();
    static void TestGetID();
    static void TestIsButtonPressed();
    static void TestIsButtonHeld();
    static void TestIsButtonReleased();
    static void TestGetAxisValue();

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
