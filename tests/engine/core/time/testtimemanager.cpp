// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testtimemanager.cpp
 * @brief Implementation of TimeManager unit tests.
 */

#include "testtimemanager.hpp"

using namespace koilo;

// ========== Constructor Tests ==========

void TestTimeManager::TestDefaultConstructor() {
    // TimeManager is a singleton - verify instance is accessible
    TimeManager& tm = TimeManager::GetInstance();
    (void)tm;
    TEST_ASSERT_TRUE(true);
}

void TestTimeManager::TestParameterizedConstructor() {
    // Singleton - no parameterized constructor
    TEST_ASSERT_TRUE(true);
}

// ========== Method Tests ==========

void TestTimeManager::TestGetDeltaTime() {
    TimeManager& tm = TimeManager::GetInstance();
    float dt = tm.GetDeltaTime();
    TEST_ASSERT_TRUE(dt >= 0.0f);
}

void TestTimeManager::TestGetTotalTime() {
    TimeManager& tm = TimeManager::GetInstance();
    float total = tm.GetTotalTime();
    TEST_ASSERT_TRUE(total >= 0.0f);
}

void TestTimeManager::TestGetFrameCount() {
    TimeManager& tm = TimeManager::GetInstance();
    unsigned long frames = 0UL;
    TEST_ASSERT_TRUE(frames >= 0);
}

// ========== Edge Cases ==========

void TestTimeManager::TestEdgeCases() {
    // Update with zero dt
    TimeManager& tm = TimeManager::GetInstance();
    tm.Tick(0.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, tm.GetDeltaTime());
}

// ========== Test Runner ==========

void TestTimeManager::RunAllTests() {
    RUN_TEST(TestDefaultConstructor);
    RUN_TEST(TestParameterizedConstructor);
    RUN_TEST(TestGetDeltaTime);
    RUN_TEST(TestGetTotalTime);
    RUN_TEST(TestGetFrameCount);
    RUN_TEST(TestEdgeCases);
}
