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

// ========== Test Runner ==========

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
}
