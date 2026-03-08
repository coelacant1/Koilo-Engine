// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testscene.cpp
 * @brief Implementation of Scene unit tests.
 */

#include "testscene.hpp"
#include <koilo/systems/scene/primitivemesh.hpp>

using namespace koilo;
// ========== Constructor Tests ==========

void TestScene::TestDefaultConstructor() {
    Scene scene;

    TEST_ASSERT_EQUAL(0, scene.GetMeshCount());
    TEST_ASSERT_NOT_NULL(scene.GetMeshes());
}

// ========== Method Tests ==========
void TestScene::TestAddMesh() {
    Scene scene;
    PrimitiveMesh prim1;
    prim1.CreateQuad(1.0f, 1.0f);
    scene.AddMesh(prim1.GetMesh());
    TEST_ASSERT_EQUAL(1, scene.GetMeshCount());

    PrimitiveMesh prim2;
    prim2.CreateQuad(1.0f, 1.0f);
    scene.AddMesh(prim2.GetMesh());
    TEST_ASSERT_EQUAL(2, scene.GetMeshCount());
}

void TestScene::TestRemoveMesh() {
    Scene scene;
    PrimitiveMesh prim1, prim2;
    prim1.CreateQuad(1.0f, 1.0f);
    prim2.CreateQuad(1.0f, 1.0f);

    scene.AddMesh(prim1.GetMesh());
    scene.AddMesh(prim2.GetMesh());
    TEST_ASSERT_EQUAL(2, scene.GetMeshCount());

    // Remove by index
    scene.RemoveMesh(0U);
    TEST_ASSERT_EQUAL(1, scene.GetMeshCount());

    // Remove by pointer
    scene.RemoveMesh(prim2.GetMesh());
    TEST_ASSERT_EQUAL(0, scene.GetMeshCount());
}

void TestScene::TestGetMeshes() {
    Scene scene;
    PrimitiveMesh prim;
    prim.CreateQuad(1.0f, 1.0f);
    scene.AddMesh(prim.GetMesh());

    Mesh** meshes = scene.GetMeshes();
    TEST_ASSERT_NOT_NULL(meshes);
    TEST_ASSERT_TRUE(meshes[0] == prim.GetMesh());
}

void TestScene::TestGetMeshCount() {
    Scene scene;
    TEST_ASSERT_EQUAL(0, scene.GetMeshCount());

    PrimitiveMesh prim1, prim2, prim3;
    prim1.CreateQuad(1.0f, 1.0f);
    prim2.CreateQuad(1.0f, 1.0f);
    prim3.CreateQuad(1.0f, 1.0f);
    scene.AddMesh(prim1.GetMesh());
    scene.AddMesh(prim2.GetMesh());
    scene.AddMesh(prim3.GetMesh());
    TEST_ASSERT_EQUAL(3, scene.GetMeshCount());
}

void TestScene::TestGetTotalTriangleCount() {
    Scene scene;
    TEST_ASSERT_EQUAL(0, scene.GetTotalTriangleCount());

    PrimitiveMesh prim;
    prim.CreateQuad(1.0f, 1.0f);
    scene.AddMesh(prim.GetMesh());

    // A quad has 2 triangles
    TEST_ASSERT_EQUAL(2, scene.GetTotalTriangleCount());
}

// ========== Edge Cases ==========

// ========== Test Runner ==========

void TestScene::TestParameterizedConstructor() {
    Scene scene1;
    TEST_ASSERT_EQUAL(0, scene1.GetMeshCount());

    Scene scene2;
    TEST_ASSERT_EQUAL(0, scene2.GetMeshCount());

    // Verify we can add a mesh
    PrimitiveMesh prim;
    prim.CreateQuad(1.0f, 1.0f);
    scene1.AddMesh(prim.GetMesh());
    TEST_ASSERT_EQUAL(1, scene1.GetMeshCount());
}

void TestScene::TestEdgeCases() {
    // Test removing from empty scene
    Scene scene;
    scene.RemoveMesh(0U);
    TEST_ASSERT_EQUAL(0, scene.GetMeshCount());

    // Test removing invalid index
    PrimitiveMesh prim;
    prim.CreateQuad(1.0f, 1.0f);
    scene.AddMesh(prim.GetMesh());
    scene.RemoveMesh(100U);
    TEST_ASSERT_EQUAL(1, scene.GetMeshCount());

    // Test removing nullptr
    scene.RemoveMesh(nullptr);
    TEST_ASSERT_EQUAL(1, scene.GetMeshCount());
}

void TestScene::TestCreateObject() {
    Scene scene;
    SceneNode* node = scene.CreateObject("player");
    TEST_ASSERT_NOT_NULL(node);
    TEST_ASSERT_EQUAL_STRING("player", node->GetName().c_str());

    // Creating with same name returns existing node
    SceneNode* same = scene.CreateObject("player");
    TEST_ASSERT_TRUE(same == node);
}

void TestScene::TestFind() {
    Scene scene;
    scene.CreateObject("camera");
    scene.CreateObject("light");

    SceneNode* found = scene.Find("camera");
    TEST_ASSERT_NOT_NULL(found);
    TEST_ASSERT_EQUAL_STRING("camera", found->GetName().c_str());

    // Not found returns nullptr
    SceneNode* missing = scene.Find("nonexistent");
    TEST_ASSERT_TRUE(missing == nullptr);
}

void TestScene::TestGetNodeCount() {
    Scene scene;
    TEST_ASSERT_EQUAL(0, scene.GetNodeCount());

    scene.CreateObject("a");
    TEST_ASSERT_EQUAL(1, scene.GetNodeCount());

    scene.CreateObject("b");
    TEST_ASSERT_EQUAL(2, scene.GetNodeCount());

    // Duplicate name doesn't increase count
    scene.CreateObject("a");
    TEST_ASSERT_EQUAL(2, scene.GetNodeCount());
}

void TestScene::TestGetMesh() {
    Scene scene;
    // Empty scene: out-of-bounds index returns nullptr
    TEST_ASSERT_TRUE(scene.GetMesh(0) == nullptr);

    PrimitiveMesh prim;
    prim.CreateQuad(1.0f, 1.0f);
    scene.AddMesh(prim.GetMesh());

    TEST_ASSERT_TRUE(scene.GetMesh(0) == prim.GetMesh());
    TEST_ASSERT_TRUE(scene.GetMesh(1) == nullptr);
}

void TestScene::RunAllTests() {
    RUN_TEST(TestDefaultConstructor);
    RUN_TEST(TestParameterizedConstructor);
    RUN_TEST(TestAddMesh);
    RUN_TEST(TestRemoveMesh);
    RUN_TEST(TestGetMeshes);
    RUN_TEST(TestGetMeshCount);
    RUN_TEST(TestGetTotalTriangleCount);
    RUN_TEST(TestEdgeCases);
    RUN_TEST(TestCreateObject);
    RUN_TEST(TestFind);
    RUN_TEST(TestGetNodeCount);
    RUN_TEST(TestGetMesh);
}
