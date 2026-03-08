// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testscriptentitymanager.cpp
 * @brief Implementation of ScriptEntityManager unit tests.
 */

#include "testscriptentitymanager.hpp"

using namespace koilo;

// ========== Constructor Tests ==========

void TestScriptEntityManager::TestDefaultConstructor() {
    ScriptEntityManager mgr;
    TEST_ASSERT_EQUAL(0, mgr.GetCount());
}

void TestScriptEntityManager::TestParameterizedConstructor() {
    // Only default constructor; verify it's functional
    ScriptEntityManager mgr;
    Entity e = mgr.Create();
    TEST_ASSERT_TRUE(e.IsValid());
    TEST_ASSERT_EQUAL(1, mgr.GetCount());
}

// ========== Method Tests ==========

void TestScriptEntityManager::TestGetCount() {
    ScriptEntityManager mgr;
    TEST_ASSERT_EQUAL(0, mgr.GetCount());
    mgr.Create();
    TEST_ASSERT_EQUAL(1, mgr.GetCount());
    mgr.Create();
    TEST_ASSERT_EQUAL(2, mgr.GetCount());
}

void TestScriptEntityManager::TestSyncTransforms() {
    ScriptEntityManager mgr;
    Entity e = mgr.Create();
    mgr.SetPosition(e, Vector3D(1.0f, 2.0f, 3.0f));
    // SyncTransforms should not crash with no attached nodes
    mgr.SyncTransforms();
    TEST_ASSERT_TRUE(true);
}

// ========== Edge Cases ==========

void TestScriptEntityManager::TestEdgeCases() {
    ScriptEntityManager mgr;

    // Multiple creates and destroy
    Entity e1 = mgr.Create();
    Entity e2 = mgr.Create();
    Entity e3 = mgr.Create();
    TEST_ASSERT_EQUAL(3, mgr.GetCount());

    mgr.Destroy(e2);
    TEST_ASSERT_FALSE(mgr.IsValid(e2));
    TEST_ASSERT_TRUE(mgr.IsValid(e1));
    TEST_ASSERT_TRUE(mgr.IsValid(e3));

    // Overwrite tag
    mgr.SetTag(e1, "first");
    mgr.SetTag(e1, "updated");
    TEST_ASSERT_EQUAL_STRING("updated", mgr.GetTag(e1).c_str());
}

// ========== Test Runner ==========

void TestScriptEntityManager::RunAllTests() {
    RUN_TEST(TestDefaultConstructor);
    RUN_TEST(TestParameterizedConstructor);

    RUN_TEST(TestGetCount);

    RUN_TEST(TestSyncTransforms);
    RUN_TEST(TestEdgeCases);
}
