// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <unity.h>
#include <koilo/systems/physics/rigidbody.hpp>
#include <koilo/systems/physics/inertiatensor.hpp>
#include <utils/testhelpers.hpp>

class TestRigidBodyAngular {
public:
    static void TestDefaultsZeroAngular();
    static void TestExistingLinearBehaviorPreserved();
    static void TestApplyTorqueIntegratesAngularVelocity();
    static void TestApplyAngularImpulseInstant();
    static void TestApplyForceAtPointGeneratesTorque();
    static void TestGetPointVelocity();
    static void TestQuaternionStaysNormalized();
    static void TestNoAngularResponseWithoutInertia();
    static void TestSetInertiaSphereDiagonal();
    static void TestSetInertiaBoxDiagonal();
    static void TestNonDynamicIgnoresTorque();
    static void TestClearForcesResetsTorque();
    static void TestColliderMirrorsRotation();
    static void RunAllTests();
};
