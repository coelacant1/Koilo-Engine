/**
 * @file testvectorkalmanfilter.cpp
 * @brief Implementation of VectorKalmanFilter unit tests.
 */

#include "testvectorkalmanfilter.hpp"
#include <cmath>

// ========== Constructor Tests ==========

void TestVectorKalmanFilter::TestParameterizedConstructor() {
    // Test construction with specific parameters
    float processNoise = 0.01f;
    float sensorNoise = 0.1f;
    float errorCovariance = 1.0f;

    VectorKalmanFilter filter(processNoise, sensorNoise, errorCovariance);

    // Filter a vector to verify it's operational
    Vector3D testVector(1.0f, 2.0f, 3.0f);
    Vector3D result = filter.Filter(testVector);

    // Result components should be valid and finite
    TEST_ASSERT_TRUE(std::isfinite(result.X));
    TEST_ASSERT_TRUE(std::isfinite(result.Y));
    TEST_ASSERT_TRUE(std::isfinite(result.Z));
}

// ========== Method Tests ==========

void TestVectorKalmanFilter::TestFilter() {
    VectorKalmanFilter filter(0.01f, 0.1f, 1.0f);

    // Filter a constant vector
    Vector3D measurement(10.0f, 20.0f, 30.0f);
    Vector3D result = filter.Filter(measurement);

    // Each component should be between 0 (initial) and the measurement
    TEST_ASSERT_TRUE(result.X > 0.0f && result.X < measurement.X);
    TEST_ASSERT_TRUE(result.Y > 0.0f && result.Y < measurement.Y);
    TEST_ASSERT_TRUE(result.Z > 0.0f && result.Z < measurement.Z);

    // Filter again with same value, should get closer
    Vector3D result2 = filter.Filter(measurement);
    TEST_ASSERT_TRUE(result2.X > result.X);
    TEST_ASSERT_TRUE(result2.Y > result.Y);
    TEST_ASSERT_TRUE(result2.Z > result.Z);
}

void TestVectorKalmanFilter::TestReset() {
    VectorKalmanFilter filter(0.01f, 0.1f, 1.0f);

    // Apply some filtering to change state
    Vector3D input(10.0f, 15.0f, 20.0f);
    filter.Filter(input);
    filter.Filter(input);

    // Reset to new estimation
    Vector3D newEstimation(5.0f, 7.0f, 9.0f);
    filter.Reset(newEstimation, 0.5f);

    // Filter a small value and verify reset worked
    Vector3D small(0.1f, 0.1f, 0.1f);
    Vector3D result = filter.Filter(small);

    // Result should be influenced by the reset estimation, not the previous state
    // The result should be closer to the reset estimation than to the previous filtered values
    TEST_ASSERT_TRUE(std::abs(result.X - newEstimation.X) < std::abs(result.X - input.X));
}

// ========== Vector Kalman Filter Behavior Tests ==========

// ========== Edge Cases ==========

// ========== Test Runner ==========

void TestVectorKalmanFilter::TestDefaultConstructor() {
    // VectorKalmanFilter has no default constructor - it requires three parameters
    // Test that we can construct with minimal/default-like parameters
    VectorKalmanFilter filter(0.01f, 0.1f, 1.0f);

    // Verify the filter is in a valid initial state by filtering a vector
    Vector3D testVector(1.0f, 2.0f, 3.0f);
    Vector3D result = filter.Filter(testVector);

    // Result should be valid and finite
    TEST_ASSERT_TRUE(std::isfinite(result.X));
    TEST_ASSERT_TRUE(std::isfinite(result.Y));
    TEST_ASSERT_TRUE(std::isfinite(result.Z));

    // Each component should be between 0 (initial estimate) and measurement
    TEST_ASSERT_TRUE(result.X >= 0.0f && result.X <= testVector.X);
    TEST_ASSERT_TRUE(result.Y >= 0.0f && result.Y <= testVector.Y);
    TEST_ASSERT_TRUE(result.Z >= 0.0f && result.Z <= testVector.Z);
}

void TestVectorKalmanFilter::TestEdgeCases() {
    // Test with zero vector
    VectorKalmanFilter zeroFilter(0.01f, 0.1f, 1.0f);
    Vector3D zeroVec(0.0f, 0.0f, 0.0f);
    Vector3D zeroResult = zeroFilter.Filter(zeroVec);

    TEST_ASSERT_TRUE(std::isfinite(zeroResult.X));
    TEST_ASSERT_TRUE(std::isfinite(zeroResult.Y));
    TEST_ASSERT_TRUE(std::isfinite(zeroResult.Z));

    // Test with negative components
    VectorKalmanFilter negFilter(0.01f, 0.1f, 1.0f);
    Vector3D negVec(-10.0f, -20.0f, -30.0f);
    Vector3D negResult1 = negFilter.Filter(negVec);
    Vector3D negResult2 = negFilter.Filter(negVec);

    TEST_ASSERT_TRUE(std::isfinite(negResult1.X));
    TEST_ASSERT_TRUE(std::isfinite(negResult2.X));
    // Should track toward negative values
    TEST_ASSERT_TRUE(negResult2.X < negResult1.X);

    // Test with very large values
    VectorKalmanFilter largeFilter(0.01f, 0.1f, 1.0f);
    Vector3D largeVec(1000.0f, 2000.0f, 3000.0f);
    Vector3D largeResult = largeFilter.Filter(largeVec);

    TEST_ASSERT_TRUE(std::isfinite(largeResult.X));
    TEST_ASSERT_TRUE(std::isfinite(largeResult.Y));
    TEST_ASSERT_TRUE(std::isfinite(largeResult.Z));

    // Test with very small values
    VectorKalmanFilter tinyFilter(0.01f, 0.1f, 1.0f);
    Vector3D tinyVec(0.001f, 0.002f, 0.003f);
    Vector3D tinyResult = tinyFilter.Filter(tinyVec);

    TEST_ASSERT_TRUE(std::isfinite(tinyResult.X));
    TEST_ASSERT_TRUE(std::isfinite(tinyResult.Y));
    TEST_ASSERT_TRUE(std::isfinite(tinyResult.Z));

    // Test convergence with constant input
    VectorKalmanFilter convFilter(0.01f, 0.1f, 1.0f);
    Vector3D target(50.0f, 75.0f, 100.0f);
    Vector3D lastResult;

    for (int i = 0; i < 50; i++) {
        lastResult = convFilter.Filter(target);
        TEST_ASSERT_TRUE(std::isfinite(lastResult.X));
    }

    // After many iterations, should be closer to target
    TEST_ASSERT_TRUE(std::abs(lastResult.X - target.X) < 20.0f);
    TEST_ASSERT_TRUE(std::abs(lastResult.Y - target.Y) < 20.0f);
    TEST_ASSERT_TRUE(std::abs(lastResult.Z - target.Z) < 20.0f);

    // Test with rapidly changing vectors
    VectorKalmanFilter changeFilter(0.1f, 0.5f, 1.0f);
    Vector3D v1(10.0f, 10.0f, 10.0f);
    Vector3D v2(20.0f, 5.0f, 15.0f);
    Vector3D v3(-5.0f, 25.0f, 0.0f);

    Vector3D r1 = changeFilter.Filter(v1);
    Vector3D r2 = changeFilter.Filter(v2);
    Vector3D r3 = changeFilter.Filter(v3);

    TEST_ASSERT_TRUE(std::isfinite(r1.X));
    TEST_ASSERT_TRUE(std::isfinite(r2.X));
    TEST_ASSERT_TRUE(std::isfinite(r3.X));

    // Test reset with extreme values
    VectorKalmanFilter resetFilter(0.01f, 0.1f, 1.0f);
    resetFilter.Filter(Vector3D(100.0f, 100.0f, 100.0f));

    Vector3D resetVec(-50.0f, -50.0f, -50.0f);
    resetFilter.Reset(resetVec, 10.0f);

    Vector3D afterReset = resetFilter.Filter(Vector3D(0.0f, 0.0f, 0.0f));
    // Should be influenced by reset estimation
    TEST_ASSERT_TRUE(std::isfinite(afterReset.X));

    // Test with high noise parameters
    VectorKalmanFilter highNoiseFilter(10.0f, 10.0f, 100.0f);
    Vector3D highNoiseResult = highNoiseFilter.Filter(Vector3D(5.0f, 5.0f, 5.0f));
    TEST_ASSERT_TRUE(std::isfinite(highNoiseResult.X));

    // Test with very low noise parameters
    VectorKalmanFilter lowNoiseFilter(0.0001f, 0.0001f, 0.01f);
    Vector3D lowNoiseResult = lowNoiseFilter.Filter(Vector3D(5.0f, 5.0f, 5.0f));
    TEST_ASSERT_TRUE(std::isfinite(lowNoiseResult.X));
}

void TestVectorKalmanFilter::RunAllTests() {
    RUN_TEST(TestParameterizedConstructor);
    RUN_TEST(TestFilter);
    RUN_TEST(TestReset);

    RUN_TEST(TestDefaultConstructor);
    RUN_TEST(TestEdgeCases);
}
