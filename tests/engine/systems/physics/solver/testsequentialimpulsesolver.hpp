// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <unity.h>

class TestSequentialImpulseSolver {
public:
    static void TestSingleNormalImpulseDrivesVnToZero();
    static void TestRestitutionReversesNormalVelocity();
    static void TestFrictionStopsTangentialMotion();
    static void TestStaticVsStaticSkipped();
    static void TestWarmStartPreservesAccumulator();
    static void TestSelfContactSkipped();
    static void TestBaumgartePushesPenetrationApart();
    static void RunAllTests();
};
