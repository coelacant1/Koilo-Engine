// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testmouse.hpp
 * @brief Unit tests for the Mouse class.
 *
 * @date 11/10/2025
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/systems/input/mouse.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestMouse
 * @brief Contains static test methods for the Mouse class.
 */
class TestMouse {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestUpdate();
    static void TestGetPosition();
    static void TestGetDelta();
    static void TestGetScrollDelta();
    static void TestIsButtonPressed();
    static void TestIsButtonHeld();
    static void TestIsButtonReleased();

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
