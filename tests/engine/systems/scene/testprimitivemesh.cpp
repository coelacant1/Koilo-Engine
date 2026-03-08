// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testprimitivemesh.cpp
 * @brief Implementation of PrimitiveMesh unit tests.
 */

#include "testprimitivemesh.hpp"

using namespace koilo;

// ========== Constructor Tests ==========

void TestPrimitiveMesh::TestDefaultConstructor() {
    PrimitiveMesh obj;
    TEST_ASSERT_TRUE(obj.GetMesh() == nullptr);
    TEST_ASSERT_FALSE(obj.HasUV());
}

void TestPrimitiveMesh::TestParameterizedConstructor() {
    // PrimitiveMesh only has a default constructor; verify it works
    PrimitiveMesh obj;
    TEST_ASSERT_TRUE(obj.GetMesh() == nullptr);
}

// ========== Method Tests ==========

void TestPrimitiveMesh::TestCreateQuad() {
    PrimitiveMesh obj;
    obj.CreateQuad(2.0f, 3.0f);
    TEST_ASSERT_NOT_NULL(obj.GetMesh());
    TEST_ASSERT_FALSE(obj.HasUV());
}

void TestPrimitiveMesh::TestCreateTexturedQuad() {
    PrimitiveMesh obj;
    obj.CreateTexturedQuad(4.0f, 5.0f);
    TEST_ASSERT_NOT_NULL(obj.GetMesh());
    TEST_ASSERT_TRUE(obj.HasUV());
}

void TestPrimitiveMesh::TestGetMesh() {
    PrimitiveMesh obj;
    TEST_ASSERT_TRUE(obj.GetMesh() == nullptr);

    obj.CreateQuad(1.0f, 1.0f);
    Mesh* mesh = obj.GetMesh();
    TEST_ASSERT_NOT_NULL(mesh);
    TEST_ASSERT_TRUE(mesh->IsEnabled());
}

void TestPrimitiveMesh::TestHasUV() {
    PrimitiveMesh obj;
    TEST_ASSERT_FALSE(obj.HasUV());

    obj.CreateQuad(1.0f, 1.0f);
    TEST_ASSERT_FALSE(obj.HasUV());

    obj.CreateTexturedQuad(1.0f, 1.0f);
    TEST_ASSERT_TRUE(obj.HasUV());
}

// ========== Edge Cases ==========

void TestPrimitiveMesh::TestEdgeCases() {
    // Calling CreateQuad twice replaces the previous mesh
    PrimitiveMesh obj;
    obj.CreateQuad(1.0f, 1.0f);
    Mesh* first = obj.GetMesh();
    TEST_ASSERT_NOT_NULL(first);

    obj.CreateTexturedQuad(2.0f, 2.0f);
    Mesh* second = obj.GetMesh();
    TEST_ASSERT_NOT_NULL(second);
    TEST_ASSERT_TRUE(obj.HasUV());
}

// ========== Test Runner ==========

void TestPrimitiveMesh::RunAllTests() {
    RUN_TEST(TestDefaultConstructor);
    RUN_TEST(TestParameterizedConstructor);
    RUN_TEST(TestCreateQuad);
    RUN_TEST(TestCreateTexturedQuad);
    RUN_TEST(TestGetMesh);
    RUN_TEST(TestHasUV);
    RUN_TEST(TestEdgeCases);
}
