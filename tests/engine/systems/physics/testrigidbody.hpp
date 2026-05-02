// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testrigidbody.hpp
 * @brief Unit tests for the RigidBody class.
 *
 * @date 22/02/2026
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/systems/physics/rigidbody.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestRigidBody
 * @brief Contains static test methods for the RigidBody class.
 */
class TestRigidBody {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestGetBodyType();
    static void TestIsDynamic();
    static void TestIsStatic();
    static void TestIsKinematic();
    static void TestGetMass();
    static void TestSetMass();
    static void TestGetVelocity();
    static void TestSetVelocity();
    static void TestApplyForce();
    static void TestApplyImpulse();
    static void TestClearForces();
    static void TestSetRestitution();
    static void TestSetFriction();
    static void TestSetLinearDamping();

    // Edge case & integration tests
    static void TestEdgeCases();

    // split integration semantics.

    // CCD bullet flag.

    /**
     * @brief Runs all test methods.
     */
    static void TestApplyAngularImpulse();
    static void TestApplyForceAtPoint();
    static void TestApplyTorque();
    static void TestGetAngularVelocity();
    static void TestGetCollider();
    static void TestGetPointVelocity();
    static void TestGetPose();
    static void TestGetPreviousPose();
    static void TestMakeDynamic();
    static void TestMakeKinematic();
    static void TestMakeStatic();
    static void TestSetAngularDamping();
    static void TestSetAngularVelocity();
    static void TestSetCollider();
    static void TestSetInertiaBox();
    static void TestSetInertiaCapsule();
    static void TestSetInertiaCylinder();
    static void TestSetInertiaSphere();
    static void TestSetPose();
    static void TestGetAllowSleep();
    static void TestIsSleeping();
    static void TestSetAllowSleep();
    static void TestSleep();
    static void TestWake();
    static void RunAllTests();
};
