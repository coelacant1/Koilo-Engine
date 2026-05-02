// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testscene.cpp
 * @brief Implementation of Scene unit tests.
 */

#include "testscene.hpp"
#include <koilo/systems/scene/primitivemesh.hpp>
#include <koilo/systems/scene/scenenode.hpp>
#include <fstream>
#include <string>
#include <cstdio>

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

void TestScene::TestSaveToKScene() {
    Scene scene;
    SceneNode* a = scene.CreateObject("root_a");
    a->SetPosition(Vector3D(1.0f, 2.0f, 3.0f));
    a->SetScale(Vector3D(2.0f, 2.0f, 2.0f));
    SceneNode* b = scene.CreateObject("child b");
    b->SetParent(a);

    const char* path = "/tmp/koilo_savetoks_test.kscene";
    bool ok = scene.SaveToKScene(path);
    TEST_ASSERT_TRUE(ok);

    std::ifstream f(path);
    TEST_ASSERT_TRUE(f.is_open());
    std::string body((std::istreambuf_iterator<char>(f)),
                     std::istreambuf_iterator<char>());

    // Both nodes must be declared, names quoted, and special chars in
    // identifiers sanitized ("child b" -> node_child_b).
    TEST_ASSERT_TRUE(body.find("var node_root_a = SceneNode(\"root_a\")") != std::string::npos);
    TEST_ASSERT_TRUE(body.find("var node_child_b = SceneNode(\"child b\")") != std::string::npos);
    // Position pass writes an XYZ vector matching what we set.
    TEST_ASSERT_TRUE(body.find("node_root_a.SetPosition(Vector3D(1") != std::string::npos);
    // Parent link pass connects b -> a.
    TEST_ASSERT_TRUE(body.find("node_child_b.SetParent(node_root_a)") != std::string::npos);
    std::remove(path);
}

void TestScene::TestBumpHierarchyGeneration() {
    // TODO: Implement test for BumpHierarchyGeneration()
    Scene obj;
    TEST_ASSERT_TRUE(false);  // Not implemented
}

void TestScene::TestHierarchyGeneration() {
    // TODO: Implement test for HierarchyGeneration()
    Scene obj;
    TEST_ASSERT_TRUE(false);  // Not implemented
}

void TestScene::TestPickNode() {
    // TODO: Implement test for PickNode()
    Scene obj;
    TEST_ASSERT_TRUE(false);  // Not implemented
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
    RUN_TEST(TestSaveToKScene);
    RUN_TEST(TestBumpHierarchyGeneration);
    RUN_TEST(TestHierarchyGeneration);
    RUN_TEST(TestPickNode);
}
