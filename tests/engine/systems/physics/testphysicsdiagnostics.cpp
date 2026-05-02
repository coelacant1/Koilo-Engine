// SPDX-License-Identifier: GPL-3.0-or-later
#include "testphysicsdiagnostics.hpp"

#include <koilo/systems/physics/physicsworld.hpp>
#include <koilo/systems/physics/rigidbody.hpp>
#include <koilo/systems/physics/spherecollider.hpp>
#include <koilo/systems/physics/physicsmaterial.hpp>
#include <utils/testhelpers.hpp>

#include <cmath>

using namespace koilo;

namespace {

void StepN(PhysicsWorld& w, int n, float dt) {
    for (int i = 0; i < n; ++i) w.Step(dt);
}

RigidBody MakeDyn(float mass, const Vector3D& pos, const Vector3D& vel = Vector3D(0,0,0)) {
    RigidBody rb(BodyType::Dynamic, mass);
    rb.SetInertiaSphere(0.5f);
    rb.SetPose(BodyPose(pos));
    rb.SetVelocity(vel);
    rb.SetLinearDamping(0.0f);
    rb.SetAngularDamping(0.0f);
    return rb;
}

} // namespace

void TestPhysicsDiagnostics::TestDefaultConstructor() {
    // TODO: Implement test for default constructor
    PhysicsDiagnostics obj;
    TEST_ASSERT_TRUE(false);  // Not implemented
}

void TestPhysicsDiagnostics::TestEdgeCases() {
    // TODO: Test edge cases (null, boundaries, extreme values)
    TEST_ASSERT_TRUE(false);  // Not implemented
}

void TestPhysicsDiagnostics::TestParameterizedConstructor() {
    // TODO: Implement test for parameterized constructor
    TEST_ASSERT_TRUE(false);  // Not implemented
}

void TestPhysicsDiagnostics::RunAllTests() {

    RUN_TEST(TestDefaultConstructor);
    RUN_TEST(TestEdgeCases);
    RUN_TEST(TestParameterizedConstructor);
}
