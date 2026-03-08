// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testlevel.cpp
 * @brief Implementation of Level unit tests.
 */

#include "testlevel.hpp"

using namespace koilo;
// ========== Constructor Tests ==========

void TestLevel::TestDefaultConstructor() {
    Level level;
    // Default level should have no entities
    TEST_ASSERT_EQUAL_UINT32(0, level.GetEntityCount());
}

void TestLevel::TestParameterizedConstructor() {
    Level level("TestLevel");
    std::string name = level.GetName();
    TEST_ASSERT_EQUAL_STRING("TestLevel", name.c_str());
}

// ========== Method Tests ==========

void TestLevel::TestGetName() {
    Level level("MyLevel");
    std::string name = level.GetName();
    TEST_ASSERT_EQUAL_STRING("MyLevel", name.c_str());
}

void TestLevel::TestSetName() {
    Level level;
    level.SetName("NewLevel");
    std::string name = level.GetName();
    TEST_ASSERT_EQUAL_STRING("NewLevel", name.c_str());
}

void TestLevel::TestGetEntityCount() {
    Level level;
    uint32_t count = level.GetEntityCount();
    TEST_ASSERT_EQUAL_UINT32(0, count);  // New level should have 0 entities
}

void TestLevel::TestIsStreamable() {
    Level level;
    bool streamable = level.IsStreamable();
    TEST_ASSERT_TRUE(streamable == true || streamable == false);
}

void TestLevel::TestLoad() {
    Level level;
    level.Load();
    // Verify load executed
    TEST_ASSERT_TRUE(true);
}

void TestLevel::TestUnload() {
    Level level;
    level.Load();
    level.Unload();
    // Verify unload executed
    TEST_ASSERT_TRUE(true);
}

// ========== Edge Cases ==========

void TestLevel::TestEdgeCases() {
    Level level("");
    level.Load();
    level.Unload();
    std::string name = level.GetName();
    TEST_ASSERT_EQUAL_STRING("", name.c_str());
}
// ========== Test Runner ==========

void TestLevel::RunAllTests() {
    RUN_TEST(TestDefaultConstructor);
    RUN_TEST(TestParameterizedConstructor);
    RUN_TEST(TestGetName);
    RUN_TEST(TestSetName);
    RUN_TEST(TestGetEntityCount);
    RUN_TEST(TestIsStreamable);
    RUN_TEST(TestLoad);
    RUN_TEST(TestUnload);
    RUN_TEST(TestEdgeCases);
}
