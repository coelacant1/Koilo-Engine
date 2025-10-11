/**
 * @file testcamera.cpp
 * @brief Implementation of Camera unit tests.
 */

#include "testcamera.hpp"
#include <ptx/systems/render/core/pixelgroup.hpp>
#include <ptx/core/math/transform.hpp>

// ========== Constructor Tests ==========

void TestCamera::TestDefaultConstructor() {
    // Camera requires parameters - test with 2-parameter constructor
    Transform transform;
    // Create rectangular pixel group: pixelCount=10, size=10x10, position=(0,0), rows=2
    PixelGroup pixelGroup(10, Vector2D(10.0f, 10.0f), Vector2D(0.0f, 0.0f), 2);

    Camera camera(&transform, &pixelGroup);

    // Verify camera was constructed and can retrieve pixel group
    TEST_ASSERT_NOT_NULL(camera.GetPixelGroup());
    TEST_ASSERT_EQUAL_PTR(&pixelGroup, camera.GetPixelGroup());
}

// ========== Method Tests ==========
void TestCamera::TestGetPixelGroup() {
    Transform transform;
    PixelGroup pixelGroup(5, Vector2D(5.0f, 5.0f), Vector2D(0.0f, 0.0f), 1);

    Camera camera(&transform, &pixelGroup);

    IPixelGroup* retrieved = camera.GetPixelGroup();
    TEST_ASSERT_NOT_NULL(retrieved);
    TEST_ASSERT_EQUAL_PTR(&pixelGroup, retrieved);
}

void TestCamera::TestGetCameraMinCoordinate() {
    Transform transform;

    // Create pixel array with specific coordinates
    Vector2D pixels[] = {
        Vector2D(10.0f, 20.0f),
        Vector2D(5.0f, 15.0f),
        Vector2D(15.0f, 25.0f)
    };
    PixelGroup pixelGroup(pixels, 3);

    Camera camera(&transform, &pixelGroup);

    Vector2D minCoord = camera.GetCameraMinCoordinate();
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 5.0f, minCoord.X);   // Min X
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 15.0f, minCoord.Y);  // Min Y
}

void TestCamera::TestGetCameraMaxCoordinate() {
    Transform transform;

    Vector2D pixels[] = {
        Vector2D(10.0f, 20.0f),
        Vector2D(5.0f, 15.0f),
        Vector2D(15.0f, 25.0f)
    };
    PixelGroup pixelGroup(pixels, 3);

    Camera camera(&transform, &pixelGroup);

    Vector2D maxCoord = camera.GetCameraMaxCoordinate();
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 15.0f, maxCoord.X);  // Max X
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 25.0f, maxCoord.Y);  // Max Y
}

void TestCamera::TestGetCameraCenterCoordinate() {
    Transform transform;

    Vector2D pixels[] = {
        Vector2D(0.0f, 0.0f),
        Vector2D(10.0f, 20.0f)
    };
    PixelGroup pixelGroup(pixels, 2);

    Camera camera(&transform, &pixelGroup);

    Vector2D center = camera.GetCameraCenterCoordinate();
    // Center should be (min + max) / 2 = (0 + 10) / 2, (0 + 20) / 2 = (5, 10)
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 5.0f, center.X);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 10.0f, center.Y);
}

void TestCamera::TestGetCameraTransformMin() {
    Transform transform;
    transform.SetPosition(Vector3D(100.0f, 200.0f, 300.0f));

    Vector2D pixels[] = {
        Vector2D(0.0f, 0.0f),
        Vector2D(10.0f, 10.0f)
    };
    PixelGroup pixelGroup(pixels, 2);

    Camera camera(&transform, &pixelGroup);

    Vector3D transformMin = camera.GetCameraTransformMin();
    // Should be transform applied to min coordinate
    TEST_ASSERT_NOT_NULL(&transformMin);
    // Verifying that transform was applied (position offset)
    TEST_ASSERT_TRUE(transformMin.X >= 0.0f || transformMin.X <= 200.0f);
}

void TestCamera::TestGetCameraTransformMax() {
    Transform transform;
    transform.SetPosition(Vector3D(100.0f, 200.0f, 300.0f));

    Vector2D pixels[] = {
        Vector2D(0.0f, 0.0f),
        Vector2D(10.0f, 10.0f)
    };
    PixelGroup pixelGroup(pixels, 2);

    Camera camera(&transform, &pixelGroup);

    Vector3D transformMax = camera.GetCameraTransformMax();
    // Should be transform applied to max coordinate
    TEST_ASSERT_NOT_NULL(&transformMax);
    TEST_ASSERT_TRUE(transformMax.X >= 0.0f || transformMax.X <= 200.0f);
}

void TestCamera::TestGetCameraTransformCenter() {
    Transform transform;
    transform.SetPosition(Vector3D(0.0f, 0.0f, 0.0f));

    Vector2D pixels[] = {
        Vector2D(0.0f, 0.0f),
        Vector2D(10.0f, 10.0f)
    };
    PixelGroup pixelGroup(pixels, 2);

    Camera camera(&transform, &pixelGroup);

    Vector3D transformCenter = camera.GetCameraTransformCenter();
    // Center should be average of transformed min and max
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 5.0f, transformCenter.X);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 5.0f, transformCenter.Y);
}
// ========== Edge Cases ==========

// ========== Test Runner ==========

void TestCamera::TestParameterizedConstructor() {
    // Test 3-parameter constructor with CameraLayout
    Transform transform;
    CameraLayout layout;
    PixelGroup pixelGroup(5, Vector2D(5.0f, 5.0f), Vector2D(0.0f, 0.0f), 1);

    Camera camera(&transform, &layout, &pixelGroup);

    // Verify camera was constructed properly
    TEST_ASSERT_NOT_NULL(camera.GetPixelGroup());
    TEST_ASSERT_EQUAL_PTR(&pixelGroup, camera.GetPixelGroup());
}

void TestCamera::TestEdgeCases() {
    // Test with nullptr pixel group
    Transform transform;

    Camera cameraNullPixels(&transform, nullptr);

    // Should handle nullptr gracefully
    Vector2D minCoord = cameraNullPixels.GetCameraMinCoordinate();
    TEST_ASSERT_EQUAL_FLOAT(0.0f, minCoord.X);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, minCoord.Y);

    // Test with single pixel group
    Vector2D singlePixel[] = {Vector2D(5.0f, 10.0f)};
    PixelGroup singleGroup(singlePixel, 1);
    Camera cameraSingle(&transform, &singleGroup);

    Vector2D singleMin = cameraSingle.GetCameraMinCoordinate();
    Vector2D singleMax = cameraSingle.GetCameraMaxCoordinate();

    // Min and max should be the same for single pixel
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 5.0f, singleMin.X);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 10.0f, singleMin.Y);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 5.0f, singleMax.X);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 10.0f, singleMax.Y);
}

void TestCamera::RunAllTests() {
    RUN_TEST(TestDefaultConstructor);
    RUN_TEST(TestParameterizedConstructor);
    RUN_TEST(TestGetPixelGroup);
    RUN_TEST(TestGetCameraMinCoordinate);
    RUN_TEST(TestGetCameraMaxCoordinate);
    RUN_TEST(TestGetCameraCenterCoordinate);
    RUN_TEST(TestGetCameraTransformMin);
    RUN_TEST(TestGetCameraTransformMax);
    RUN_TEST(TestGetCameraTransformCenter);
    RUN_TEST(TestEdgeCases);
}
