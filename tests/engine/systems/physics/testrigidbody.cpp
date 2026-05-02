// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testrigidbody.cpp
 * @brief Implementation of RigidBody unit tests.
 */

#include "testrigidbody.hpp"
#include <koilo/systems/physics/spherecollider.hpp>

using namespace koilo;

// ========== Constructor Tests ==========

void TestRigidBody::TestDefaultConstructor() {
    RigidBody obj;
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, obj.GetMass());
    TEST_ASSERT_TRUE(obj.IsDynamic());
    Vector3D v = obj.GetVelocity();
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, v.X);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, v.Y);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, v.Z);
}

void TestRigidBody::TestParameterizedConstructor() {
    RigidBody obj(BodyType::Static, 5.0f);
    TEST_ASSERT_TRUE(obj.IsStatic());
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 5.0f, obj.GetMass());
}

// ========== Method Tests ==========

void TestRigidBody::TestGetBodyType() {
    RigidBody obj(BodyType::Kinematic, 1.0f);
    TEST_ASSERT_TRUE(obj.GetBodyType() == BodyType::Kinematic);
}

void TestRigidBody::TestIsDynamic() {
    RigidBody obj(BodyType::Dynamic, 1.0f);
    TEST_ASSERT_TRUE(obj.IsDynamic());
    TEST_ASSERT_TRUE(!obj.IsStatic());
    TEST_ASSERT_TRUE(!obj.IsKinematic());
}

void TestRigidBody::TestIsStatic() {
    RigidBody obj(BodyType::Static, 1.0f);
    TEST_ASSERT_TRUE(obj.IsStatic());
    TEST_ASSERT_TRUE(!obj.IsDynamic());
    TEST_ASSERT_TRUE(!obj.IsKinematic());
}

void TestRigidBody::TestIsKinematic() {
    RigidBody obj(BodyType::Kinematic, 1.0f);
    TEST_ASSERT_TRUE(obj.IsKinematic());
    TEST_ASSERT_TRUE(!obj.IsDynamic());
    TEST_ASSERT_TRUE(!obj.IsStatic());
}

void TestRigidBody::TestGetMass() {
    RigidBody obj(BodyType::Dynamic, 3.5f);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 3.5f, obj.GetMass());
}

void TestRigidBody::TestSetMass() {
    RigidBody obj;
    obj.SetMass(10.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 10.0f, obj.GetMass());
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.1f, obj.GetInverseMass());
}

void TestRigidBody::TestGetVelocity() {
    RigidBody obj;
    Vector3D v = obj.GetVelocity();
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, v.X);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, v.Y);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, v.Z);
}

void TestRigidBody::TestSetVelocity() {
    RigidBody obj;
    obj.SetVelocity(Vector3D(1.0f, 2.0f, 3.0f));
    Vector3D v = obj.GetVelocity();
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, v.X);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 2.0f, v.Y);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 3.0f, v.Z);
}

void TestRigidBody::TestApplyForce() {
    RigidBody obj;
    SphereCollider collider(Vector3D(0, 0, 0), 1.0f);
    obj.SetCollider(&collider);
    obj.ApplyForce(Vector3D(10, 0, 0));
    // Integrate to see force effect: v += (F/m)*dt
    obj.Integrate(1.0f, Vector3D(0, 0, 0));
    Vector3D v = obj.GetVelocity();
    TEST_ASSERT_FLOAT_WITHIN(0.2f, 10.0f, v.X);
}

void TestRigidBody::TestApplyImpulse() {
    RigidBody obj;
    obj.ApplyImpulse(Vector3D(5, 0, 0));
    Vector3D v = obj.GetVelocity();
    // Impulse directly changes velocity: v += impulse * inverseMass
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 5.0f, v.X);
}

void TestRigidBody::TestClearForces() {
    RigidBody obj;
    SphereCollider collider(Vector3D(0, 0, 0), 1.0f);
    obj.SetCollider(&collider);
    obj.ApplyForce(Vector3D(10, 0, 0));
    obj.ClearForces();
    // Integrate with no gravity - velocity should not change from force
    obj.Integrate(1.0f, Vector3D(0, 0, 0));
    Vector3D v = obj.GetVelocity();
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, v.X);
}

void TestRigidBody::TestSetRestitution() {
    RigidBody obj;
    obj.SetRestitution(0.8f);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.8f, obj.GetRestitution());
}

void TestRigidBody::TestSetFriction() {
    RigidBody obj;
    obj.SetFriction(0.5f);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.5f, obj.GetFriction());
}

void TestRigidBody::TestSetLinearDamping() {
    RigidBody obj;
    obj.SetLinearDamping(0.3f);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.3f, obj.GetLinearDamping());
}

// ========== Edge Cases ==========

void TestRigidBody::TestEdgeCases() {
    // Collider defaults to nullptr
    RigidBody obj;
    TEST_ASSERT_TRUE(obj.GetCollider() == nullptr);

    // Set and get collider
    SphereCollider collider;
    obj.SetCollider(&collider);
    TEST_ASSERT_TRUE(obj.GetCollider() == &collider);

    // Set collider back to null
    obj.SetCollider(nullptr);
    TEST_ASSERT_TRUE(obj.GetCollider() == nullptr);
}

// ========== split integration ==========

// ========== bullet flag ==========

// ========== Test Runner ==========

void TestRigidBody::TestApplyAngularImpulse() {
    // TODO: Implement test for ApplyAngularImpulse()
    RigidBody obj;
    TEST_ASSERT_TRUE(false);  // Not implemented
}

void TestRigidBody::TestApplyForceAtPoint() {
    // TODO: Implement test for ApplyForceAtPoint()
    RigidBody obj;
    TEST_ASSERT_TRUE(false);  // Not implemented
}

void TestRigidBody::TestApplyTorque() {
    // TODO: Implement test for ApplyTorque()
    RigidBody obj;
    TEST_ASSERT_TRUE(false);  // Not implemented
}

void TestRigidBody::TestGetAngularVelocity() {
    // TODO: Implement test for GetAngularVelocity()
    RigidBody obj;
    TEST_ASSERT_TRUE(false);  // Not implemented
}

void TestRigidBody::TestGetCollider() {
    // TODO: Implement test for GetCollider()
    RigidBody obj;
    TEST_ASSERT_TRUE(false);  // Not implemented
}

void TestRigidBody::TestGetPointVelocity() {
    // TODO: Implement test for GetPointVelocity()
    RigidBody obj;
    TEST_ASSERT_TRUE(false);  // Not implemented
}

void TestRigidBody::TestGetPose() {
    // TODO: Implement test for GetPose()
    RigidBody obj;
    TEST_ASSERT_TRUE(false);  // Not implemented
}

void TestRigidBody::TestGetPreviousPose() {
    // TODO: Implement test for GetPreviousPose()
    RigidBody obj;
    TEST_ASSERT_TRUE(false);  // Not implemented
}

void TestRigidBody::TestMakeDynamic() {
    // TODO: Implement test for MakeDynamic()
    RigidBody obj;
    TEST_ASSERT_TRUE(false);  // Not implemented
}

void TestRigidBody::TestMakeKinematic() {
    // TODO: Implement test for MakeKinematic()
    RigidBody obj;
    TEST_ASSERT_TRUE(false);  // Not implemented
}

void TestRigidBody::TestMakeStatic() {
    // TODO: Implement test for MakeStatic()
    RigidBody obj;
    TEST_ASSERT_TRUE(false);  // Not implemented
}

void TestRigidBody::TestSetAngularDamping() {
    // TODO: Implement test for SetAngularDamping()
    RigidBody obj;
    TEST_ASSERT_TRUE(false);  // Not implemented
}

void TestRigidBody::TestSetAngularVelocity() {
    // TODO: Implement test for SetAngularVelocity()
    RigidBody obj;
    TEST_ASSERT_TRUE(false);  // Not implemented
}

void TestRigidBody::TestSetCollider() {
    // TODO: Implement test for SetCollider()
    RigidBody obj;
    TEST_ASSERT_TRUE(false);  // Not implemented
}

void TestRigidBody::TestSetInertiaBox() {
    // TODO: Implement test for SetInertiaBox()
    RigidBody obj;
    TEST_ASSERT_TRUE(false);  // Not implemented
}

void TestRigidBody::TestSetInertiaCapsule() {
    // TODO: Implement test for SetInertiaCapsule()
    RigidBody obj;
    TEST_ASSERT_TRUE(false);  // Not implemented
}

void TestRigidBody::TestSetInertiaCylinder() {
    // TODO: Implement test for SetInertiaCylinder()
    RigidBody obj;
    TEST_ASSERT_TRUE(false);  // Not implemented
}

void TestRigidBody::TestSetInertiaSphere() {
    // TODO: Implement test for SetInertiaSphere()
    RigidBody obj;
    TEST_ASSERT_TRUE(false);  // Not implemented
}

void TestRigidBody::TestSetPose() {
    // TODO: Implement test for SetPose()
    RigidBody obj;
    TEST_ASSERT_TRUE(false);  // Not implemented
}

void TestRigidBody::TestGetAllowSleep() {
    // TODO: Implement test for GetAllowSleep()
    RigidBody obj;
    TEST_ASSERT_TRUE(false);  // Not implemented
}

void TestRigidBody::TestIsSleeping() {
    // TODO: Implement test for IsSleeping()
    RigidBody obj;
    TEST_ASSERT_TRUE(false);  // Not implemented
}

void TestRigidBody::TestSetAllowSleep() {
    // TODO: Implement test for SetAllowSleep()
    RigidBody obj;
    TEST_ASSERT_TRUE(false);  // Not implemented
}

void TestRigidBody::TestSleep() {
    // TODO: Implement test for Sleep()
    RigidBody obj;
    TEST_ASSERT_TRUE(false);  // Not implemented
}

void TestRigidBody::TestWake() {
    // TODO: Implement test for Wake()
    RigidBody obj;
    TEST_ASSERT_TRUE(false);  // Not implemented
}

void TestRigidBody::RunAllTests() {
    RUN_TEST(TestDefaultConstructor);
    RUN_TEST(TestParameterizedConstructor);
    RUN_TEST(TestGetBodyType);
    RUN_TEST(TestIsDynamic);
    RUN_TEST(TestIsStatic);
    RUN_TEST(TestIsKinematic);
    RUN_TEST(TestGetMass);
    RUN_TEST(TestSetMass);
    RUN_TEST(TestGetVelocity);
    RUN_TEST(TestSetVelocity);
    RUN_TEST(TestApplyForce);
    RUN_TEST(TestApplyImpulse);
    RUN_TEST(TestClearForces);
    RUN_TEST(TestSetRestitution);
    RUN_TEST(TestSetFriction);
    RUN_TEST(TestSetLinearDamping);
    RUN_TEST(TestEdgeCases);

    RUN_TEST(TestApplyAngularImpulse);
    RUN_TEST(TestApplyForceAtPoint);
    RUN_TEST(TestApplyTorque);
    RUN_TEST(TestGetAngularVelocity);
    RUN_TEST(TestGetCollider);
    RUN_TEST(TestGetPointVelocity);
    RUN_TEST(TestGetPose);
    RUN_TEST(TestGetPreviousPose);
    RUN_TEST(TestMakeDynamic);
    RUN_TEST(TestMakeKinematic);
    RUN_TEST(TestMakeStatic);
    RUN_TEST(TestSetAngularDamping);
    RUN_TEST(TestSetAngularVelocity);
    RUN_TEST(TestSetCollider);
    RUN_TEST(TestSetInertiaBox);
    RUN_TEST(TestSetInertiaCapsule);
    RUN_TEST(TestSetInertiaCylinder);
    RUN_TEST(TestSetInertiaSphere);
    RUN_TEST(TestSetPose);
    RUN_TEST(TestGetAllowSleep);
    RUN_TEST(TestIsSleeping);
    RUN_TEST(TestSetAllowSleep);
    RUN_TEST(TestSleep);
    RUN_TEST(TestWake);
}
