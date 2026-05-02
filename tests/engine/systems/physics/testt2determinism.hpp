// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

class TestT2Determinism {
public:
    static void TestStrictFpScopedEnvConstructAndRestore();
    static void TestT2IgnoresWallClockBudget();
    static void TestT2SameBinaryReplayBitExact();
    static void TestStrictBuildMacroVisibility();
    static void RunAllTests();
};
