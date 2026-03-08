// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testboxcollider.cpp
 * @brief Implementation of BoxCollider unit tests.
 *
 * @date 24/10/2025
 * @author Coela
 */

#include "testboxcollider.hpp"
#include <koilo/systems/physics/boxcollider.hpp>
#include <koilo/core/geometry/ray.hpp>

using namespace koilo;

void TestBoxCollider::TestDefaultConstructor() {
    BoxCollider box;
    
    Vector3D pos = box.GetPosition();
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, pos.X);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, pos.Y);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, pos.Z);
    
    Vector3D size = box.GetSize();
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, size.X);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, size.Y);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, size.Z);
}

void TestBoxCollider::TestParameterizedConstructor() {
    Vector3D center(5.0f, 10.0f, 15.0f);
    Vector3D size(2.0f, 4.0f, 6.0f);
    
    BoxCollider box(center, size);
    
    Vector3D pos = box.GetPosition();
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 5.0f, pos.X);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 10.0f, pos.Y);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 15.0f, pos.Z);
    
    Vector3D retrievedSize = box.GetSize();
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 2.0f, retrievedSize.X);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 4.0f, retrievedSize.Y);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 6.0f, retrievedSize.Z);
}

void TestBoxCollider::TestGetPosition() {
    BoxCollider box(Vector3D(1, 2, 3), Vector3D(1, 1, 1));
    
    Vector3D pos = box.GetPosition();
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, pos.X);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 2.0f, pos.Y);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 3.0f, pos.Z);
}

void TestBoxCollider::TestSetPosition() {
    BoxCollider box;
    
    box.SetPosition(Vector3D(10, 20, 30));
    
    Vector3D pos = box.GetPosition();
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 10.0f, pos.X);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 20.0f, pos.Y);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 30.0f, pos.Z);
}

void TestBoxCollider::TestClosestPoint() {
    // Box centered at origin with size (2,2,2) -> half-extents (1,1,1)
    BoxCollider box(Vector3D(0, 0, 0), Vector3D(2, 2, 2));

    // Point outside along +X: closest should be on the +X face
    Vector3D closest = box.ClosestPoint(Vector3D(5, 0, 0));
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, closest.X);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, closest.Y);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, closest.Z);
}

void TestBoxCollider::TestContainsPoint() {
    BoxCollider box(Vector3D(0, 0, 0), Vector3D(4, 4, 4));

    // Point inside
    TEST_ASSERT_TRUE(box.ContainsPoint(Vector3D(1, 1, 1)));

    // Point outside
    TEST_ASSERT_TRUE(!box.ContainsPoint(Vector3D(10, 0, 0)));

    // Point on boundary
    TEST_ASSERT_TRUE(box.ContainsPoint(Vector3D(2, 0, 0)));
}

void TestBoxCollider::TestEdgeCases() {
    // Default box
    BoxCollider box;
    Vector3D size = box.GetSize();
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, size.X);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, size.Y);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, size.Z);

    // Large box contains distant interior point
    BoxCollider big(Vector3D(0, 0, 0), Vector3D(2000, 2000, 2000));
    TEST_ASSERT_TRUE(big.ContainsPoint(Vector3D(500, 500, 500)));
}

void TestBoxCollider::TestScriptRaycast() {
    // TODO: Implement test for ScriptRaycast()
    BoxCollider obj;
    TEST_IGNORE_MESSAGE("Stub");
}

void TestBoxCollider::RunAllTests() {
    RUN_TEST(TestDefaultConstructor);
    RUN_TEST(TestParameterizedConstructor);
    RUN_TEST(TestGetPosition);
    RUN_TEST(TestSetPosition);

    RUN_TEST(TestClosestPoint);
    RUN_TEST(TestContainsPoint);
    RUN_TEST(TestEdgeCases);

    RUN_TEST(TestScriptRaycast);
}
