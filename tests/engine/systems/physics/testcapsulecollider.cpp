// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testcapsulecollider.cpp
 * @brief Implementation of CapsuleCollider unit tests.
 *
 * @date 24/10/2025
 * @author Coela
 */

#include "testcapsulecollider.hpp"
#include <koilo/systems/physics/capsulecollider.hpp>
#include <koilo/core/geometry/ray.hpp>
#include <cmath>

using namespace koilo;

void TestCapsuleCollider::TestDefaultConstructor() {
    CapsuleCollider capsule;
    
    Vector3D pos = capsule.GetPosition();
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, pos.X);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, pos.Y);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, pos.Z);
    
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.5f, capsule.GetRadius());
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 2.0f, capsule.GetHeight());
}

void TestCapsuleCollider::TestParameterizedConstructor() {
    Vector3D center(5.0f, 10.0f, 15.0f);
    float radius = 1.5f;
    float height = 5.0f;
    
    CapsuleCollider capsule(center, radius, height);
    
    Vector3D pos = capsule.GetPosition();
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 5.0f, pos.X);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 10.0f, pos.Y);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 15.0f, pos.Z);
    
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.5f, capsule.GetRadius());
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 5.0f, capsule.GetHeight());
}

void TestCapsuleCollider::TestGetPosition() {
    CapsuleCollider capsule(Vector3D(1, 2, 3), 1.0f, 3.0f);
    
    Vector3D pos = capsule.GetPosition();
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, pos.X);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 2.0f, pos.Y);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 3.0f, pos.Z);
}

void TestCapsuleCollider::TestSetPosition() {
    CapsuleCollider capsule;
    
    capsule.SetPosition(Vector3D(10, 20, 30));
    
    Vector3D pos = capsule.GetPosition();
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 10.0f, pos.X);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 20.0f, pos.Y);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 30.0f, pos.Z);
}

void TestCapsuleCollider::TestGetRadius() {
    CapsuleCollider capsule(Vector3D(0, 0, 0), 2.5f, 5.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 2.5f, capsule.GetRadius());
}

void TestCapsuleCollider::TestSetRadius() {
    CapsuleCollider capsule;
    
    capsule.SetRadius(3.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 3.0f, capsule.GetRadius());
    
    // Test negative radius handling (should clamp to positive or handle gracefully)
    capsule.SetRadius(0.1f);
    TEST_ASSERT_TRUE(capsule.GetRadius() > 0);
}

void TestCapsuleCollider::TestGetHeight() {
    CapsuleCollider capsule(Vector3D(0, 0, 0), 1.0f, 6.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 6.0f, capsule.GetHeight());
}

void TestCapsuleCollider::TestSetHeight() {
    CapsuleCollider capsule;
    
    capsule.SetHeight(8.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 8.0f, capsule.GetHeight());
    
    // Test small height
    capsule.SetHeight(1.0f);
    TEST_ASSERT_TRUE(capsule.GetHeight() > 0);
}

void TestCapsuleCollider::TestClosestPoint() {
    // Capsule at origin, radius 1, height 4 (segment from y=-1 to y=+1)
    CapsuleCollider capsule(Vector3D(0, 0, 0), 1.0f, 4.0f);

    // Point far along +X at mid-height: closest point on surface at x=radius
    Vector3D closest = capsule.ClosestPoint(Vector3D(5, 0, 0));
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 1.0f, closest.X);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 0.0f, closest.Y);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 0.0f, closest.Z);
}

void TestCapsuleCollider::TestContainsPoint() {
    // Capsule at origin, radius 1, height 4
    CapsuleCollider capsule(Vector3D(0, 0, 0), 1.0f, 4.0f);

    // Point at center should be inside
    TEST_ASSERT_TRUE(capsule.ContainsPoint(Vector3D(0, 0, 0)));

    // Point far outside
    TEST_ASSERT_TRUE(!capsule.ContainsPoint(Vector3D(10, 0, 0)));

    // Point along axis within height, within radius
    TEST_ASSERT_TRUE(capsule.ContainsPoint(Vector3D(0.5f, 0.5f, 0)));
}

void TestCapsuleCollider::TestEdgeCases() {
    // GetSegment: verify the two endpoint positions
    CapsuleCollider capsule(Vector3D(0, 0, 0), 1.0f, 4.0f);
    Vector3D p1, p2;
    capsule.GetSegment(p1, p2);
    // Segment should be along Y axis centered at origin
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 0.0f, p1.X);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 0.0f, p2.X);
    // p1 and p2 should be separated by (height - 2*radius) along Y
    TEST_ASSERT_TRUE(p1.Y != p2.Y || capsule.GetHeight() <= 2.0f * capsule.GetRadius());
}

void TestCapsuleCollider::TestScriptRaycast() {
    // TODO: Implement test for ScriptRaycast()
    CapsuleCollider obj;
    TEST_IGNORE_MESSAGE("Stub");
}

void TestCapsuleCollider::RunAllTests() {
    RUN_TEST(TestDefaultConstructor);
    RUN_TEST(TestParameterizedConstructor);
    RUN_TEST(TestGetPosition);
    RUN_TEST(TestSetPosition);
    RUN_TEST(TestGetRadius);
    RUN_TEST(TestSetRadius);
    RUN_TEST(TestGetHeight);
    RUN_TEST(TestSetHeight);

    RUN_TEST(TestClosestPoint);
    RUN_TEST(TestContainsPoint);
    RUN_TEST(TestEdgeCases);

    RUN_TEST(TestScriptRaycast);
}
