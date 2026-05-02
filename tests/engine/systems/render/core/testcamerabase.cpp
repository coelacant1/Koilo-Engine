// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testcamerabase.cpp
 * @brief Implementation of CameraBase unit tests using concrete Camera class.
 *
 * @date 24/10/2025
 * @author Coela
 */

#include "testcamerabase.hpp"
#include <koilo/systems/scene/camera/camera.hpp>
#include <koilo/systems/render/core/pixelgroup.hpp>

using namespace koilo;

void TestCameraBase::TestDefaultConstruction() {
    // Use Camera as concrete implementation of CameraBase
    // Create pixel locations
    Vector2D pixelLocs[100];
    for (int i = 0; i < 100; i++) {
        pixelLocs[i] = Vector2D(i % 10, i / 10);
    }
    
    PixelGroup pixelGroup(pixelLocs, 100);
    Transform transform;
    Camera camera(&transform, &pixelGroup);
    
    // Camera should be constructed
    TEST_ASSERT_NOT_NULL(camera.GetPixelGroup());
}

void TestCameraBase::TestTransformAccess() {
    Vector2D pixelLocs[100];
    for (int i = 0; i < 100; i++) {
        pixelLocs[i] = Vector2D(i % 10, i / 10);
    }
    
    PixelGroup pixelGroup(pixelLocs, 100);
    Transform transform;
    Camera camera(&transform, &pixelGroup);
    
    // Access transform
    Transform* t = camera.GetTransform();
    TEST_ASSERT_NOT_NULL(t);
    
    // Modify transform
    t->SetPosition(Vector3D(5, 10, 15));
    Vector3D pos = t->GetPosition();
    
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 5.0f, pos.X);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 10.0f, pos.Y);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 15.0f, pos.Z);
}

void TestCameraBase::TestCameraLayoutAccess() {
    Vector2D pixelLocs[100];
    for (int i = 0; i < 100; i++) {
        pixelLocs[i] = Vector2D(i % 10, i / 10);
    }
    
    PixelGroup pixelGroup(pixelLocs, 100);
    Transform transform;
    Camera camera(&transform, &pixelGroup);
    
    CameraLayout* layout = camera.GetCameraLayout();
    TEST_ASSERT_NOT_NULL(layout);
}

void TestCameraBase::TestLookOffsetManagement() {
    Vector2D pixelLocs[100];
    for (int i = 0; i < 100; i++) {
        pixelLocs[i] = Vector2D(i % 10, i / 10);
    }
    
    PixelGroup pixelGroup(pixelLocs, 100);
    Transform transform;
    Camera camera(&transform, &pixelGroup);
    
    // Set look offset
    Quaternion offset(0, 0.3826834f, 0, 0.9238795f); // ~45 deg around Y
    camera.SetLookOffset(offset);
    
    Quaternion retrieved = camera.GetLookOffset();
    
    // Verify quaternion values (approximate due to conversions)
    TEST_ASSERT_FLOAT_WITHIN(0.01f, offset.W, retrieved.W);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, offset.X, retrieved.X);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, offset.Y, retrieved.Y);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, offset.Z, retrieved.Z);
}

void TestCameraBase::Test2DModeFlag() {
    Vector2D pixelLocs[100];
    for (int i = 0; i < 100; i++) {
        pixelLocs[i] = Vector2D(i % 10, i / 10);
    }
    
    PixelGroup pixelGroup(pixelLocs, 100);
    Transform transform;
    Camera camera(&transform, &pixelGroup);
    
    // Test 2D mode
    camera.Set2D(true);
    TEST_ASSERT_TRUE(camera.Is2D());
    
    camera.Set2D(false);
    TEST_ASSERT_FALSE(camera.Is2D());
}

void TestCameraBase::TestVirtualInterfaceAccess() {
    Vector2D pixelLocs[100];
    for (int i = 0; i < 100; i++) {
        pixelLocs[i] = Vector2D(i % 10, i / 10);
    }
    
    PixelGroup pixelGroup(pixelLocs, 100);
    Transform transform;
    Camera camera(&transform, &pixelGroup);
    
    // Test virtual interface methods
    Vector2D minCoord = camera.GetCameraMinCoordinate();
    Vector2D maxCoord = camera.GetCameraMaxCoordinate();
    [[maybe_unused]] Vector2D centerCoord = camera.GetCameraCenterCoordinate();
    
    // Min should be less than max
    TEST_ASSERT_TRUE(minCoord.X <= maxCoord.X);
    TEST_ASSERT_TRUE(minCoord.Y <= maxCoord.Y);
    
    // Test 3D transforms
    Vector3D minTransform = camera.GetCameraTransformMin();
    Vector3D maxTransform = camera.GetCameraTransformMax();
    [[maybe_unused]] Vector3D centerTransform = camera.GetCameraTransformCenter();
    
    TEST_ASSERT_TRUE(minTransform.X <= maxTransform.X);
    TEST_ASSERT_TRUE(minTransform.Y <= maxTransform.Y);
    TEST_ASSERT_TRUE(minTransform.Z <= maxTransform.Z);
    
    // Test pixel group access
    IPixelGroup* pg = camera.GetPixelGroup();
    TEST_ASSERT_NOT_NULL(pg);
    TEST_ASSERT_EQUAL(&pixelGroup, pg);
}

void TestCameraBase::RunAllTests() {
    RUN_TEST(TestDefaultConstruction);
    RUN_TEST(TestTransformAccess);
    RUN_TEST(TestCameraLayoutAccess);
    RUN_TEST(TestLookOffsetManagement);
    RUN_TEST(Test2DModeFlag);
    RUN_TEST(TestVirtualInterfaceAccess);
}
