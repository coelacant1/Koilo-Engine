// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testvolumecamera.cpp
 * @brief Tests for VolumeCamera: construction, sampling, gamma, brightness,
 *        bounds computation, and packed output.
 */
#include "testvolumecamera.hpp"
#include <koilo/systems/scene/camera/volumecamera.hpp>
#include <koilo/systems/render/core/pixelgroup.hpp>
#include <koilo/core/math/transform.hpp>

#include <cstring>
#include <vector>
#include <cmath>

using namespace koilo;

// Helper: create a 2x2 red FBO (RGB888, all pixels = (255,0,0))
static std::vector<uint8_t> MakeRedFBO(uint32_t w, uint32_t h) {
    std::vector<uint8_t> fbo(w * h * 3, 0);
    for (size_t i = 0; i < fbo.size(); i += 3) {
        fbo[i] = 255;  // R
    }
    return fbo;
}

// Helper: create a solid color FBO
static std::vector<uint8_t> MakeSolidFBO(uint32_t w, uint32_t h,
                                          uint8_t r, uint8_t g, uint8_t b) {
    std::vector<uint8_t> fbo(w * h * 3);
    for (size_t i = 0; i < fbo.size(); i += 3) {
        fbo[i] = r;
        fbo[i + 1] = g;
        fbo[i + 2] = b;
    }
    return fbo;
}

// Helper: pixel positions for a single pixel at UV center (0.5, 0.5)
static Vector2D sCenterPixel(0.5f, 0.5f);

// -- Construction --

static void test_ConstructWithPixelGroup() {
    PixelGroup pg(&sCenterPixel, 1);
    VolumeCamera cam(&pg);

    TEST_ASSERT_NOT_NULL(cam.GetPixelGroup());
    TEST_ASSERT_EQUAL_UINT32(1, cam.GetPixelCount());
    TEST_ASSERT_NOT_NULL(cam.GetTransform());
}

static void test_ConstructWithExternalTransform() {
    PixelGroup pg(&sCenterPixel, 1);
    Transform t;
    t.SetPosition(Vector3D(1, 2, 3));
    VolumeCamera cam(&t, &pg);

    auto* xform = cam.GetTransform();
    TEST_ASSERT_NOT_NULL(xform);
    auto pos = xform->GetPosition();
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, pos.X);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 2.0f, pos.Y);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 3.0f, pos.Z);
}

static void test_DefaultResolution() {
    PixelGroup pg(&sCenterPixel, 1);
    VolumeCamera cam(&pg);
    TEST_ASSERT_EQUAL_UINT32(128, cam.GetFBOWidth());
    TEST_ASSERT_EQUAL_UINT32(128, cam.GetFBOHeight());
}

static void test_SetResolution() {
    PixelGroup pg(&sCenterPixel, 1);
    VolumeCamera cam(&pg);
    cam.SetResolution(64, 32);
    TEST_ASSERT_EQUAL_UINT32(64, cam.GetFBOWidth());
    TEST_ASSERT_EQUAL_UINT32(32, cam.GetFBOHeight());
}

// -- Sampling --

static void test_SampleSolidRedFBO() {
    PixelGroup pg(&sCenterPixel, 1);
    VolumeCamera cam(&pg);
    cam.SetGamma(1.0f);
    cam.SetBrightness(255);

    auto fbo = MakeRedFBO(4, 4);
    cam.SamplePixels(fbo.data(), 4, 4);

    const uint8_t* out = cam.GetPackedOutput();
    TEST_ASSERT_NOT_NULL(out);
    TEST_ASSERT_EQUAL_UINT8(255, out[0]); // R
    TEST_ASSERT_EQUAL_UINT8(0, out[1]);   // G
    TEST_ASSERT_EQUAL_UINT8(0, out[2]);   // B
}

static void test_SampleMultiplePixels() {
    // Two pixels: top-left and bottom-right corners
    Vector2D positions[2] = {{0.0f, 0.0f}, {1.0f, 1.0f}};
    PixelGroup pg(positions, 2);
    VolumeCamera cam(&pg);
    cam.SetGamma(1.0f);
    cam.SetBrightness(255);

    auto fbo = MakeSolidFBO(4, 4, 100, 150, 200);
    cam.SamplePixels(fbo.data(), 4, 4);

    const uint8_t* out = cam.GetPackedOutput();
    TEST_ASSERT_EQUAL_UINT32(6, cam.GetPackedOutputSize());
    // Both pixels sample a solid color so both should match
    TEST_ASSERT_EQUAL_UINT8(100, out[0]);
    TEST_ASSERT_EQUAL_UINT8(150, out[1]);
    TEST_ASSERT_EQUAL_UINT8(200, out[2]);
    TEST_ASSERT_EQUAL_UINT8(100, out[3]);
    TEST_ASSERT_EQUAL_UINT8(150, out[4]);
    TEST_ASSERT_EQUAL_UINT8(200, out[5]);
}

static void test_SampleOutOfRangePixelIsBlack() {
    // Pixel at UV (-0.5, 2.0) -- out of range
    Vector2D offscreen(-0.5f, 2.0f);
    PixelGroup pg(&offscreen, 1);
    VolumeCamera cam(&pg);
    cam.SetGamma(1.0f);

    auto fbo = MakeSolidFBO(4, 4, 255, 255, 255);
    cam.SamplePixels(fbo.data(), 4, 4);

    const uint8_t* out = cam.GetPackedOutput();
    TEST_ASSERT_EQUAL_UINT8(0, out[0]);
    TEST_ASSERT_EQUAL_UINT8(0, out[1]);
    TEST_ASSERT_EQUAL_UINT8(0, out[2]);
}

static void test_SampleNullFBOIsNoop() {
    PixelGroup pg(&sCenterPixel, 1);
    VolumeCamera cam(&pg);
    cam.SamplePixels(nullptr, 4, 4);
    TEST_ASSERT_NULL(cam.GetPackedOutput());
}

// -- Gamma and brightness --

static void test_GammaReducesBrightValues() {
    PixelGroup pg(&sCenterPixel, 1);
    VolumeCamera cam(&pg);
    cam.SetGamma(2.2f);
    cam.SetBrightness(255);

    // White FBO
    auto fbo = MakeSolidFBO(4, 4, 255, 255, 255);
    cam.SamplePixels(fbo.data(), 4, 4);
    const uint8_t* out = cam.GetPackedOutput();
    // gamma(255/255)^2.2 * 255 = 255 (1.0^2.2 = 1.0)
    TEST_ASSERT_EQUAL_UINT8(255, out[0]);

    // Mid-gray (128)
    auto fboGray = MakeSolidFBO(4, 4, 128, 128, 128);
    cam.SamplePixels(fboGray.data(), 4, 4);
    out = cam.GetPackedOutput();
    // gamma(128/255)^2.2 * 255 ~ 56
    TEST_ASSERT_TRUE(out[0] < 128); // Should be significantly darker
    TEST_ASSERT_TRUE(out[0] > 30);  // But not black
}

static void test_BrightnessScalesOutput() {
    PixelGroup pg(&sCenterPixel, 1);
    VolumeCamera cam(&pg);
    cam.SetGamma(1.0f);
    cam.SetBrightness(128); // ~50%

    auto fbo = MakeSolidFBO(4, 4, 200, 200, 200);
    cam.SamplePixels(fbo.data(), 4, 4);
    const uint8_t* out = cam.GetPackedOutput();
    // 200 * (128/255) ~ 100
    TEST_ASSERT_TRUE(out[0] < 150);
    TEST_ASSERT_TRUE(out[0] > 50);
}

static void test_ZeroBrightnessIsBlack() {
    PixelGroup pg(&sCenterPixel, 1);
    VolumeCamera cam(&pg);
    cam.SetGamma(1.0f);
    cam.SetBrightness(0);

    auto fbo = MakeSolidFBO(4, 4, 255, 255, 255);
    cam.SamplePixels(fbo.data(), 4, 4);
    const uint8_t* out = cam.GetPackedOutput();
    TEST_ASSERT_EQUAL_UINT8(0, out[0]);
    TEST_ASSERT_EQUAL_UINT8(0, out[1]);
    TEST_ASSERT_EQUAL_UINT8(0, out[2]);
}

// -- Bounds --

static void test_CameraBoundsFromPixelGroup() {
    Vector2D positions[3] = {{0.1f, 0.2f}, {0.9f, 0.8f}, {0.5f, 0.5f}};
    PixelGroup pg(positions, 3);
    VolumeCamera cam(&pg);

    Vector2D minC = cam.GetCameraMinCoordinate();
    Vector2D maxC = cam.GetCameraMaxCoordinate();
    Vector2D center = cam.GetCameraCenterCoordinate();

    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.1f, minC.X);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.2f, minC.Y);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.9f, maxC.X);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.8f, maxC.Y);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.5f, center.X);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.5f, center.Y);
}

// -- SetPixelGroup --

static void test_SetPixelGroupResetsState() {
    Vector2D pos1(0.5f, 0.5f);
    Vector2D pos2(0.2f, 0.8f);
    PixelGroup pg1(&pos1, 1);
    PixelGroup pg2(&pos2, 1);

    VolumeCamera cam(&pg1);
    cam.SetGamma(1.0f);
    auto fbo = MakeSolidFBO(4, 4, 100, 100, 100);
    cam.SamplePixels(fbo.data(), 4, 4);
    TEST_ASSERT_EQUAL_UINT32(3, cam.GetPackedOutputSize());

    cam.SetPixelGroup(&pg2);
    // Old output should be cleared
    TEST_ASSERT_EQUAL_UINT32(0, cam.GetPackedOutputSize());
    TEST_ASSERT_NULL(cam.GetPackedOutput());
}

// -- Not 2D --

static void test_VolumeCameraIsNot2D() {
    PixelGroup pg(&sCenterPixel, 1);
    VolumeCamera cam(&pg);
    TEST_ASSERT_FALSE(cam.Is2D());
}

void TestVolumeCamera::TestDefaultConstructor() {
    // TODO: Implement test for default constructor
    VolumeCamera obj;
    TEST_ASSERT_TRUE(false);  // Not implemented
}

void TestVolumeCamera::TestEdgeCases() {
    // TODO: Test edge cases (null, boundaries, extreme values)
    TEST_ASSERT_TRUE(false);  // Not implemented
}

void TestVolumeCamera::TestGetBrightness() {
    // TODO: Implement test for GetBrightness()
    VolumeCamera obj;
    TEST_ASSERT_TRUE(false);  // Not implemented
}

void TestVolumeCamera::TestGetCameraCenterCoordinate() {
    // TODO: Implement test for GetCameraCenterCoordinate()
    VolumeCamera obj;
    TEST_ASSERT_TRUE(false);  // Not implemented
}

void TestVolumeCamera::TestGetCameraMaxCoordinate() {
    // TODO: Implement test for GetCameraMaxCoordinate()
    VolumeCamera obj;
    TEST_ASSERT_TRUE(false);  // Not implemented
}

void TestVolumeCamera::TestGetCameraMinCoordinate() {
    // TODO: Implement test for GetCameraMinCoordinate()
    VolumeCamera obj;
    TEST_ASSERT_TRUE(false);  // Not implemented
}

void TestVolumeCamera::TestGetCameraTransformCenter() {
    // TODO: Implement test for GetCameraTransformCenter()
    VolumeCamera obj;
    TEST_ASSERT_TRUE(false);  // Not implemented
}

void TestVolumeCamera::TestGetCameraTransformMax() {
    // TODO: Implement test for GetCameraTransformMax()
    VolumeCamera obj;
    TEST_ASSERT_TRUE(false);  // Not implemented
}

void TestVolumeCamera::TestGetCameraTransformMin() {
    // TODO: Implement test for GetCameraTransformMin()
    VolumeCamera obj;
    TEST_ASSERT_TRUE(false);  // Not implemented
}

void TestVolumeCamera::TestGetFBOHeight() {
    // TODO: Implement test for GetFBOHeight()
    VolumeCamera obj;
    TEST_ASSERT_TRUE(false);  // Not implemented
}

void TestVolumeCamera::TestGetFBOWidth() {
    // TODO: Implement test for GetFBOWidth()
    VolumeCamera obj;
    TEST_ASSERT_TRUE(false);  // Not implemented
}

void TestVolumeCamera::TestGetGamma() {
    // TODO: Implement test for GetGamma()
    VolumeCamera obj;
    TEST_ASSERT_TRUE(false);  // Not implemented
}

void TestVolumeCamera::TestGetPixelCount() {
    // TODO: Implement test for GetPixelCount()
    VolumeCamera obj;
    TEST_ASSERT_TRUE(false);  // Not implemented
}

void TestVolumeCamera::TestGetPixelGroup() {
    // TODO: Implement test for GetPixelGroup()
    VolumeCamera obj;
    TEST_ASSERT_TRUE(false);  // Not implemented
}

void TestVolumeCamera::TestParameterizedConstructor() {
    // TODO: Implement test for parameterized constructor
    TEST_ASSERT_TRUE(false);  // Not implemented
}

void TestVolumeCamera::TestSetBrightness() {
    // TODO: Implement test for SetBrightness()
    VolumeCamera obj;
    TEST_ASSERT_TRUE(false);  // Not implemented
}

void TestVolumeCamera::TestSetGamma() {
    // TODO: Implement test for SetGamma()
    VolumeCamera obj;
    TEST_ASSERT_TRUE(false);  // Not implemented
}

void TestVolumeCamera::TestSetResolution() {
    // TODO: Implement test for SetResolution()
    VolumeCamera obj;
    TEST_ASSERT_TRUE(false);  // Not implemented
}

void TestVolumeCamera::RunAllTests() {
    RUN_TEST(test_ConstructWithPixelGroup);
    RUN_TEST(test_ConstructWithExternalTransform);
    RUN_TEST(test_DefaultResolution);
    RUN_TEST(test_SetResolution);
    RUN_TEST(test_SampleSolidRedFBO);
    RUN_TEST(test_SampleMultiplePixels);
    RUN_TEST(test_SampleOutOfRangePixelIsBlack);
    RUN_TEST(test_SampleNullFBOIsNoop);
    RUN_TEST(test_GammaReducesBrightValues);
    RUN_TEST(test_BrightnessScalesOutput);
    RUN_TEST(test_ZeroBrightnessIsBlack);
    RUN_TEST(test_CameraBoundsFromPixelGroup);
    RUN_TEST(test_SetPixelGroupResetsState);
    RUN_TEST(test_VolumeCameraIsNot2D);
    RUN_TEST(TestDefaultConstructor);
    RUN_TEST(TestEdgeCases);
    RUN_TEST(TestGetBrightness);
    RUN_TEST(TestGetCameraCenterCoordinate);
    RUN_TEST(TestGetCameraMaxCoordinate);
    RUN_TEST(TestGetCameraMinCoordinate);
    RUN_TEST(TestGetCameraTransformCenter);
    RUN_TEST(TestGetCameraTransformMax);
    RUN_TEST(TestGetCameraTransformMin);
    RUN_TEST(TestGetFBOHeight);
    RUN_TEST(TestGetFBOWidth);
    RUN_TEST(TestGetGamma);
    RUN_TEST(TestGetPixelCount);
    RUN_TEST(TestGetPixelGroup);
    RUN_TEST(TestParameterizedConstructor);
    RUN_TEST(TestSetBrightness);
    RUN_TEST(TestSetGamma);
    RUN_TEST(TestSetResolution);
}
