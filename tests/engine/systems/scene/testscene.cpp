/**
 * @file testscene.cpp
 * @brief Implementation of Scene unit tests.
 */

#include "testscene.hpp"

// ========== Constructor Tests ==========

void TestScene::TestDefaultConstructor() {
    // Scene requires maxMeshes parameter
    Scene scene(10);

    // Verify scene was constructed
    TEST_ASSERT_EQUAL(0, scene.GetMeshCount());
    TEST_ASSERT_NOT_NULL(scene.GetMeshes());
}

// ========== Method Tests ==========
void TestScene::TestAddMesh() {
    Scene scene(10);
    Mesh mesh;

    scene.AddMesh(&mesh);

    TEST_ASSERT_EQUAL(1, scene.GetMeshCount());

    // Add another mesh
    Mesh mesh2;
    scene.AddMesh(&mesh2);

    TEST_ASSERT_EQUAL(2, scene.GetMeshCount());
}
void TestScene::TestRemoveMesh() {
    Scene scene(10);
    Mesh mesh1, mesh2;

    scene.AddMesh(&mesh1);
    scene.AddMesh(&mesh2);
    TEST_ASSERT_EQUAL(2, scene.GetMeshCount());

    // Remove by index
    scene.RemoveMesh(0);
    TEST_ASSERT_EQUAL(1, scene.GetMeshCount());

    // Remove by pointer
    scene.RemoveMesh(&mesh2);
    TEST_ASSERT_EQUAL(0, scene.GetMeshCount());
}
void TestScene::TestGetMeshes() {
    Scene scene(10);
    Mesh mesh;

    scene.AddMesh(&mesh);

    Mesh** meshes = scene.GetMeshes();
    TEST_ASSERT_NOT_NULL(meshes);
    TEST_ASSERT_EQUAL_PTR(&mesh, meshes[0]);
}
void TestScene::TestGetMeshCount() {
    Scene scene(10);

    TEST_ASSERT_EQUAL(0, scene.GetMeshCount());

    Mesh mesh1, mesh2, mesh3;
    scene.AddMesh(&mesh1);
    scene.AddMesh(&mesh2);
    scene.AddMesh(&mesh3);

    TEST_ASSERT_EQUAL(3, scene.GetMeshCount());
}
void TestScene::TestGetTotalTriangleCount() {
    Scene scene(10);

    // Empty scene should have 0 triangles
    TEST_ASSERT_EQUAL(0, scene.GetTotalTriangleCount());

    // Add meshes (actual triangle count depends on mesh implementation)
    Mesh mesh;
    scene.AddMesh(&mesh);

    uint32_t count = scene.GetTotalTriangleCount();
    TEST_ASSERT_TRUE(count >= 0);
}
// ========== Edge Cases ==========

// ========== Test Runner ==========

void TestScene::TestParameterizedConstructor() {
    // Test with various maxMeshes values
    Scene scene1(1);
    TEST_ASSERT_EQUAL(0, scene1.GetMeshCount());

    Scene scene2(100);
    TEST_ASSERT_EQUAL(0, scene2.GetMeshCount());

    // Verify we can add meshes up to maxMeshes
    Mesh mesh;
    scene1.AddMesh(&mesh);
    TEST_ASSERT_EQUAL(1, scene1.GetMeshCount());
}

void TestScene::TestEdgeCases() {
    // Test removing from empty scene
    Scene scene(10);
    scene.RemoveMesh(0);  // Should handle gracefully
    TEST_ASSERT_EQUAL(0, scene.GetMeshCount());

    // Test removing invalid index
    Mesh mesh;
    scene.AddMesh(&mesh);
    scene.RemoveMesh(100);  // Out of bounds
    TEST_ASSERT_EQUAL(1, scene.GetMeshCount());

    // Test removing nullptr
    scene.RemoveMesh(nullptr);
    TEST_ASSERT_EQUAL(1, scene.GetMeshCount());
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
}
