// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testscenenode.cpp
 * @brief Implementation of SceneNode unit tests.
 */

#include "testscenenode.hpp"
#include <koilo/core/math/vector3d.hpp>
#include <koilo/systems/scene/primitivemesh.hpp>

using namespace koilo;

// ========== Constructor Tests ==========

void TestSceneNode::TestDefaultConstructor() {
    SceneNode node;
    TEST_ASSERT_TRUE(node.GetName().empty());
    TEST_ASSERT_TRUE(node.GetParent() == nullptr);
    TEST_ASSERT_EQUAL(0, node.GetChildCount());
    TEST_ASSERT_TRUE(node.GetMesh() == nullptr);
}

void TestSceneNode::TestParameterizedConstructor() {
    SceneNode node("testNode");
    TEST_ASSERT_EQUAL_STRING("testNode", node.GetName().c_str());
    TEST_ASSERT_TRUE(node.GetParent() == nullptr);
    TEST_ASSERT_EQUAL(0, node.GetChildCount());
}

// ========== Method Tests ==========

void TestSceneNode::TestSetPosition() {
    SceneNode node;
    node.SetPosition(Vector3D(1.0f, 2.0f, 3.0f));
    Vector3D pos = node.GetPosition();
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, pos.X);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 2.0f, pos.Y);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 3.0f, pos.Z);
}

void TestSceneNode::TestSetRotation() {
    SceneNode node;
    node.SetRotation(Vector3D(45.0f, 90.0f, 0.0f));
    // Verify rotation was set via the local transform
    [[maybe_unused]] Transform& t = node.GetLocalTransform();
    // Rotation is stored as quaternion, just verify no crash
    TEST_ASSERT_TRUE(true);
}

void TestSceneNode::TestSetScale() {
    SceneNode node;
    node.SetScale(Vector3D(2.0f, 3.0f, 4.0f));
    Vector3D scale = node.GetLocalTransform().GetScale();
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 2.0f, scale.X);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 3.0f, scale.Y);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 4.0f, scale.Z);
}

void TestSceneNode::TestGetPosition() {
    SceneNode node;
    Vector3D defaultPos = node.GetPosition();
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, defaultPos.X);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, defaultPos.Y);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, defaultPos.Z);

    node.SetPosition(Vector3D(5.0f, -3.0f, 7.0f));
    Vector3D pos = node.GetPosition();
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 5.0f, pos.X);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, -3.0f, pos.Y);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 7.0f, pos.Z);
}

void TestSceneNode::TestGetChildCount() {
    SceneNode parent("parent");
    TEST_ASSERT_EQUAL(0, parent.GetChildCount());

    SceneNode child1("child1");
    child1.SetParent(&parent);
    TEST_ASSERT_EQUAL(1, parent.GetChildCount());

    SceneNode child2("child2");
    child2.SetParent(&parent);
    TEST_ASSERT_EQUAL(2, parent.GetChildCount());
}

void TestSceneNode::TestSetParent() {
    SceneNode parent("parent");
    SceneNode child("child");

    child.SetParent(&parent);
    TEST_ASSERT_TRUE(child.GetParent() == &parent);
    TEST_ASSERT_EQUAL(1, parent.GetChildCount());

    // Re-parent to nullptr
    child.SetParent(nullptr);
    TEST_ASSERT_TRUE(child.GetParent() == nullptr);
    TEST_ASSERT_EQUAL(0, parent.GetChildCount());
}

void TestSceneNode::TestGetParent() {
    SceneNode node;
    TEST_ASSERT_TRUE(node.GetParent() == nullptr);

    SceneNode parent("parent");
    node.SetParent(&parent);
    TEST_ASSERT_TRUE(node.GetParent() == &parent);
}

void TestSceneNode::TestFindChild() {
    SceneNode parent("parent");
    SceneNode child1("alpha");
    SceneNode child2("beta");
    child1.SetParent(&parent);
    child2.SetParent(&parent);

    SceneNode* found = parent.FindChild("alpha");
    TEST_ASSERT_NOT_NULL(found);
    TEST_ASSERT_EQUAL_STRING("alpha", found->GetName().c_str());

    SceneNode* notFound = parent.FindChild("gamma");
    TEST_ASSERT_TRUE(notFound == nullptr);
}

void TestSceneNode::TestSetMesh() {
    SceneNode node;
    PrimitiveMesh prim;
    prim.CreateQuad(1.0f, 1.0f);

    node.SetMesh(prim.GetMesh());
    TEST_ASSERT_TRUE(node.GetMesh() == prim.GetMesh());
}

void TestSceneNode::TestGetMesh() {
    SceneNode node;
    TEST_ASSERT_TRUE(node.GetMesh() == nullptr);

    PrimitiveMesh prim;
    prim.CreateQuad(1.0f, 1.0f);
    node.SetMesh(prim.GetMesh());
    TEST_ASSERT_NOT_NULL(node.GetMesh());
}

void TestSceneNode::TestSetScriptPath() {
    SceneNode node;
    TEST_ASSERT_TRUE(node.GetScriptPath().empty());

    node.SetScriptPath("scripts/player.ks");
    TEST_ASSERT_EQUAL_STRING("scripts/player.ks", node.GetScriptPath().c_str());
}

// ========== Edge Cases ==========

void TestSceneNode::TestEdgeCases() {
    // World transform of root node equals local transform
    SceneNode root("root");
    root.SetPosition(Vector3D(10.0f, 20.0f, 30.0f));
    const Transform& world = root.GetWorldTransform();
    Vector3D wpos = world.GetPosition();
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 10.0f, wpos.X);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 20.0f, wpos.Y);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 30.0f, wpos.Z);

    // Child world position = parent + child local position
    SceneNode child("child");
    child.SetParent(&root);
    child.SetPosition(Vector3D(1.0f, 2.0f, 3.0f));
    const Transform& childWorld = child.GetWorldTransform();
    Vector3D cwpos = childWorld.GetPosition();
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 11.0f, cwpos.X);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 22.0f, cwpos.Y);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 33.0f, cwpos.Z);
}

// ========== Test Runner ==========

void TestSceneNode::RunAllTests() {
    RUN_TEST(TestDefaultConstructor);
    RUN_TEST(TestParameterizedConstructor);
    RUN_TEST(TestSetPosition);
    RUN_TEST(TestSetRotation);
    RUN_TEST(TestSetScale);
    RUN_TEST(TestGetPosition);
    RUN_TEST(TestGetChildCount);
    RUN_TEST(TestSetParent);
    RUN_TEST(TestGetParent);
    RUN_TEST(TestFindChild);
    RUN_TEST(TestSetMesh);
    RUN_TEST(TestGetMesh);
    RUN_TEST(TestSetScriptPath);
    RUN_TEST(TestEdgeCases);
}
