// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

class TestPhysicsStress {
public:
    static void TestBoxStackSettles();
    static void TestDzhanibekovIntermediateAxisFlip();
    static void TestBulletDoesNotTunnel();
    static void TestNonBulletTunnelsThroughThinWall();
    static void TestFrictionHoldsBelowThreshold();
    static void TestFrictionSlipsAboveThreshold();
    static void TestFrictionWarmStartReducesStackJitter();
    static void TestFrictionBasisStableUnderRotation();
    static void RunAllTests();
};
