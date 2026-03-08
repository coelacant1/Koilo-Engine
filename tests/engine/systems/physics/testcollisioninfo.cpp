// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testcollisioninfo.cpp
 * @brief Implementation of CollisionInfo unit tests.
 */

#include "testcollisioninfo.hpp"

using namespace koilo;
// ========== Constructor Tests ==========

void TestCollisionInfo::TestDefaultConstructor() {
    CollisionInfo info;
    // Verify default values
    TEST_ASSERT_NULL(info.colliderA);
    TEST_ASSERT_NULL(info.colliderB);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, info.contactPoint.X);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, info.contactPoint.Y);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, info.contactPoint.Z);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, info.penetrationDepth);
}

void TestCollisionInfo::TestParameterizedConstructor() {
    // CollisionInfo only has default constructor
    CollisionInfo info;
    info.penetrationDepth = 0.5f;
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.5f, info.penetrationDepth);
}

// ========== Edge Cases ==========

void TestCollisionInfo::TestEdgeCases() {
    CollisionInfo info;
    
    // Set contact point
    info.contactPoint = Vector3D(1.0f, 2.0f, 3.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, info.contactPoint.X);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 2.0f, info.contactPoint.Y);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 3.0f, info.contactPoint.Z);
    
    // Set normal
    info.normal = Vector3D(0.0f, 1.0f, 0.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, info.normal.Y);
}

// ========== Test Runner ==========

void TestCollisionInfo::RunAllTests() {
    RUN_TEST(TestDefaultConstructor);
    RUN_TEST(TestParameterizedConstructor);
    RUN_TEST(TestEdgeCases);
}
