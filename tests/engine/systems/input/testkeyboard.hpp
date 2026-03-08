// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testkeyboard.hpp
 * @brief Unit tests for the Keyboard class.
 *
 * @date 11/10/2025
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/systems/input/keyboard.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestKeyboard
 * @brief Contains static test methods for the Keyboard class.
 */
class TestKeyboard {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestUpdate();
    static void TestIsKeyPressed();
    static void TestIsKeyHeld();
    static void TestIsKeyReleased();
    static void TestGetTextInput();
    static void TestClearTextInput();

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
