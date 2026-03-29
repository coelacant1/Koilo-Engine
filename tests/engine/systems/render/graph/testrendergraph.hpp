// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <unity.h>

namespace TestRenderGraph {
    void TestEmptyGraphCompiles();
    void TestSinglePassCompiles();
    void TestLinearChainOrder();
    void TestDiamondDAGOrder();
    void TestCycleDetection();
    void TestResourceLifetimes();
    void TestExecuteCallsCallbacks();
    void TestClearResetsState();
    void TestUncompiledExecuteIsNoop();
    void TestIndependentPassesPreserveInsertionOrder();
    void TestWriteAfterWriteOrdering();
    void RunAllTests();
}
