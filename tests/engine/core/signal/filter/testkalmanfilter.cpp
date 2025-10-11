/**
 * @file testkalmanfilter.cpp
 * @brief Implementation of KalmanFilter unit tests.
 */

#include "testkalmanfilter.hpp"
#include <cmath>

// ========== Constructor Tests ==========

void TestKalmanFilter::TestParameterizedConstructor() {
    // Test construction with specific parameters
    float processNoise = 0.01f;
    float sensorNoise = 0.1f;
    float errorCovariance = 1.0f;

    KalmanFilter filter(processNoise, sensorNoise, errorCovariance);

    // Verify parameters are stored correctly
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, processNoise, filter.GetProcessNoise());
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, sensorNoise, filter.GetSensorNoise());
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, errorCovariance, filter.GetErrorCovariance());

    // Initial estimation should be 0
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.0f, filter.GetEstimation());
}

// ========== Method Tests ==========

void TestKalmanFilter::TestReset() {
    KalmanFilter filter(0.01f, 0.1f, 1.0f);

    // Apply some filtering to change state
    filter.Filter(10.0f);
    filter.Filter(15.0f);

    // Reset to new values
    filter.Reset(5.0f, 0.5f);

    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 5.0f, filter.GetEstimation());
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.5f, filter.GetErrorCovariance());
}

void TestKalmanFilter::TestFilter() {
    KalmanFilter filter(0.01f, 0.1f, 1.0f);

    // Filter a constant value
    float measurement = 10.0f;
    float result = filter.Filter(measurement);

    // Result should be between 0 (initial estimate) and measurement
    TEST_ASSERT_TRUE(result > 0.0f);
    TEST_ASSERT_TRUE(result < measurement);

    // Filter again with same value, should get closer
    float result2 = filter.Filter(measurement);
    TEST_ASSERT_TRUE(result2 > result);
    TEST_ASSERT_TRUE(result2 <= measurement);
}

void TestKalmanFilter::TestGetEstimation() {
    KalmanFilter filter(0.01f, 0.1f, 1.0f);

    // Initial estimation should be 0
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.0f, filter.GetEstimation());

    // After filtering, estimation should match returned value
    float filtered = filter.Filter(5.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, filtered, filter.GetEstimation());
}

void TestKalmanFilter::TestGetProcessNoise() {
    float processNoise = 0.025f;
    KalmanFilter filter(processNoise, 0.1f, 1.0f);

    TEST_ASSERT_FLOAT_WITHIN(0.0001f, processNoise, filter.GetProcessNoise());
}

void TestKalmanFilter::TestGetSensorNoise() {
    float sensorNoise = 0.15f;
    KalmanFilter filter(0.01f, sensorNoise, 1.0f);

    TEST_ASSERT_FLOAT_WITHIN(0.0001f, sensorNoise, filter.GetSensorNoise());
}

void TestKalmanFilter::TestGetErrorCovariance() {
    float errorCovariance = 2.5f;
    KalmanFilter filter(0.01f, 0.1f, errorCovariance);

    TEST_ASSERT_FLOAT_WITHIN(0.0001f, errorCovariance, filter.GetErrorCovariance());
}

void TestKalmanFilter::TestSetProcessNoise() {
    KalmanFilter filter(0.01f, 0.1f, 1.0f);

    float newProcessNoise = 0.05f;
    filter.SetProcessNoise(newProcessNoise);

    TEST_ASSERT_FLOAT_WITHIN(0.0001f, newProcessNoise, filter.GetProcessNoise());
}

void TestKalmanFilter::TestSetSensorNoise() {
    KalmanFilter filter(0.01f, 0.1f, 1.0f);

    float newSensorNoise = 0.2f;
    filter.SetSensorNoise(newSensorNoise);

    TEST_ASSERT_FLOAT_WITHIN(0.0001f, newSensorNoise, filter.GetSensorNoise());
}

void TestKalmanFilter::TestSetErrorCovariance() {
    KalmanFilter filter(0.01f, 0.1f, 1.0f);

    float newErrorCovariance = 3.0f;
    filter.SetErrorCovariance(newErrorCovariance);

    TEST_ASSERT_FLOAT_WITHIN(0.0001f, newErrorCovariance, filter.GetErrorCovariance());
}

// ========== Edge Cases ==========

// ========== Test Runner ==========

void TestKalmanFilter::TestDefaultConstructor() {
    // KalmanFilter has no default constructor - it requires three parameters
    // Test that we can construct with minimal/default-like parameters
    KalmanFilter filter(0.01f, 0.1f, 1.0f);

    // Verify the filter is in a valid initial state
    TEST_ASSERT_TRUE(std::isfinite(filter.GetEstimation()));
    TEST_ASSERT_TRUE(std::isfinite(filter.GetProcessNoise()));
    TEST_ASSERT_TRUE(std::isfinite(filter.GetSensorNoise()));
    TEST_ASSERT_TRUE(std::isfinite(filter.GetErrorCovariance()));

    // Initial estimation should be 0
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.0f, filter.GetEstimation());
}

void TestKalmanFilter::TestEdgeCases() {
    // Test with very low noise values
    KalmanFilter lowNoiseFilter(0.0001f, 0.0001f, 0.1f);
    float lowNoiseResult = lowNoiseFilter.Filter(10.0f);
    TEST_ASSERT_TRUE(std::isfinite(lowNoiseResult));

    // Test with high noise values
    KalmanFilter highNoiseFilter(1.0f, 1.0f, 10.0f);
    float highNoiseResult = highNoiseFilter.Filter(10.0f);
    TEST_ASSERT_TRUE(std::isfinite(highNoiseResult));

    // Test with zero measurement
    KalmanFilter zeroFilter(0.01f, 0.1f, 1.0f);
    float zeroResult = zeroFilter.Filter(0.0f);
    TEST_ASSERT_TRUE(std::isfinite(zeroResult));

    // Test with negative measurements (Kalman filter should handle any float)
    KalmanFilter negFilter(0.01f, 0.1f, 1.0f);
    float negResult1 = negFilter.Filter(-10.0f);
    float negResult2 = negFilter.Filter(-15.0f);
    TEST_ASSERT_TRUE(std::isfinite(negResult1));
    TEST_ASSERT_TRUE(std::isfinite(negResult2));
    // Estimation should track toward negative values
    TEST_ASSERT_TRUE(negResult2 < 0.0f);

    // Test with very large values
    KalmanFilter largeFilter(0.01f, 0.1f, 1.0f);
    float largeResult = largeFilter.Filter(1000.0f);
    TEST_ASSERT_TRUE(std::isfinite(largeResult));

    // Test convergence with noisy data around a constant
    KalmanFilter convFilter(0.01f, 0.5f, 1.0f);
    float target = 50.0f;
    float lastEstimate = 0.0f;

    // Simulate noisy measurements around target value
    for (int i = 0; i < 50; i++) {
        // Add synthetic noise (simple alternating pattern)
        float noise = (i % 2 == 0) ? 1.0f : -1.0f;
        float noisyMeasurement = target + noise;
        lastEstimate = convFilter.Filter(noisyMeasurement);
        TEST_ASSERT_TRUE(std::isfinite(lastEstimate));
    }

    // After many iterations, estimate should be closer to target than initial
    TEST_ASSERT_TRUE(std::abs(lastEstimate - target) < target);

    // Test reset functionality with edge values
    convFilter.Reset(0.0f, 0.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.0f, convFilter.GetEstimation());
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.0f, convFilter.GetErrorCovariance());

    // Test parameter setters with extreme values
    KalmanFilter paramFilter(0.01f, 0.1f, 1.0f);
    paramFilter.SetProcessNoise(100.0f);
    paramFilter.SetSensorNoise(100.0f);
    paramFilter.SetErrorCovariance(100.0f);

    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 100.0f, paramFilter.GetProcessNoise());
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 100.0f, paramFilter.GetSensorNoise());
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 100.0f, paramFilter.GetErrorCovariance());

    // Filter should still work with these extreme parameters
    float extremeResult = paramFilter.Filter(5.0f);
    TEST_ASSERT_TRUE(std::isfinite(extremeResult));
}

void TestKalmanFilter::RunAllTests() {
    RUN_TEST(TestParameterizedConstructor);
    RUN_TEST(TestReset);
    RUN_TEST(TestFilter);
    RUN_TEST(TestGetEstimation);
    RUN_TEST(TestGetProcessNoise);
    RUN_TEST(TestGetSensorNoise);
    RUN_TEST(TestGetErrorCovariance);
    RUN_TEST(TestSetProcessNoise);
    RUN_TEST(TestSetSensorNoise);
    RUN_TEST(TestSetErrorCovariance);

    RUN_TEST(TestDefaultConstructor);
    RUN_TEST(TestEdgeCases);
}
