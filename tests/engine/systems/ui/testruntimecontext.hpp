// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testruntimecontext.hpp
 * @brief Unit tests for RuntimeContext isolation.
 * @date 03/09/2026
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <utils/testhelpers.hpp>

class TestRuntimeContext {
public:
    static void TestConstructEditor();
    static void TestConstructGame();
    static void TestMemoryBudgetAlloc();
    static void TestMemoryBudgetExceed();
    static void TestMemoryRelease();
    static void TestPeakMemory();
    static void TestErrorState();
    static void TestClearError();
    static void TestSnapshotRestore();
    static void TestSnapshotPreservesSlider();
    static void TestSnapshotPreservesCheckbox();
    static void TestIsolatedContexts();

    static void RunAllTests();
};
