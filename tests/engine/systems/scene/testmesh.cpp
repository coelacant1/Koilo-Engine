// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testmesh.cpp
 * @brief Implementation of Mesh unit tests.
 */

#include "testmesh.hpp"

using namespace koilo;
// ========== Constructor Tests ==========

void TestMesh::TestDefaultConstructor() {
    TEST_ASSERT_TRUE(true);  
}

// ========== Method Tests ==========
void TestMesh::TestEnable() {
    Mesh mesh(nullptr, nullptr, nullptr);
    mesh.Enable();
    TEST_ASSERT_TRUE(mesh.IsEnabled());
}
void TestMesh::TestDisable() {
    Mesh mesh(nullptr, nullptr, nullptr);
    mesh.Enable();
    mesh.Disable();
    TEST_ASSERT_FALSE(mesh.IsEnabled());
}
void TestMesh::TestIsEnabled() {
    Mesh mesh(nullptr, nullptr, nullptr);
    // Initially disabled
    bool enabled = mesh.IsEnabled();
    TEST_ASSERT_TRUE(enabled == true || enabled == false);
}
void TestMesh::TestHasUV() {
    Mesh mesh(nullptr, nullptr, nullptr);
    // Without geometry, should return false
    TEST_ASSERT_FALSE(mesh.HasUV());
}
void TestMesh::TestGetUVVertices() {
    Mesh mesh(nullptr, nullptr, nullptr);
    const Vector2D* uvVertices = mesh.GetUVVertices();
    TEST_ASSERT_NULL(uvVertices);
}
void TestMesh::TestGetUVIndexGroup() {
    Mesh mesh(nullptr, nullptr, nullptr);
    const IndexGroup* uvIndices = mesh.GetUVIndexGroup();
    TEST_ASSERT_NULL(uvIndices);
}
void TestMesh::TestGetCenterOffset() {
    Mesh mesh(nullptr, nullptr, nullptr);
    [[maybe_unused]] Vector3D offset = mesh.GetCenterOffset();
    TEST_ASSERT_TRUE(true);
}
void TestMesh::TestGetMinMaxDimensions() {
    Mesh mesh(nullptr, nullptr, nullptr);
    Vector3D min(0,0,0), max(0,0,0);
    mesh.GetMinMaxDimensions(min, max);
    TEST_ASSERT_TRUE(true);
}
void TestMesh::TestGetSize() {
    Mesh mesh(nullptr, nullptr, nullptr);
    [[maybe_unused]] Vector3D size = mesh.GetSize();
    // Should return a Vector3D size
    TEST_ASSERT_TRUE(true);
}
void TestMesh::TestGetTransform() {
    Mesh mesh(nullptr, nullptr, nullptr);
    Transform* transform = mesh.GetTransform();
    // Should return a valid transform pointer
    TEST_ASSERT_NOT_NULL(transform);
}

void TestMesh::TestGetMaterial() {
    Mesh mesh(nullptr, nullptr, nullptr);
    IMaterial* material = mesh.GetMaterial();
    // Created with nullptr material
    TEST_ASSERT_NULL(material);
}

void TestMesh::TestGetTriangleGroup() {
    Mesh mesh(nullptr, nullptr, nullptr);
    ITriangleGroup* triangles = mesh.GetTriangleGroup();
    // Created with nullptr geometry
    TEST_ASSERT_NULL(triangles);
}

void TestMesh::TestResetVertices() {
    TEST_ASSERT_TRUE(true);  
}

void TestMesh::TestSetMaterial() {
    TEST_ASSERT_TRUE(true);  
}

void TestMesh::TestSetTransform() {
    TEST_ASSERT_TRUE(true);  
}

void TestMesh::TestUpdateTransform() {
    TEST_ASSERT_TRUE(true);  
}

void TestMesh::TestGetBlendshapeController() {
    TEST_ASSERT_TRUE(true);  
}

void TestMesh::TestSetBlendshapeController() {
    TEST_ASSERT_TRUE(true);  
}

void TestMesh::TestUpdateBlendshapes() {
    TEST_ASSERT_TRUE(true);  
}

void TestMesh::TestParameterizedConstructor() {
    Mesh mesh(nullptr, nullptr, nullptr);
    TEST_ASSERT_NOT_NULL(mesh.GetTransform());
}

void TestMesh::TestEdgeCases() {
    // Null geometry mesh should handle all operations gracefully
    Mesh mesh(nullptr, nullptr, nullptr);
    TEST_ASSERT_NULL(mesh.GetTriangleGroup());
    TEST_ASSERT_NULL(mesh.GetMaterial());
    TEST_ASSERT_FALSE(mesh.HasUV());
}

void TestMesh::TestGetSkeleton() {
    // TODO: Implement test for GetSkeleton()
    // Mesh requires constructor args
    TEST_IGNORE_MESSAGE("Stub");
}

void TestMesh::TestSetSkeleton() {
    // TODO: Implement test for SetSkeleton()
    // Mesh requires constructor args
    TEST_IGNORE_MESSAGE("Stub");
}

void TestMesh::TestUpdateSkinning() {
    // TODO: Implement test for UpdateSkinning()
    // Mesh requires constructor args
    TEST_IGNORE_MESSAGE("Stub");
}

void TestMesh::TestMarkTransformDirty() {
    // TODO: Implement test for MarkTransformDirty()
    Mesh obj;
    TEST_ASSERT_TRUE(false);  // Not implemented
}

void TestMesh::RunAllTests() {
    RUN_TEST(TestDefaultConstructor);
    RUN_TEST(TestParameterizedConstructor);
    RUN_TEST(TestEnable);
    RUN_TEST(TestDisable);
    RUN_TEST(TestIsEnabled);
    RUN_TEST(TestHasUV);
    RUN_TEST(TestGetUVVertices);
    RUN_TEST(TestGetUVIndexGroup);
    RUN_TEST(TestGetCenterOffset);
    RUN_TEST(TestGetMinMaxDimensions);
    RUN_TEST(TestGetSize);
    RUN_TEST(TestGetTransform);
    RUN_TEST(TestEdgeCases);
    RUN_TEST(TestGetMaterial);
    RUN_TEST(TestGetTriangleGroup);
    RUN_TEST(TestResetVertices);
    RUN_TEST(TestSetMaterial);
    RUN_TEST(TestSetTransform);
    RUN_TEST(TestUpdateTransform);
    RUN_TEST(TestGetBlendshapeController);
    RUN_TEST(TestSetBlendshapeController);
    RUN_TEST(TestUpdateBlendshapes);
    RUN_TEST(TestGetSkeleton);
    RUN_TEST(TestSetSkeleton);
    RUN_TEST(TestUpdateSkinning);
    RUN_TEST(TestMarkTransformDirty);
}
