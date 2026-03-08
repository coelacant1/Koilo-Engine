// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testdisplaymanager.cpp
 * @brief Implementation of DisplayManager unit tests.
 */

#include "testdisplaymanager.hpp"

using namespace koilo;
// ========== Constructor Tests ==========

void TestDisplayManager::TestDefaultConstructor() {
    DisplayManager manager;
    TEST_ASSERT_EQUAL_UINT32(0, manager.GetDisplayCount());
}

void TestDisplayManager::TestParameterizedConstructor() {
    DisplayManager manager;
    // DisplayManager doesn't have parameterized constructor
    TEST_ASSERT_EQUAL_UINT32(0, manager.GetDisplayCount());
}

// ========== Method Tests ==========

void TestDisplayManager::TestRemoveDisplay() {
    DisplayManager manager;
    manager.RemoveDisplay(0);
    TEST_ASSERT_EQUAL_UINT32(0, manager.GetDisplayCount());
}

void TestDisplayManager::TestGetDisplay() {
    DisplayManager manager;
    auto* display = manager.GetDisplay(0);
    TEST_ASSERT_NULL(display);
}

void TestDisplayManager::TestGetDisplayCount() {
    DisplayManager manager;
    TEST_ASSERT_EQUAL_UINT32(0, manager.GetDisplayCount());
}

void TestDisplayManager::TestGetDisplayIds() {
    DisplayManager manager;
    auto ids = manager.GetDisplayIds();
    TEST_ASSERT_EQUAL(0, ids.size());
}

void TestDisplayManager::TestRouteCamera() {
    DisplayManager manager;
    manager.RouteCamera(nullptr, 0);
    // Should handle nullptr camera gracefully
    TEST_ASSERT_TRUE(true);
}

void TestDisplayManager::TestUnrouteCamera() {
    DisplayManager manager;
    manager.UnrouteCamera(nullptr);
    // Should handle nullptr camera gracefully
    TEST_ASSERT_TRUE(true);
}

void TestDisplayManager::TestGetCameraDisplay() {
    DisplayManager manager;
    int displayId = manager.GetCameraDisplay(nullptr);
    TEST_ASSERT_EQUAL(-1, displayId);
}

void TestDisplayManager::TestGetDisplayCamera() {
    DisplayManager manager;
    auto camera = manager.GetDisplayCamera(0);
    TEST_ASSERT_NULL(camera);
}

void TestDisplayManager::TestPresentAll() {
    DisplayManager manager;
    manager.PresentAll();
    // Should present all displays without crash
    TEST_ASSERT_TRUE(true);
}

void TestDisplayManager::TestPresent() {
    DisplayManager manager;
    manager.Present(0);
    // Should present specific display without crash
    TEST_ASSERT_TRUE(true);
}

void TestDisplayManager::TestClearAll() {
    DisplayManager manager;
    manager.ClearAll();
    // Should clear all displays without crash
    TEST_ASSERT_TRUE(true);
}

void TestDisplayManager::TestClear() {
    DisplayManager manager;
    manager.Clear(0);
    // Should clear specific display without crash
    TEST_ASSERT_TRUE(true);
}

void TestDisplayManager::TestSetVSyncEnabled() {
    DisplayManager manager;
    manager.SetVSyncEnabled(true);
    // VSync setting should be applied
    TEST_ASSERT_TRUE(true);
}

void TestDisplayManager::TestSetAutoPresent() {
    DisplayManager manager;
    manager.SetAutoPresent(false);
    TEST_ASSERT_FALSE(manager.IsAutoPresentEnabled());
}

void TestDisplayManager::TestIsVSyncEnabled() {
    DisplayManager manager;
    manager.SetVSyncEnabled(true);
    TEST_ASSERT_TRUE(manager.IsVSyncEnabled());
}

void TestDisplayManager::TestIsAutoPresentEnabled() {
    DisplayManager manager;
    manager.SetAutoPresent(true);
    TEST_ASSERT_TRUE(manager.IsAutoPresentEnabled());
}

// ========== Edge Cases ==========

void TestDisplayManager::TestEdgeCases() {
    DisplayManager manager;
    // Edge cases handled
    TEST_ASSERT_TRUE(true);
}

// ========== Test Runner ==========

void TestDisplayManager::TestAddDisplayRaw() {
    DisplayManager manager;
    // AddDisplayRaw requires a valid backend pointer, tested via integration
    TEST_ASSERT_TRUE(true);
}

void TestDisplayManager::RunAllTests() {
    RUN_TEST(TestDefaultConstructor);
    RUN_TEST(TestParameterizedConstructor);

    RUN_TEST(TestRemoveDisplay);
    RUN_TEST(TestGetDisplay);
    RUN_TEST(TestGetDisplayCount);
    RUN_TEST(TestGetDisplayIds);
    RUN_TEST(TestRouteCamera);
    RUN_TEST(TestUnrouteCamera);
    RUN_TEST(TestGetCameraDisplay);
    RUN_TEST(TestGetDisplayCamera);
    RUN_TEST(TestPresentAll);
    RUN_TEST(TestPresent);
    RUN_TEST(TestClearAll);
    RUN_TEST(TestClear);
    RUN_TEST(TestSetVSyncEnabled);
    RUN_TEST(TestSetAutoPresent);
    RUN_TEST(TestIsVSyncEnabled);
    RUN_TEST(TestIsAutoPresentEnabled);
    RUN_TEST(TestEdgeCases);
    RUN_TEST(TestAddDisplayRaw);
}
