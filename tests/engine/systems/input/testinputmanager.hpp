// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testinputmanager.hpp
 * @brief Unit tests for the InputManager class.
 *
 * @date 11/10/2025
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/systems/input/inputmanager.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestInputManager
 * @brief Contains static test methods for the InputManager class.
 */
class TestInputManager {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestUpdate();
    static void TestIsKeyPressed();
    static void TestIsKeyHeld();
    static void TestGetMousePosition();
    static void TestGetMouseDelta();
    static void TestIsMouseButtonPressed();
    static void TestIsGamepadConnected();
    static void TestIsActionPressed();
    static void TestIsActionHeld();
    static void TestGetAxis();

    // Edge case & integration tests
    static void TestEdgeCases();

    // Input listener tests (#28)

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
