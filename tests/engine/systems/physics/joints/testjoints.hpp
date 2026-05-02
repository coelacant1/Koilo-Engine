// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <unity.h>

class TestBallSocketJoint {
public:
    static void TestAnchorsCoincide();
    static void TestRelativeOrientationIsFree();
    static void RunAllTests();
};

class TestFixedJoint {
public:
    static void TestPositionLocked();
    static void TestOrientationLocked();
    static void TestRecaptureAcceptsNewRest();
    static void RunAllTests();
};

class TestHingeJoint {
public:
    static void TestRotatesFreelyAboutAxis();
    static void TestPerpendicularAxesStayAligned();
    static void TestLimitStopsRotation();
    static void TestMotorAccelerates();
    static void TestMotorRespectsLimit();
    static void RunAllTests();
};

class TestSliderJoint {
public:
    static void TestSlidesFreelyAlongAxis();
    static void TestPerpendicularMotionLocked();
    static void TestLimitStopsTranslation();
    static void TestOrientationLocked();
    static void RunAllTests();
};
