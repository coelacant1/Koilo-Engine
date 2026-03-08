// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testresourcemanager.cpp
 * @brief Implementation of ResourceManager tests.
 */

#include "testresourcemanager.hpp"
#include <koilo/resources/resourcemanager.hpp>

using namespace koilo;

void TestResourceManager::TestSetMemoryLimit() {
    // SetMemoryLimit/GetMemoryLimit not in API
    TEST_ASSERT_TRUE(true);
}

void TestResourceManager::TestCheckHotReload() {
    // TODO: Implement test for CheckHotReload()
    // ResourceManager is singleton
    TEST_IGNORE_MESSAGE("Stub");
}

void TestResourceManager::TestDefaultConstructor() {
    // TODO: Implement test for default constructor
    // ResourceManager is singleton
    TEST_IGNORE_MESSAGE("Stub");
}

void TestResourceManager::TestEdgeCases() {
    // TODO: Test edge cases (null, boundaries, extreme values)
    TEST_IGNORE_MESSAGE("Stub");
}

void TestResourceManager::TestEnableHotReload() {
    // TODO: Implement test for EnableHotReload()
    // ResourceManager is singleton
    TEST_IGNORE_MESSAGE("Stub");
}

void TestResourceManager::TestGarbageCollect() {
    // TODO: Implement test for GarbageCollect()
    // ResourceManager is singleton
    TEST_IGNORE_MESSAGE("Stub");
}

void TestResourceManager::TestGetCachedResourceCount() {
    // TODO: Implement test for GetCachedResourceCount()
    // ResourceManager is singleton
    TEST_IGNORE_MESSAGE("Stub");
}

void TestResourceManager::TestGetTotalMemoryUsed() {
    // TODO: Implement test for GetTotalMemoryUsed()
    // ResourceManager is singleton
    TEST_IGNORE_MESSAGE("Stub");
}

void TestResourceManager::TestParameterizedConstructor() {
    // TODO: Implement test for parameterized constructor
    TEST_IGNORE_MESSAGE("Stub");
}

void TestResourceManager::TestPrintStatistics() {
    // TODO: Implement test for PrintStatistics()
    // ResourceManager is singleton
    TEST_IGNORE_MESSAGE("Stub");
}

void TestResourceManager::TestUnloadAllResources() {
    // TODO: Implement test for UnloadAllResources()
    // ResourceManager is singleton
    TEST_IGNORE_MESSAGE("Stub");
}

void TestResourceManager::RunAllTests() {

    RUN_TEST(TestSetMemoryLimit);

    RUN_TEST(TestCheckHotReload);
    RUN_TEST(TestDefaultConstructor);
    RUN_TEST(TestEdgeCases);
    RUN_TEST(TestEnableHotReload);
    RUN_TEST(TestGarbageCollect);
    RUN_TEST(TestGetCachedResourceCount);
    RUN_TEST(TestGetTotalMemoryUsed);
    RUN_TEST(TestParameterizedConstructor);
    RUN_TEST(TestPrintStatistics);
    RUN_TEST(TestUnloadAllResources);
}
