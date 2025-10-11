/**
 * @file testsimplexnoise.cpp
 * @brief Implementation of SimplexNoise unit tests.
 */

#include "testsimplexnoise.hpp"

// ========== Constructor Tests ==========

void TestSimplexNoise::TestDefaultConstructor() {
    // SimplexNoise has no default constructor - requires a seed parameter
    // Test with a simple seed
    SimplexNoise noise(0);

    // Verify noise can generate values
    float value = noise.Noise(0.0f, 0.0f);
    TEST_ASSERT_TRUE(std::isfinite(value));

    // Simplex noise typically returns values in range [-1, 1]
    TEST_ASSERT_TRUE(value >= -1.5f && value <= 1.5f);
}

// ========== Method Tests ==========
void TestSimplexNoise::TestSetScale() {
    SimplexNoise noise(12345);

    // Test setting different scales
    noise.SetScale(Vector3D(1.0f, 1.0f, 1.0f));
    float val1 = noise.GetNoise(Vector3D(10.0f, 10.0f, 10.0f));
    TEST_ASSERT_TRUE(std::isfinite(val1));

    noise.SetScale(Vector3D(2.0f, 2.0f, 2.0f));
    float val2 = noise.GetNoise(Vector3D(10.0f, 10.0f, 10.0f));
    TEST_ASSERT_TRUE(std::isfinite(val2));

    // Different scales should (usually) produce different noise values
    // But both should be finite and in valid range

    noise.SetScale(Vector3D(0.5f, 0.5f, 0.5f));
    float val3 = noise.GetNoise(Vector3D(10.0f, 10.0f, 10.0f));
    TEST_ASSERT_TRUE(std::isfinite(val3));

    noise.SetScale(Vector3D(10.0f, 10.0f, 10.0f));
    float val4 = noise.GetNoise(Vector3D(10.0f, 10.0f, 10.0f));
    TEST_ASSERT_TRUE(std::isfinite(val4));
}

void TestSimplexNoise::TestSetZPosition() {
    SimplexNoise noise(42);

    // Test different Z positions
    noise.SetZPosition(0.0f);
    float val1 = noise.GetNoise(Vector3D(5.0f, 5.0f, 0.0f));
    TEST_ASSERT_TRUE(std::isfinite(val1));

    noise.SetZPosition(10.0f);
    float val2 = noise.GetNoise(Vector3D(5.0f, 5.0f, 0.0f));
    TEST_ASSERT_TRUE(std::isfinite(val2));

    noise.SetZPosition(-10.0f);
    float val3 = noise.GetNoise(Vector3D(5.0f, 5.0f, 0.0f));
    TEST_ASSERT_TRUE(std::isfinite(val3));

    noise.SetZPosition(100.0f);
    float val4 = noise.GetNoise(Vector3D(5.0f, 5.0f, 0.0f));
    TEST_ASSERT_TRUE(std::isfinite(val4));

    // All values should be in typical simplex noise range
    TEST_ASSERT_TRUE(val1 >= -1.5f && val1 <= 1.5f);
    TEST_ASSERT_TRUE(val2 >= -1.5f && val2 <= 1.5f);
    TEST_ASSERT_TRUE(val3 >= -1.5f && val3 <= 1.5f);
    TEST_ASSERT_TRUE(val4 >= -1.5f && val4 <= 1.5f);
}

void TestSimplexNoise::TestGetNoise() {
    SimplexNoise noise(999);

    // Test getting noise at various positions
    float noise1 = noise.GetNoise(Vector3D(0.0f, 0.0f, 0.0f));
    TEST_ASSERT_TRUE(std::isfinite(noise1));
    TEST_ASSERT_TRUE(noise1 >= -1.5f && noise1 <= 1.5f);

    float noise2 = noise.GetNoise(Vector3D(10.0f, 10.0f, 10.0f));
    TEST_ASSERT_TRUE(std::isfinite(noise2));
    TEST_ASSERT_TRUE(noise2 >= -1.5f && noise2 <= 1.5f);

    float noise3 = noise.GetNoise(Vector3D(-5.0f, -5.0f, -5.0f));
    TEST_ASSERT_TRUE(std::isfinite(noise3));
    TEST_ASSERT_TRUE(noise3 >= -1.5f && noise3 <= 1.5f);

    float noise4 = noise.GetNoise(Vector3D(100.0f, 200.0f, 300.0f));
    TEST_ASSERT_TRUE(std::isfinite(noise4));
    TEST_ASSERT_TRUE(noise4 >= -1.5f && noise4 <= 1.5f);

    // Test with fractional coordinates
    float noise5 = noise.GetNoise(Vector3D(1.5f, 2.7f, 3.9f));
    TEST_ASSERT_TRUE(std::isfinite(noise5));
    TEST_ASSERT_TRUE(noise5 >= -1.5f && noise5 <= 1.5f);

    // Test 2D noise overload
    float noise2D1 = noise.Noise(0.0f, 0.0f);
    TEST_ASSERT_TRUE(std::isfinite(noise2D1));
    TEST_ASSERT_TRUE(noise2D1 >= -1.5f && noise2D1 <= 1.5f);

    float noise2D2 = noise.Noise(15.5f, 27.3f);
    TEST_ASSERT_TRUE(std::isfinite(noise2D2));
    TEST_ASSERT_TRUE(noise2D2 >= -1.5f && noise2D2 <= 1.5f);

    // Test 3D noise overload
    float noise3D1 = noise.Noise(0.0f, 0.0f, 0.0f);
    TEST_ASSERT_TRUE(std::isfinite(noise3D1));
    TEST_ASSERT_TRUE(noise3D1 >= -1.5f && noise3D1 <= 1.5f);

    float noise3D2 = noise.Noise(5.5f, 10.3f, 15.7f);
    TEST_ASSERT_TRUE(std::isfinite(noise3D2));
    TEST_ASSERT_TRUE(noise3D2 >= -1.5f && noise3D2 <= 1.5f);
}

// ========== Edge Cases ==========

// ========== Test Runner ==========

void TestSimplexNoise::TestParameterizedConstructor() {
    // Test with various seeds
    SimplexNoise noise1(0);
    float val1 = noise1.Noise(5.0f, 5.0f);
    TEST_ASSERT_TRUE(std::isfinite(val1));

    SimplexNoise noise2(12345);
    float val2 = noise2.Noise(5.0f, 5.0f);
    TEST_ASSERT_TRUE(std::isfinite(val2));

    SimplexNoise noise3(-999);
    float val3 = noise3.Noise(5.0f, 5.0f);
    TEST_ASSERT_TRUE(std::isfinite(val3));

    SimplexNoise noise4(999999);
    float val4 = noise4.Noise(5.0f, 5.0f);
    TEST_ASSERT_TRUE(std::isfinite(val4));

    // Different seeds should produce different noise patterns
    // But all should be in valid range
    TEST_ASSERT_TRUE(val1 >= -1.5f && val1 <= 1.5f);
    TEST_ASSERT_TRUE(val2 >= -1.5f && val2 <= 1.5f);
    TEST_ASSERT_TRUE(val3 >= -1.5f && val3 <= 1.5f);
    TEST_ASSERT_TRUE(val4 >= -1.5f && val4 <= 1.5f);
}

void TestSimplexNoise::TestEdgeCases() {
    SimplexNoise noise(42);

    // Test with zero coordinates
    float zero2D = noise.Noise(0.0f, 0.0f);
    TEST_ASSERT_TRUE(std::isfinite(zero2D));

    float zero3D = noise.Noise(0.0f, 0.0f, 0.0f);
    TEST_ASSERT_TRUE(std::isfinite(zero3D));

    // Test with very large coordinates
    float large2D = noise.Noise(10000.0f, 10000.0f);
    TEST_ASSERT_TRUE(std::isfinite(large2D));

    float large3D = noise.Noise(10000.0f, 10000.0f, 10000.0f);
    TEST_ASSERT_TRUE(std::isfinite(large3D));

    // Test with negative coordinates
    float neg2D = noise.Noise(-100.0f, -200.0f);
    TEST_ASSERT_TRUE(std::isfinite(neg2D));

    float neg3D = noise.Noise(-100.0f, -200.0f, -300.0f);
    TEST_ASSERT_TRUE(std::isfinite(neg3D));

    // Test with very small fractional values
    float tiny2D = noise.Noise(0.001f, 0.002f);
    TEST_ASSERT_TRUE(std::isfinite(tiny2D));

    float tiny3D = noise.Noise(0.001f, 0.002f, 0.003f);
    TEST_ASSERT_TRUE(std::isfinite(tiny3D));

    // Test with extreme scale values
    noise.SetScale(Vector3D(0.001f, 0.001f, 0.001f));
    float tinyScale = noise.GetNoise(Vector3D(10.0f, 10.0f, 10.0f));
    TEST_ASSERT_TRUE(std::isfinite(tinyScale));

    noise.SetScale(Vector3D(1000.0f, 1000.0f, 1000.0f));
    float largeScale = noise.GetNoise(Vector3D(10.0f, 10.0f, 10.0f));
    TEST_ASSERT_TRUE(std::isfinite(largeScale));

    // Test with negative scale (edge case)
    noise.SetScale(Vector3D(-1.0f, -1.0f, -1.0f));
    float negScale = noise.GetNoise(Vector3D(10.0f, 10.0f, 10.0f));
    TEST_ASSERT_TRUE(std::isfinite(negScale));

    // Test with mixed positive/negative scale
    noise.SetScale(Vector3D(1.0f, -1.0f, 1.0f));
    float mixedScale = noise.GetNoise(Vector3D(10.0f, 10.0f, 10.0f));
    TEST_ASSERT_TRUE(std::isfinite(mixedScale));

    // Test continuity: nearby points should have similar values (smoothness test)
    SimplexNoise continuityNoise(123);
    float p1 = continuityNoise.Noise(10.0f, 10.0f);
    float p2 = continuityNoise.Noise(10.01f, 10.0f);  // Very close point

    // Both should be finite
    TEST_ASSERT_TRUE(std::isfinite(p1));
    TEST_ASSERT_TRUE(std::isfinite(p2));

    // Simplex noise is continuous, so very close points should have similar values
    // (not drastically different)
    float diff = std::abs(p1 - p2);
    TEST_ASSERT_TRUE(diff < 1.0f);  // Should not jump more than 1.0

    // Test with different Z positions combined with GetNoise
    noise.SetZPosition(50.0f);
    float zPos1 = noise.GetNoise(Vector3D(5.0f, 5.0f, 0.0f));
    TEST_ASSERT_TRUE(std::isfinite(zPos1));

    noise.SetZPosition(-50.0f);
    float zPos2 = noise.GetNoise(Vector3D(5.0f, 5.0f, 0.0f));
    TEST_ASSERT_TRUE(std::isfinite(zPos2));

    // Test same seed produces same output
    SimplexNoise noise1(555);
    SimplexNoise noise2(555);
    float same1 = noise1.Noise(7.5f, 8.5f);
    float same2 = noise2.Noise(7.5f, 8.5f);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, same1, same2);

    // Test multiple noise samples in a grid
    for (int x = 0; x < 10; x++) {
        for (int y = 0; y < 10; y++) {
            float gridNoise = noise.Noise(static_cast<float>(x), static_cast<float>(y));
            TEST_ASSERT_TRUE(std::isfinite(gridNoise));
            TEST_ASSERT_TRUE(gridNoise >= -1.5f && gridNoise <= 1.5f);
        }
    }
}

void TestSimplexNoise::RunAllTests() {
    RUN_TEST(TestDefaultConstructor);
    RUN_TEST(TestParameterizedConstructor);

    RUN_TEST(TestSetScale);
    RUN_TEST(TestSetZPosition);
    RUN_TEST(TestGetNoise);
    RUN_TEST(TestEdgeCases);
}
