/**
 * @file testcameramanager.cpp
 * @brief Implementation of CameraManager unit tests.
 */

#include "testcameramanager.hpp"
#include <ptx/systems/render/core/camera.hpp>
#include <ptx/systems/render/core/pixelgroup.hpp>
#include <ptx/core/math/transform.hpp>

// ========== Constructor Tests ==========

void TestCameraManager::TestDefaultConstructor() {
    // Create test cameras
    Transform transform1, transform2;
    PixelGroup pixelGroup1(4, Vector2D(4.0f, 4.0f), Vector2D(0.0f, 0.0f), 2);
    PixelGroup pixelGroup2(4, Vector2D(4.0f, 4.0f), Vector2D(0.0f, 0.0f), 2);

    Camera camera1(&transform1, &pixelGroup1);
    Camera camera2(&transform2, &pixelGroup2);

    CameraBase* cameraArray[] = {&camera1, &camera2};

    CameraManager manager(cameraArray, 2);

    // Verify manager was constructed
    TEST_ASSERT_EQUAL(2, manager.GetCameraCount());
    TEST_ASSERT_NOT_NULL(manager.GetCameras());
}

// ========== Method Tests ==========
void TestCameraManager::TestGetCameras() {
    Transform transform;
    PixelGroup pixelGroup(4, Vector2D(4.0f, 4.0f), Vector2D(0.0f, 0.0f), 2);
    Camera camera(&transform, &pixelGroup);

    CameraBase* cameraArray[] = {&camera};

    CameraManager manager(cameraArray, 1);

    CameraBase** retrieved = manager.GetCameras();
    TEST_ASSERT_NOT_NULL(retrieved);
    TEST_ASSERT_EQUAL_PTR(&camera, retrieved[0]);
}

void TestCameraManager::TestGetCameraCount() {
    Transform t1, t2, t3;
    PixelGroup p1(4, Vector2D(4.0f, 4.0f), Vector2D(0.0f, 0.0f), 2);
    PixelGroup p2(4, Vector2D(4.0f, 4.0f), Vector2D(0.0f, 0.0f), 2);
    PixelGroup p3(4, Vector2D(4.0f, 4.0f), Vector2D(0.0f, 0.0f), 2);

    Camera c1(&t1, &p1);
    Camera c2(&t2, &p2);
    Camera c3(&t3, &p3);

    CameraBase* cameraArray[] = {&c1, &c2, &c3};

    CameraManager manager(cameraArray, 3);

    uint8_t count = manager.GetCameraCount();
    TEST_ASSERT_EQUAL(3, count);
}

// ========== Edge Cases ==========

// ========== Test Runner ==========

void TestCameraManager::TestParameterizedConstructor() {
    // Test with various camera counts
    Transform t1, t2, t3, t4, t5;
    PixelGroup p1(4, Vector2D(4.0f, 4.0f), Vector2D(0.0f, 0.0f), 2);
    PixelGroup p2(4, Vector2D(4.0f, 4.0f), Vector2D(0.0f, 0.0f), 2);
    PixelGroup p3(4, Vector2D(4.0f, 4.0f), Vector2D(0.0f, 0.0f), 2);
    PixelGroup p4(4, Vector2D(4.0f, 4.0f), Vector2D(0.0f, 0.0f), 2);
    PixelGroup p5(4, Vector2D(4.0f, 4.0f), Vector2D(0.0f, 0.0f), 2);

    Camera c1(&t1, &p1);
    Camera c2(&t2, &p2);
    Camera c3(&t3, &p3);
    Camera c4(&t4, &p4);
    Camera c5(&t5, &p5);

    CameraBase* cameraArray[] = {&c1, &c2, &c3, &c4, &c5};

    CameraManager manager(cameraArray, 5);

    TEST_ASSERT_EQUAL(5, manager.GetCameraCount());
    TEST_ASSERT_EQUAL_PTR(&c1, manager.GetCameras()[0]);
    TEST_ASSERT_EQUAL_PTR(&c5, manager.GetCameras()[4]);
}

void TestCameraManager::TestEdgeCases() {
    // Test with single camera
    Transform transform;
    PixelGroup pixelGroup(4, Vector2D(4.0f, 4.0f), Vector2D(0.0f, 0.0f), 2);
    Camera camera(&transform, &pixelGroup);

    CameraBase* singleArray[] = {&camera};
    CameraManager singleManager(singleArray, 1);

    TEST_ASSERT_EQUAL(1, singleManager.GetCameraCount());
    TEST_ASSERT_EQUAL_PTR(&camera, singleManager.GetCameras()[0]);

    // Test with nullptr array (should still construct, but be careful)
    CameraManager nullManager(nullptr, 0);
    TEST_ASSERT_EQUAL(0, nullManager.GetCameraCount());
}

void TestCameraManager::RunAllTests() {
    RUN_TEST(TestDefaultConstructor);
    RUN_TEST(TestParameterizedConstructor);
    RUN_TEST(TestGetCameras);
    RUN_TEST(TestGetCameraCount);
    RUN_TEST(TestEdgeCases);
}
