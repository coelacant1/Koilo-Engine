// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

class TestDeterministicReplay {
public:
    static void TestSameSceneBitExact();
    static void TestScriptedForcesBitExact();
    static void TestCollisionBitExact();
    static void TestJointSceneBitExact();
    static void TestDifferentInsertionOrderDiverges();
    static void RunAllTests();
};
