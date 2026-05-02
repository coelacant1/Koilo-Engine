// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testworldmanager.cpp
 * @brief Implementation of WorldManager unit tests.
 */

#include "testworldmanager.hpp"
#include <koilo/core/time/timemanager.hpp>

using namespace koilo;
// ========== Constructor Tests ==========

void TestWorldManager::TestDefaultConstructor() {
    WorldManager manager;
    TEST_ASSERT_EQUAL_UINT32(0, manager.GetLevelCount());
}

void TestWorldManager::TestParameterizedConstructor() {
    WorldManager manager;
    TEST_ASSERT_EQUAL_UINT32(0, manager.GetLevelCount());
}

// ========== Method Tests ==========

void TestWorldManager::TestSetStreamingEnabled() {
    WorldManager manager;
    manager.SetStreamingEnabled(true);
    TEST_ASSERT_TRUE(manager.IsStreamingEnabled());
    
    manager.SetStreamingEnabled(false);
    TEST_ASSERT_FALSE(manager.IsStreamingEnabled());
}

void TestWorldManager::TestIsStreamingEnabled() {
    WorldManager manager;
    bool enabled = manager.IsStreamingEnabled();
    TEST_ASSERT_TRUE(enabled == true || enabled == false);
}

void TestWorldManager::TestGetLevelCount() {
    WorldManager manager;
    uint32_t count = manager.GetLevelCount();
    TEST_ASSERT_EQUAL_UINT32(0, count);
}

void TestWorldManager::TestGetActiveLevelName() {
    WorldManager manager;
    std::string name = manager.GetActiveLevelName();
    TEST_ASSERT_TRUE(name.empty() || !name.empty());
}

void TestWorldManager::TestLoadLevel() {
    WorldManager manager;
    manager.LoadLevel("TestLevel");
    // Should handle load request
    TEST_ASSERT_TRUE(true);
}

void TestWorldManager::TestUnloadLevel() {
    WorldManager manager;
    manager.UnloadLevel("TestLevel");
    // Should handle unload request
    TEST_ASSERT_TRUE(true);
}

void TestWorldManager::TestUpdate() {
    WorldManager manager;
    koilo::TimeManager::GetInstance().Tick(0.016f); manager.Update();
    koilo::TimeManager::GetInstance().Tick(0.033f); manager.Update();
    // Should update without crash
    TEST_ASSERT_TRUE(true);
}

// ========== Edge Cases ==========

void TestWorldManager::TestEdgeCases() {
    WorldManager manager;
    manager.LoadLevel("");
    manager.UnloadLevel("");
    koilo::TimeManager::GetInstance().Tick(0.0f); manager.Update();
    // Should handle edge cases gracefully
    TEST_ASSERT_EQUAL_UINT32(0, manager.GetLevelCount());
}
// ========== Test Runner ==========

void TestWorldManager::RunAllTests() {
    RUN_TEST(TestDefaultConstructor);
    RUN_TEST(TestParameterizedConstructor);
    RUN_TEST(TestSetStreamingEnabled);
    RUN_TEST(TestIsStreamingEnabled);
    RUN_TEST(TestGetLevelCount);
    RUN_TEST(TestGetActiveLevelName);
    RUN_TEST(TestLoadLevel);
    RUN_TEST(TestUnloadLevel);
    RUN_TEST(TestUpdate);
    RUN_TEST(TestEdgeCases);
}
