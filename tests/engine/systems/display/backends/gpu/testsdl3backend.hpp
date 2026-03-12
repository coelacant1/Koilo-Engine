// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testsdl3backend.hpp
 * @brief Unit tests for the SDL3Backend class.
 *
 * @date 23/02/2026
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/systems/display/backends/gpu/sdl3backend.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestSDL3Backend
 * @brief Contains static test methods for the SDL3Backend class.
 */
class TestSDL3Backend {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestInitialize();
    static void TestShutdown();
    static void TestIsInitialized();
    static void TestGetInfo();
    static void TestPresent();
    static void TestClear();
    static void TestSetTitle();
    static void TestSetFullscreen();
    static void TestIsWindowOpen();
    static void TestProcessEvents();

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
