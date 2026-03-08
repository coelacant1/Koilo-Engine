// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testrayhitinfo.cpp
 * @brief Implementation of RayHitInfo unit tests.
 */

#include "testrayhitinfo.hpp"

using namespace koilo;
// ========== Constructor Tests ==========

void TestRayHitInfo::TestDefaultConstructor() {
    RayHitInfo hitInfo;
    TEST_ASSERT_TRUE(true);
}

void TestRayHitInfo::TestParameterizedConstructor() {
    RayHitInfo hitInfo;
    hitInfo.point = Vector3D(1.0f, 2.0f, 3.0f);
    hitInfo.normal = Vector3D(0.0f, 1.0f, 0.0f);
    hitInfo.distance = 5.0f;
    hitInfo.hit = true;
    TEST_ASSERT_TRUE(hitInfo.hit);
}

// ========== Edge Cases ==========

void TestRayHitInfo::TestEdgeCases() {
    RayHitInfo hitInfo;
    TEST_ASSERT_FALSE(hitInfo.hit);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, hitInfo.distance);
}

// ========== Test Runner ==========

void TestRayHitInfo::RunAllTests() {
    RUN_TEST(TestDefaultConstructor);
    RUN_TEST(TestParameterizedConstructor);
    RUN_TEST(TestEdgeCases);
}
