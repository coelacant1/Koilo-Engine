// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <unity.h>

namespace TestHotReload {
    void TestDefaultState();
    void TestEnableDisable();
    void TestPollWithNoWatchers();
    void TestWatchShadersNullPipeline();
    void TestWatchScriptEmptyPath();
    void TestWatchScriptNullEngine();
    void RunAllTests();
}
