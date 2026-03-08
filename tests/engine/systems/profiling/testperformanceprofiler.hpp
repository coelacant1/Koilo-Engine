// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testperformanceprofiler.hpp
 * @brief Unit tests for PerformanceProfiler.
 *
 * @date 24/10/2025
 * @author Coela
 */

#pragma once

#include <unity.h>


class TestPerformanceProfiler {
public:
    static void TestBeginFrame();
    static void TestEndFrame();

    static void TestGetFPS();

    static void TestClear();
    static void TestDefaultConstructor();
    static void TestEdgeCases();
    static void TestIsEnabled();
    static void TestParameterizedConstructor();
    static void TestPrintReport();
    static void TestSetEnabled();
    static void RunAllTests();
};

