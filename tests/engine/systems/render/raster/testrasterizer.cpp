/**
 * @file testrasterizer.cpp
 * @brief Implementation of Rasterizer unit tests.
 */

#include "testrasterizer.hpp"

// ========== Constructor Tests ==========

void TestRasterizer::TestDefaultConstructor() {
    // Rasterizer is a static utility class with no constructors
    // Test that the static Rasterize method can be called with nullptr safely

    Rasterizer::Rasterize(nullptr, nullptr);

    // If we get here without crashing, the method handles nullptr gracefully
    TEST_ASSERT_TRUE(true);
}

// ========== Edge Cases ==========

// ========== Test Runner ==========

void TestRasterizer::TestParameterizedConstructor() {
    // Rasterizer has no constructors - test with valid minimal parameters

    Scene scene;
    Transform transform;
    PixelGroup pixelGroup(4, Vector2D(20.0f, 20.0f), Vector2D(0.0f, 0.0f), 2);
    Camera camera(&transform, &pixelGroup);

    // Test that Rasterize can be called with valid parameters
    Rasterizer::Rasterize(&scene, &camera);

    TEST_ASSERT_TRUE(true);
}

void TestRasterizer::TestEdgeCases() {
    // Test with valid scene but nullptr camera
    Scene scene;
    Rasterizer::Rasterize(&scene, nullptr);

    // Test with nullptr scene but valid camera
    Transform transform;
    PixelGroup pixelGroup(1, Vector2D(10.0f, 10.0f), Vector2D(0.0f, 0.0f), 1);
    Camera camera(&transform, &pixelGroup);
    Rasterizer::Rasterize(nullptr, &camera);

    // If we get here, method handles edge cases gracefully
    TEST_ASSERT_TRUE(true);
}

void TestRasterizer::RunAllTests() {
    RUN_TEST(TestDefaultConstructor);
    RUN_TEST(TestParameterizedConstructor);
    RUN_TEST(TestEdgeCases);
}
