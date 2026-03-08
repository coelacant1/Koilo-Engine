// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testmemoryprofiler.hpp
 * @brief Unit tests for MemoryProfiler.
 *
 * @date 24/10/2025
 * @author Coela
 */

#pragma once

#include <unity.h>


class TestMemoryProfiler {
public:

    static void TestGetStats();

    static void TestClear();
    static void TestDefaultConstructor();
    static void TestEdgeCases();
    static void TestIsEnabled();
    static void TestParameterizedConstructor();
    static void TestPrintLeaks();
    static void TestPrintReport();
    static void TestSetEnabled();
    static void RunAllTests();
};

