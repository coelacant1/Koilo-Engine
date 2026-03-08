// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testphysicsworld.cpp
 * @brief Implementation of PhysicsWorld unit tests.
 */

#include "testphysicsworld.hpp"
#include <koilo/systems/physics/spherecollider.hpp>

using namespace koilo;

// ========== Constructor Tests ==========

void TestPhysicsWorld::TestDefaultConstructor() {
    PhysicsWorld obj;
    TEST_ASSERT_EQUAL(0, obj.GetBodyCount());
    Vector3D g = obj.GetGravity();
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, g.X);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, -9.81f, g.Y);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, g.Z);
}

void TestPhysicsWorld::TestParameterizedConstructor() {
    PhysicsWorld obj(Vector3D(0, -20, 0));
    Vector3D g = obj.GetGravity();
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, g.X);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, -20.0f, g.Y);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, g.Z);
}

// ========== Method Tests ==========

void TestPhysicsWorld::TestAddBody() {
    PhysicsWorld obj;
    RigidBody body;
    obj.AddBody(&body);
    TEST_ASSERT_EQUAL(1, obj.GetBodyCount());
}

void TestPhysicsWorld::TestRemoveBody() {
    PhysicsWorld obj;
    RigidBody body;
    obj.AddBody(&body);
    TEST_ASSERT_EQUAL(1, obj.GetBodyCount());
    obj.RemoveBody(&body);
    TEST_ASSERT_EQUAL(0, obj.GetBodyCount());
}

void TestPhysicsWorld::TestRemoveAllBodies() {
    PhysicsWorld obj;
    RigidBody b1, b2, b3;
    obj.AddBody(&b1);
    obj.AddBody(&b2);
    obj.AddBody(&b3);
    TEST_ASSERT_EQUAL(3, obj.GetBodyCount());
    obj.RemoveAllBodies();
    TEST_ASSERT_EQUAL(0, obj.GetBodyCount());
}

void TestPhysicsWorld::TestGetBodyCount() {
    PhysicsWorld obj;
    TEST_ASSERT_EQUAL(0, obj.GetBodyCount());
    RigidBody b1, b2;
    obj.AddBody(&b1);
    TEST_ASSERT_EQUAL(1, obj.GetBodyCount());
    obj.AddBody(&b2);
    TEST_ASSERT_EQUAL(2, obj.GetBodyCount());
}

void TestPhysicsWorld::TestGetGravity() {
    PhysicsWorld obj;
    Vector3D g = obj.GetGravity();
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, g.X);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, -9.81f, g.Y);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, g.Z);
}

void TestPhysicsWorld::TestSetGravity() {
    PhysicsWorld obj;
    obj.SetGravity(Vector3D(0, -15.0f, 0));
    Vector3D g = obj.GetGravity();
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, g.X);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, -15.0f, g.Y);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, g.Z);
}

void TestPhysicsWorld::TestSetFixedTimestep() {
    PhysicsWorld obj;
    obj.SetFixedTimestep(0.02f);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.02f, obj.GetFixedTimestep());
}

void TestPhysicsWorld::TestSetMaxSubSteps() {
    PhysicsWorld obj;
    obj.SetMaxSubSteps(5);
    TEST_ASSERT_EQUAL(5, obj.GetMaxSubSteps());
}

void TestPhysicsWorld::TestClearCollisionCallbacks() {
    PhysicsWorld obj;
    obj.ClearCollisionCallbacks();
    TEST_ASSERT_TRUE(true);
}

void TestPhysicsWorld::TestStep() {
    PhysicsWorld obj;
    SphereCollider collider(Vector3D(0, 10, 0), 1.0f);
    RigidBody body(BodyType::Dynamic, 1.0f);
    body.SetCollider(&collider);
    obj.AddBody(&body);
    obj.Step();
    TEST_ASSERT_TRUE(true);
}

// ========== Edge Cases ==========

void TestPhysicsWorld::TestEdgeCases() {
    PhysicsWorld obj;
    // Remove nullptr should not crash
    obj.RemoveBody(nullptr);
    TEST_ASSERT_EQUAL(0, obj.GetBodyCount());

    // RemoveAllBodies on empty world
    obj.RemoveAllBodies();
    TEST_ASSERT_EQUAL(0, obj.GetBodyCount());
}

// ========== Test Runner ==========

void TestPhysicsWorld::RunAllTests() {
    RUN_TEST(TestDefaultConstructor);
    RUN_TEST(TestParameterizedConstructor);
    RUN_TEST(TestAddBody);
    RUN_TEST(TestRemoveBody);
    RUN_TEST(TestRemoveAllBodies);
    RUN_TEST(TestGetBodyCount);
    RUN_TEST(TestGetGravity);
    RUN_TEST(TestSetGravity);
    RUN_TEST(TestSetFixedTimestep);
    RUN_TEST(TestSetMaxSubSteps);
    RUN_TEST(TestClearCollisionCallbacks);
    RUN_TEST(TestStep);
    RUN_TEST(TestEdgeCases);
}
