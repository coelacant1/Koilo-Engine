// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <unity.h>

class TestIslandsAndSleep {
public:
    static void TestUnionFindGroupsConnectedBodies();
    static void TestStaticBodyDoesNotBridgeIslands();
    static void TestSleepingBodyAnchorsByDefault();
    static void TestSleepingBodyBridgesWhenRequested();
    static void TestSleepManagerSleepsQuietIsland();
    static void TestSleepManagerWakesOnMotion();
    static void TestAllowSleepFalseKeepsBodyAwake();
    static void TestSetVelocityWakesBody();
    static void TestSolverTreatsSleepingBodyAsStatic();
    static void RunAllTests();
};
