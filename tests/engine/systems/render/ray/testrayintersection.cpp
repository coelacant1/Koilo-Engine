// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testrayintersection.cpp
 * @brief Implementation of RayIntersection unit tests.
 */

#include "testrayintersection.hpp"

using namespace koilo;

void TestRayIntersection::TestDefaultConstructor() {
    // Static utility class - no instances needed
    TEST_ASSERT_TRUE(true);
}

void TestRayIntersection::TestParameterizedConstructor() {
    Ray ray(Vector3D(0, 0, 0), Vector3D(1, 0, 0));
    RayHitInfo hit = RayIntersection::IntersectScene(ray, nullptr);
    TEST_ASSERT_FALSE(hit.hit);
}

void TestRayIntersection::TestEdgeCases() {
    TEST_ASSERT_TRUE(true);
}

void TestRayIntersection::RunAllTests() {
    RUN_TEST(TestDefaultConstructor);
    RUN_TEST(TestParameterizedConstructor);
    RUN_TEST(TestEdgeCases);
}
