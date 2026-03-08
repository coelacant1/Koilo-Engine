// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testmorphablemesh.cpp
 * @brief Implementation of MorphableMesh unit tests.
 */

#include "testmorphablemesh.hpp"

using namespace koilo;

// ========== Constructor Tests ==========

void TestMorphableMesh::TestDefaultConstructor() {
    MorphableMesh obj;
    TEST_ASSERT_FALSE(obj.IsLoaded());
    TEST_ASSERT_TRUE(obj.GetMesh() == nullptr);
}

void TestMorphableMesh::TestParameterizedConstructor() {
    // MorphableMesh only has a default constructor
    MorphableMesh obj;
    TEST_ASSERT_FALSE(obj.IsLoaded());
}

// ========== Method Tests ==========

void TestMorphableMesh::TestLoad() {
    MorphableMesh obj;
    // Loading a nonexistent file should fail
    bool result = obj.Load("nonexistent_file.kmesh");
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_FALSE(obj.IsLoaded());
}

void TestMorphableMesh::TestIsLoaded() {
    MorphableMesh obj;
    TEST_ASSERT_FALSE(obj.IsLoaded());
}

void TestMorphableMesh::TestSetMorphWeight() {
    MorphableMesh obj;
    // Setting weight on unloaded mesh should return false
    bool result = obj.SetMorphWeight("Anger", 1.0f);
    TEST_ASSERT_FALSE(result);
}

void TestMorphableMesh::TestGetMorphWeight() {
    MorphableMesh obj;
    // Getting weight on unloaded mesh returns 0
    float weight = obj.GetMorphWeight("Anger");
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, weight);
}

void TestMorphableMesh::TestGetMorphCount() {
    MorphableMesh obj;
    TEST_ASSERT_EQUAL(0, obj.GetMorphCount());
}

void TestMorphableMesh::TestGetVertexCount() {
    MorphableMesh obj;
    TEST_ASSERT_EQUAL(0, obj.GetVertexCount());
}

void TestMorphableMesh::TestUpdate() {
    MorphableMesh obj;
    // Update on unloaded mesh should not crash
    obj.Update();
    TEST_ASSERT_FALSE(obj.IsLoaded());
}

void TestMorphableMesh::TestResetMorphs() {
    MorphableMesh obj;
    // ResetMorphs on unloaded mesh should not crash
    obj.ResetMorphs();
    TEST_ASSERT_FALSE(obj.IsLoaded());
}

void TestMorphableMesh::TestGetMesh() {
    MorphableMesh obj;
    TEST_ASSERT_TRUE(obj.GetMesh() == nullptr);
}

void TestMorphableMesh::TestGetError() {
    MorphableMesh obj;
    const char* err = obj.GetError();
    TEST_ASSERT_NOT_NULL(err);
}

// ========== Edge Cases ==========

void TestMorphableMesh::TestEdgeCases() {
    MorphableMesh obj;
    // Multiple operations on unloaded mesh should be safe
    obj.Update();
    obj.ResetMorphs();
    TEST_ASSERT_EQUAL(0, obj.GetMorphCount());
    TEST_ASSERT_EQUAL(0, obj.GetVertexCount());
    TEST_ASSERT_TRUE(obj.GetMesh() == nullptr);
}

// ========== Test Runner ==========

void TestMorphableMesh::RunAllTests() {
    RUN_TEST(TestDefaultConstructor);
    RUN_TEST(TestParameterizedConstructor);
    RUN_TEST(TestLoad);
    RUN_TEST(TestIsLoaded);
    RUN_TEST(TestSetMorphWeight);
    RUN_TEST(TestGetMorphWeight);
    RUN_TEST(TestGetMorphCount);
    RUN_TEST(TestGetVertexCount);
    RUN_TEST(TestUpdate);
    RUN_TEST(TestResetMorphs);
    RUN_TEST(TestGetMesh);
    RUN_TEST(TestGetError);
    RUN_TEST(TestEdgeCases);
}
