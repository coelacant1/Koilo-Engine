// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testrenderer.cpp
 * @brief Implementation of RenderingEngine unit tests.
 *
 * @date 24/10/2025
 * @author Coela
 */

#include "testrenderer.hpp"
#include <koilo/systems/render/renderer.hpp>
#include <koilo/systems/scene/camera/camera.hpp>
#include <koilo/systems/scene/camera/cameramanager.hpp>
#include <koilo/systems/render/core/pixelgroup.hpp>
#include <koilo/systems/scene/scene.hpp>

using namespace koilo;

void TestRenderingEngine::TestRasterizeNullScene() {
    // Create camera array for manager
    Vector2D pixelLocs[64];
    for (int i = 0; i < 64; i++) {
        pixelLocs[i] = Vector2D(i % 8, i / 8);
    }
    PixelGroup pixelGroup(pixelLocs, 64);
    Transform transform;
    Camera camera(&transform, &pixelGroup);
    CameraBase* cameras[1] = { &camera };
    CameraManager cameraManager(cameras, 1);
    
    // Should not crash with null scene
    RenderingEngine::Rasterize(nullptr, &cameraManager);
    
    TEST_ASSERT_TRUE(true);
}

void TestRenderingEngine::TestRasterizeNullCameraManager() {
    Scene scene;
    
    // Should not crash with null camera manager
    RenderingEngine::Rasterize(&scene, nullptr);
    
    TEST_ASSERT_TRUE(true);
}

void TestRenderingEngine::TestRasterizeEmptyScene() {
    Scene scene;
    
    // Empty camera array
    CameraBase* cameras[1] = { nullptr };
    CameraManager cameraManager(cameras, 0);
    
    // Empty scene and no cameras - should complete without error
    RenderingEngine::Rasterize(&scene, &cameraManager);
    
    TEST_ASSERT_TRUE(true);
}

void TestRenderingEngine::TestRasterizeWithCamera() {
    Scene scene;
    
    // Create camera
    Vector2D pixelLocs[64];
    for (int i = 0; i < 64; i++) {
        pixelLocs[i] = Vector2D(i % 8, i / 8);
    }
    PixelGroup pixelGroup(pixelLocs, 64);
    Transform transform;
    Camera camera(&transform, &pixelGroup);
    
    CameraBase* cameras[1] = { &camera };
    CameraManager cameraManager(cameras, 1);
    
    // Rasterize scene with camera
    RenderingEngine::Rasterize(&scene, &cameraManager);
    
    TEST_ASSERT_TRUE(true);
}

void TestRenderingEngine::TestRayTraceNullScene() {
    // Create camera array for manager
    Vector2D pixelLocs[64];
    for (int i = 0; i < 64; i++) {
        pixelLocs[i] = Vector2D(i % 8, i / 8);
    }
    PixelGroup pixelGroup(pixelLocs, 64);
    Transform transform;
    Camera camera(&transform, &pixelGroup);
    CameraBase* cameras[1] = { &camera };
    CameraManager cameraManager(cameras, 1);
    
    // Should not crash with null scene
    RenderingEngine::RayTrace(nullptr, &cameraManager);
    
    TEST_ASSERT_TRUE(true);
}

void TestRenderingEngine::TestRayTraceNullCameraManager() {
    Scene scene;
    
    // Should not crash with null camera manager
    RenderingEngine::RayTrace(&scene, nullptr);
    
    TEST_ASSERT_TRUE(true);
}

void TestRenderingEngine::TestRayTraceWithCamera() {
    Scene scene;
    
    // Create camera
    Vector2D pixelLocs[64];
    for (int i = 0; i < 64; i++) {
        pixelLocs[i] = Vector2D(i % 8, i / 8);
    }
    PixelGroup pixelGroup(pixelLocs, 64);
    Transform transform;
    Camera camera(&transform, &pixelGroup);
    
    CameraBase* cameras[1] = { &camera };
    CameraManager cameraManager(cameras, 1);
    
    // Ray trace scene with camera
    RenderingEngine::RayTrace(&scene, &cameraManager);
    
    TEST_ASSERT_TRUE(true);
}

void TestRenderingEngine::RunAllTests() {
    RUN_TEST(TestRasterizeNullScene);
    RUN_TEST(TestRasterizeNullCameraManager);
    RUN_TEST(TestRasterizeEmptyScene);
    RUN_TEST(TestRasterizeWithCamera);
    RUN_TEST(TestRayTraceNullScene);
    RUN_TEST(TestRayTraceNullCameraManager);
    RUN_TEST(TestRayTraceWithCamera);
}
