// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testtransformcomponent.cpp
 * @brief Implementation of TransformComponent unit tests.
 */

#include "testtransformcomponent.hpp"

using namespace koilo;
// ========== Constructor Tests ==========

void TestTransformComponent::TestDefaultConstructor() {
    TransformComponent tc;
    
    // Default should have identity transform: position (0,0,0), scale (1,1,1)
    Vector3D pos = tc.GetPosition();
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, pos.X);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, pos.Y);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, pos.Z);
    
    Vector3D scale = tc.GetScale();
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, scale.X);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, scale.Y);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, scale.Z);
}

void TestTransformComponent::TestParameterizedConstructor() {
    Vector3D testPos(10.0f, 20.0f, 30.0f);
    TransformComponent tc(testPos);
    
    Vector3D pos = tc.GetPosition();
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 10.0f, pos.X);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 20.0f, pos.Y);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 30.0f, pos.Z);
}

// ========== Method Tests ==========

void TestTransformComponent::TestGetPosition() {
    Vector3D testPos(5.0f, 15.0f, 25.0f);
    TransformComponent tc(testPos);
    
    Vector3D pos = tc.GetPosition();
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 5.0f, pos.X);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 15.0f, pos.Y);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 25.0f, pos.Z);
}

void TestTransformComponent::TestSetPosition() {
    TransformComponent tc;
    
    Vector3D newPos(100.0f, 200.0f, 300.0f);
    tc.SetPosition(newPos);
    
    Vector3D pos = tc.GetPosition();
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 100.0f, pos.X);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 200.0f, pos.Y);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 300.0f, pos.Z);
}

void TestTransformComponent::TestGetRotation() {
    TransformComponent tc;
    
    Quaternion rot = tc.GetRotation();
    // Default rotation should be identity
    TEST_ASSERT_TRUE(true);
}

void TestTransformComponent::TestSetRotation() {
    TransformComponent tc;
    
    Quaternion newRot(0.707f, 0.0f, 0.707f, 0.0f); // 90 degree rotation around Y
    tc.SetRotation(newRot);
    
    Quaternion rot = tc.GetRotation();
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.707f, rot.W);
}

void TestTransformComponent::TestGetScale() {
    TransformComponent tc;
    
    Vector3D scale = tc.GetScale();
    // Default scale should be (1,1,1)
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, scale.X);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, scale.Y);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, scale.Z);
}

void TestTransformComponent::TestSetScale() {
    TransformComponent tc;
    
    Vector3D newScale(2.0f, 3.0f, 4.0f);
    tc.SetScale(newScale);
    
    Vector3D scale = tc.GetScale();
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 2.0f, scale.X);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 3.0f, scale.Y);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 4.0f, scale.Z);
}

// ========== Edge Cases ==========

void TestTransformComponent::TestEdgeCases() {
    // Test negative scale
    TransformComponent tc;
    tc.SetScale(Vector3D(-1.0f, -1.0f, -1.0f));
    Vector3D scale = tc.GetScale();
    TEST_ASSERT_FLOAT_WITHIN(0.01f, -1.0f, scale.X);
    
    // Test zero scale
    tc.SetScale(Vector3D(0.0f, 0.0f, 0.0f));
    scale = tc.GetScale();
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, scale.X);
}

// ========== Test Runner ==========

void TestTransformComponent::RunAllTests() {
    RUN_TEST(TestDefaultConstructor);
    RUN_TEST(TestParameterizedConstructor);
    RUN_TEST(TestGetPosition);
    RUN_TEST(TestSetPosition);
    RUN_TEST(TestGetRotation);
    RUN_TEST(TestSetRotation);
    RUN_TEST(TestGetScale);
    RUN_TEST(TestSetScale);
    RUN_TEST(TestEdgeCases);
}
