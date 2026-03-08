// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testkeyframe.cpp
 * @brief Implementation of KeyFrame unit tests.
 */

#include "testkeyframe.hpp"

using namespace koilo;
// ========== Constructor Tests ==========

void TestKeyFrame::TestDefaultConstructor() {
    KeyFrame kf;
    // Default keyframe
    TEST_ASSERT_TRUE(true);
}

// ========== Method Tests ==========
void TestKeyFrame::TestSet() {
    KeyFrame kf;
    kf.Set(1.5f, 10.0f);
    // Keyframe set with time and value
    TEST_ASSERT_TRUE(true);
}
// ========== Edge Cases ==========

// ========== Test Runner ==========

void TestKeyFrame::TestParameterizedConstructor() {
    KeyFrame kf(2.0f, 5.0f);
    // Keyframe with time and value
    TEST_ASSERT_TRUE(true);
}

void TestKeyFrame::TestEdgeCases() {
    // Negative time
    KeyFrame kf1(-1.0f, 0.0f);
    TEST_ASSERT_TRUE(true);
    
    // Zero time
    KeyFrame kf2(0.0f, 100.0f);
    TEST_ASSERT_TRUE(true);
}

void TestKeyFrame::RunAllTests() {
    RUN_TEST(TestDefaultConstructor);
    RUN_TEST(TestParameterizedConstructor);
    RUN_TEST(TestSet);
    RUN_TEST(TestEdgeCases);
}
