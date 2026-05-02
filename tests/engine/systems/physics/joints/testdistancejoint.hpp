// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <unity.h>

class TestDistanceJoint {
public:
    static void TestHoldsAnchorsAtTargetLength();
    static void TestPendulumWithStaticAnchor();
    static void TestJointOnlySceneStillSolves();
    static void TestRemoveBodyAutoDetachesJoint();
    static void TestAddJointDeduplicates();
    static void RunAllTests();
};
