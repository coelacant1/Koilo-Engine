// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <unity.h>
#include <koilo/systems/physics/broadphase/dbvt.hpp>

class TestDBVT {
public:
    static void TestInsertAssignsHandleAndContainsAabb();
    static void TestRemoveFreesHandle();
    static void TestMoveNoOpWhenWithinFat();
    static void TestMoveReinsertsWhenEscaping();
    static void TestQueryOverlapsLeaves();
    static void TestQueryAllPairsReturnsCanonicalOrder();
    static void TestQueryAllPairsExcludesNonOverlapping();
    static void TestQuality();
    static void RunAllTests();
};
