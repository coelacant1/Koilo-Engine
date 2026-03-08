// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testray.cpp
 * @brief Implementation of Ray unit tests.
 */

#include "testray.hpp"

using namespace koilo;
// ========== Constructor Tests ==========

void TestRay::TestDefaultConstructor() {
    Ray ray;
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, ray.origin.X);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, ray.origin.Y);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, ray.origin.Z);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, ray.direction.X);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, ray.direction.Y);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, ray.direction.Z);
}

void TestRay::TestParameterizedConstructor() {
    Vector3D origin(1.0f, 2.0f, 3.0f);
    Vector3D direction(1.0f, 0.0f, 0.0f);
    
    Ray ray(origin, direction);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, ray.origin.X);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 2.0f, ray.origin.Y);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 3.0f, ray.origin.Z);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, ray.direction.Magnitude());
}

// ========== Method Tests ==========

void TestRay::TestGetPoint() {
    Vector3D origin(0.0f, 0.0f, 0.0f);
    Vector3D direction(1.0f, 0.0f, 0.0f);
    Ray ray(origin, direction);
    
    Vector3D point1 = ray.GetPoint(0.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, point1.X);
    
    Vector3D point2 = ray.GetPoint(5.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 5.0f, point2.X);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, point2.Y);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, point2.Z);
}

void TestRay::TestNormalize() {
    Vector3D origin(0.0f, 0.0f, 0.0f);
    Vector3D direction(3.0f, 4.0f, 0.0f);
    Ray ray(origin, direction);
    
    ray.Normalize();
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, ray.direction.Magnitude());
}

void TestRay::TestIsNormalized() {
    Vector3D origin(0.0f, 0.0f, 0.0f);
    Vector3D normalDir(1.0f, 0.0f, 0.0f);
    Ray ray1(origin, normalDir);
    TEST_ASSERT_TRUE(ray1.IsNormalized());
    
    Vector3D unnormalDir(5.0f, 0.0f, 0.0f);
    Ray ray2(origin, unnormalDir);
    ray2.direction = unnormalDir;
    TEST_ASSERT_FALSE(ray2.IsNormalized());
}

void TestRay::TestTranslate() {
    Vector3D origin(1.0f, 2.0f, 3.0f);
    Vector3D direction(1.0f, 0.0f, 0.0f);
    Ray ray(origin, direction);
    
    Vector3D offset(5.0f, 10.0f, 15.0f);
    Ray translated = ray.Translate(offset);
    
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 6.0f, translated.origin.X);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 12.0f, translated.origin.Y);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 18.0f, translated.origin.Z);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, translated.direction.X);
}

void TestRay::TestClosestPoint() {
    Vector3D origin(0.0f, 0.0f, 0.0f);
    Vector3D direction(1.0f, 0.0f, 0.0f);
    Ray ray(origin, direction);
    
    Vector3D point(5.0f, 3.0f, 0.0f);
    Vector3D closest = ray.ClosestPoint(point);
    
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 5.0f, closest.X);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, closest.Y);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, closest.Z);
}

void TestRay::TestClosestDistance() {
    Vector3D origin(0.0f, 0.0f, 0.0f);
    Vector3D direction(1.0f, 0.0f, 0.0f);
    Ray ray(origin, direction);
    
    Vector3D point(5.0f, 3.0f, 0.0f);
    float distance = ray.ClosestDistance(point);
    
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 5.0f, distance);
}

// ========== Edge Cases ==========

void TestRay::TestEdgeCases() {
    // Test zero-length direction (should normalize to unit length)
    Vector3D origin(0.0f, 0.0f, 0.0f);
    Vector3D zeroDir(0.0f, 0.0f, 0.0f);
    Ray ray1(origin, zeroDir);
    ray1.Normalize();
    
    // Test FromPoints static method
    Vector3D from(1.0f, 2.0f, 3.0f);
    Vector3D to(4.0f, 6.0f, 8.0f);
    Ray ray2 = Ray::FromPoints(from, to);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, ray2.origin.X);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, ray2.direction.Magnitude());
    
    // Test negative distance (GetPoint should work with t >= 0)
    Vector3D dir(1.0f, 0.0f, 0.0f);
    Ray ray3(origin, dir);
    Vector3D pointNeg = ray3.GetPoint(-5.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, -5.0f, pointNeg.X);
    
    // Test very large distance
    Vector3D pointLarge = ray3.GetPoint(1e6f);
    TEST_ASSERT_FLOAT_WITHIN(1.0f, 1e6f, pointLarge.X);
    
    // Test closest point behind ray origin
    Vector3D behindPoint(-5.0f, 0.0f, 0.0f);
    Vector3D closestBehind = ray3.ClosestPoint(behindPoint);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, closestBehind.X);
}

// ========== Test Runner ==========

void TestRay::RunAllTests() {
    RUN_TEST(TestDefaultConstructor);
    RUN_TEST(TestParameterizedConstructor);
    RUN_TEST(TestGetPoint);
    RUN_TEST(TestNormalize);
    RUN_TEST(TestIsNormalized);
    RUN_TEST(TestTranslate);
    RUN_TEST(TestClosestPoint);
    RUN_TEST(TestClosestDistance);
    RUN_TEST(TestEdgeCases);
}
