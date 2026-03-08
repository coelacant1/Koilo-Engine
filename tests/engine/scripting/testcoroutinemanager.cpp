// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testcoroutinemanager.cpp
 * @brief Implementation of CoroutineManager unit tests.
 */

#include "testcoroutinemanager.hpp"

using namespace koilo;
using namespace koilo::scripting;

// ========== Constructor Tests ==========

void TestCoroutineManager::TestDefaultConstructor() {
    // TODO: Implement test for default constructor
    CoroutineManager obj;
    TEST_IGNORE_MESSAGE("Stub");
}

void TestCoroutineManager::TestParameterizedConstructor() {
    // TODO: Implement test for parameterized constructor
    TEST_IGNORE_MESSAGE("Stub");
}

// ========== Method Tests ==========

void TestCoroutineManager::TestStart() {
    // TODO: Implement test for Start()
    CoroutineManager obj;
    TEST_IGNORE_MESSAGE("Stub");
}

void TestCoroutineManager::TestResumeAll() {
    // TODO: Implement test for ResumeAll()
    CoroutineManager obj;
    TEST_IGNORE_MESSAGE("Stub");
}

void TestCoroutineManager::TestHasActive() {
    // TODO: Implement test for HasActive()
    CoroutineManager obj;
    TEST_IGNORE_MESSAGE("Stub");
}

void TestCoroutineManager::TestCount() {
    // TODO: Implement test for Count()
    CoroutineManager obj;
    TEST_IGNORE_MESSAGE("Stub");
}

void TestCoroutineManager::TestClear() {
    // TODO: Implement test for Clear()
    CoroutineManager obj;
    TEST_IGNORE_MESSAGE("Stub");
}

void TestCoroutineManager::TestPendingStarts() {
    // TODO: Implement test for PendingStarts()
    CoroutineManager obj;
    TEST_IGNORE_MESSAGE("Stub");
}

void TestCoroutineManager::TestActive() {
    // TODO: Implement test for Active()
    CoroutineManager obj;
    TEST_IGNORE_MESSAGE("Stub");
}

// ========== Edge Cases ==========

void TestCoroutineManager::TestEdgeCases() {
    // TODO: Test edge cases (null, boundaries, extreme values)
    TEST_IGNORE_MESSAGE("Stub");
}

// ========== Test Runner ==========

void TestCoroutineManager::TestStopByName() {
    CoroutineManager mgr;
    // Start creates a pending entry; simulate an active coroutine
    mgr.Active().emplace_back();
    mgr.Active().back().name = "testFunc";
    mgr.Active().back().finished = false;
    TEST_ASSERT_TRUE(mgr.StopByName("testFunc"));
    TEST_ASSERT_TRUE(mgr.Active().back().finished);
    TEST_ASSERT_FALSE(mgr.StopByName("nonexistent"));
}

void TestCoroutineManager::RunAllTests() {
    RUN_TEST(TestDefaultConstructor);
    RUN_TEST(TestParameterizedConstructor);
    RUN_TEST(TestStart);
    RUN_TEST(TestResumeAll);
    RUN_TEST(TestHasActive);
    RUN_TEST(TestCount);
    RUN_TEST(TestClear);
    RUN_TEST(TestPendingStarts);
    RUN_TEST(TestActive);
    RUN_TEST(TestEdgeCases);
    RUN_TEST(TestStopByName);
}
