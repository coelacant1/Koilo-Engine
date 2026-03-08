// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testwait.cpp
 * @brief Implementation of Wait unit tests.
 */

#include "testwait.hpp"
#include <unistd.h>

using namespace koilo;
// ========== Constructor Tests ==========

void TestWait::TestDefaultConstructor() {
    Wait wait(0);
    TEST_ASSERT_TRUE(wait.IsFinished());
}

// ========== Method Tests ==========
void TestWait::TestReset() {
    Wait wait(100);
    wait.Reset();
    TEST_ASSERT_FALSE(wait.IsFinished());
}

void TestWait::TestIsFinished() {
    Wait wait(1);
    TEST_ASSERT_FALSE(wait.IsFinished());
    // After sufficient time, should finish (use usleep for desktop)
    usleep(50000);  // 50ms
    TEST_ASSERT_TRUE(wait.IsFinished());
}

// ========== Edge Cases ==========

// ========== Test Runner ==========

void TestWait::TestParameterizedConstructor() {
    Wait wait(100);
    TEST_ASSERT_FALSE(wait.IsFinished());
}

void TestWait::TestEdgeCases() {
    Wait zeroWait(0);
    TEST_ASSERT_TRUE(zeroWait.IsFinished());
    
    Wait longWait(10000);
    TEST_ASSERT_FALSE(longWait.IsFinished());
}

void TestWait::RunAllTests() {
    RUN_TEST(TestDefaultConstructor);
    RUN_TEST(TestParameterizedConstructor);
    RUN_TEST(TestReset);
    RUN_TEST(TestIsFinished);
    RUN_TEST(TestEdgeCases);
}
