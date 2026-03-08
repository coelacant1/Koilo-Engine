// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testprofiler.hpp
 * @brief Unit tests for the Profiler class.
 *
 * @date 11/10/2025
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/debug/profiler.hpp>
#include <utils/testhelpers.hpp>
#include <thread>
#include <chrono>

/**
 * @class TestProfiler
 * @brief Contains static test methods for the Profiler class.
 */
class TestProfiler {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestEnable();
    static void TestDisable();
    static void TestIsEnabled();
    static void TestBeginFrame();
    static void TestEndFrame();
    static void TestGetLastFrameTime();
    static void TestGetFPS();
    static void TestClearStats();
    static void TestPrintStats();

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
