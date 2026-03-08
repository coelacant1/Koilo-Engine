// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testraycasthit.cpp
 * @brief Implementation of RaycastHit unit tests.
 */

#include "testraycasthit.hpp"

using namespace koilo;
// ========== Constructor Tests ==========

void TestRaycastHit::TestDefaultConstructor() {
    RaycastHit hit;
    // Verify default state
    TEST_ASSERT_NULL(hit.collider);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, hit.distance);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, hit.point.X);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, hit.point.Y);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, hit.point.Z);
}

void TestRaycastHit::TestParameterizedConstructor() {
    // RaycastHit only has default constructor
    RaycastHit hit;
    hit.distance = 5.0f;
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 5.0f, hit.distance);
}

// ========== Edge Cases ==========

void TestRaycastHit::TestEdgeCases() {
    RaycastHit hit;
    
    // Set hit point
    hit.point = Vector3D(10.0f, 20.0f, 30.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 10.0f, hit.point.X);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 20.0f, hit.point.Y);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 30.0f, hit.point.Z);
    
    // Set normal
    hit.normal = Vector3D(0.0f, 0.0f, 1.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, hit.normal.Z);
    
    // Large distance
    hit.distance = 1000.0f;
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 1000.0f, hit.distance);
}

// ========== Test Runner ==========

void TestRaycastHit::RunAllTests() {
    RUN_TEST(TestDefaultConstructor);
    RUN_TEST(TestParameterizedConstructor);
    RUN_TEST(TestEdgeCases);
}
