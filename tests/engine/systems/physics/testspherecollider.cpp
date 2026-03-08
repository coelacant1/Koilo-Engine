// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testspherecollider.cpp
 * @brief Implementation of SphereCollider unit tests.
 *
 * @date 24/10/2025
 * @author Coela
 */

#include "testspherecollider.hpp"
#include <koilo/systems/physics/spherecollider.hpp>
#include <koilo/core/geometry/ray.hpp>
#include <cmath>

using namespace koilo;

void TestSphereCollider::TestDefaultConstructor() {
    SphereCollider sphere;
    
    Vector3D pos = sphere.GetPosition();
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, pos.X);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, pos.Y);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, pos.Z);
    
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, sphere.GetRadius());
}

void TestSphereCollider::TestParameterizedConstructor() {
    Vector3D center(5.0f, 10.0f, 15.0f);
    float radius = 3.5f;
    
    SphereCollider sphere(center, radius);
    
    Vector3D pos = sphere.GetPosition();
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 5.0f, pos.X);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 10.0f, pos.Y);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 15.0f, pos.Z);
    
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 3.5f, sphere.GetRadius());
}

void TestSphereCollider::TestGetPosition() {
    SphereCollider sphere(Vector3D(1, 2, 3), 5.0f);
    
    Vector3D pos = sphere.GetPosition();
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, pos.X);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 2.0f, pos.Y);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 3.0f, pos.Z);
}

void TestSphereCollider::TestSetPosition() {
    SphereCollider sphere;
    
    sphere.SetPosition(Vector3D(10, 20, 30));
    
    Vector3D pos = sphere.GetPosition();
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 10.0f, pos.X);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 20.0f, pos.Y);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 30.0f, pos.Z);
}

void TestSphereCollider::TestClosestPoint() {
    SphereCollider sphere(Vector3D(0, 0, 0), 2.0f);

    // Point outside sphere: closest point should be on the surface
    Vector3D closest = sphere.ClosestPoint(Vector3D(5, 0, 0));
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 2.0f, closest.X);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, closest.Y);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, closest.Z);

    // Point inside sphere: closest point is the point itself (already contained)
    Vector3D insideClosest = sphere.ClosestPoint(Vector3D(0.5f, 0, 0));
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.5f, insideClosest.X);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, insideClosest.Y);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, insideClosest.Z);
}

void TestSphereCollider::TestContainsPoint() {
    SphereCollider sphere(Vector3D(0, 0, 0), 3.0f);

    // Point inside
    TEST_ASSERT_TRUE(sphere.ContainsPoint(Vector3D(1, 1, 1)));

    // Point outside
    TEST_ASSERT_TRUE(!sphere.ContainsPoint(Vector3D(10, 0, 0)));

    // Point on surface (boundary)
    TEST_ASSERT_TRUE(sphere.ContainsPoint(Vector3D(3, 0, 0)));
}

void TestSphereCollider::TestEdgeCases() {
    // Zero-radius sphere at origin
    SphereCollider tiny(Vector3D(0, 0, 0), 0.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, tiny.GetRadius());

    // Large sphere
    SphereCollider big(Vector3D(0, 0, 0), 1000.0f);
    TEST_ASSERT_TRUE(big.ContainsPoint(Vector3D(500, 500, 0)));
}

void TestSphereCollider::TestScriptRaycast() {
    // TODO: Implement test for ScriptRaycast()
    SphereCollider obj;
    TEST_IGNORE_MESSAGE("Stub");
}

void TestSphereCollider::RunAllTests() {
    RUN_TEST(TestDefaultConstructor);
    RUN_TEST(TestParameterizedConstructor);
    RUN_TEST(TestGetPosition);
    RUN_TEST(TestSetPosition);

    RUN_TEST(TestClosestPoint);
    RUN_TEST(TestContainsPoint);
    RUN_TEST(TestEdgeCases);

    RUN_TEST(TestScriptRaycast);
}
