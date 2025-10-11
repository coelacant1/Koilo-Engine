/**
 * @file testquaternionkalmanfilter.cpp
 * @brief Implementation of QuaternionKalmanFilter unit tests.
 */

#include "testquaternionkalmanfilter.hpp"
#include <cmath>

// ========== Constructor Tests ==========

void TestQuaternionKalmanFilter::TestDefaultConstructor() {
    QuaternionKalmanFilter filter;

    // Default constructor should create a valid filter
    // Test by filtering a quaternion
    Quaternion input(1.0f, 0.0f, 0.0f, 0.0f);
    Quaternion result = filter.Filter(input);

    // Result should be valid and finite
    TEST_ASSERT_TRUE(std::isfinite(result.W));
    TEST_ASSERT_TRUE(std::isfinite(result.X));
    TEST_ASSERT_TRUE(std::isfinite(result.Y));
    TEST_ASSERT_TRUE(std::isfinite(result.Z));
}

void TestQuaternionKalmanFilter::TestParameterizedConstructor() {
    // Test construction with specific parameters
    float gain = 0.5f;
    int memory = 10;

    QuaternionKalmanFilter filter(gain, memory);

    // Filter a quaternion to verify it's operational
    Quaternion input(1.0f, 0.0f, 0.0f, 0.0f);
    Quaternion result = filter.Filter(input);

    // Result should be valid
    TEST_ASSERT_TRUE(std::isfinite(result.W));
    TEST_ASSERT_TRUE(std::isfinite(result.X));
    TEST_ASSERT_TRUE(std::isfinite(result.Y));
    TEST_ASSERT_TRUE(std::isfinite(result.Z));
}

// ========== Method Tests ==========

void TestQuaternionKalmanFilter::TestFilter() {
    QuaternionKalmanFilter filter(0.3f, 5);

    // Filter identity quaternion
    Quaternion identity(1.0f, 0.0f, 0.0f, 0.0f);
    Quaternion result = filter.Filter(identity);

    // First result should be the identity (or close to it)
    TEST_ASSERT_FLOAT_WITHIN(0.1f, identity.W, result.W);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, identity.X, result.X);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, identity.Y, result.Y);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, identity.Z, result.Z);

    // Filter same quaternion again
    Quaternion result2 = filter.Filter(identity);

    // Should still be close to identity
    TEST_ASSERT_FLOAT_WITHIN(0.1f, identity.W, result2.W);
}

// ========== Quaternion Filter Behavior Tests ==========

// ========== Edge Cases ==========

// ========== Test Runner ==========

void TestQuaternionKalmanFilter::TestEdgeCases() {
    // Test with zero quaternion components
    QuaternionKalmanFilter zeroFilter(0.3f, 5);
    Quaternion zeroInput(0.0f, 0.0f, 0.0f, 0.0f);
    Quaternion zeroResult = zeroFilter.Filter(zeroInput);

    // Should handle zero quaternion gracefully
    TEST_ASSERT_TRUE(std::isfinite(zeroResult.W));
    TEST_ASSERT_TRUE(std::isfinite(zeroResult.X));
    TEST_ASSERT_TRUE(std::isfinite(zeroResult.Y));
    TEST_ASSERT_TRUE(std::isfinite(zeroResult.Z));

    // Test with very small gain (heavy filtering)
    QuaternionKalmanFilter smallGainFilter(0.01f, 20);
    Quaternion input1(1.0f, 0.0f, 0.0f, 0.0f);
    Quaternion result1 = smallGainFilter.Filter(input1);
    TEST_ASSERT_TRUE(std::isfinite(result1.W));

    // Test with large gain (minimal filtering)
    QuaternionKalmanFilter largeGainFilter(0.99f, 2);
    Quaternion input2(0.707f, 0.707f, 0.0f, 0.0f);
    Quaternion result2 = largeGainFilter.Filter(input2);
    TEST_ASSERT_TRUE(std::isfinite(result2.W));
    TEST_ASSERT_TRUE(std::isfinite(result2.X));

    // Test with negative quaternion components (valid quaternion)
    QuaternionKalmanFilter negFilter(0.3f, 5);
    Quaternion negInput(-0.5f, -0.5f, 0.5f, 0.5f);
    Quaternion negResult = negFilter.Filter(negInput);
    TEST_ASSERT_TRUE(std::isfinite(negResult.W));
    TEST_ASSERT_TRUE(std::isfinite(negResult.X));
    TEST_ASSERT_TRUE(std::isfinite(negResult.Y));
    TEST_ASSERT_TRUE(std::isfinite(negResult.Z));

    // Test sequence of similar quaternions (should converge)
    QuaternionKalmanFilter seqFilter(0.2f, 10);
    Quaternion target(0.866f, 0.5f, 0.0f, 0.0f);  // 60 degree rotation around X
    Quaternion lastResult;

    for (int i = 0; i < 15; i++) {
        lastResult = seqFilter.Filter(target);
        TEST_ASSERT_TRUE(std::isfinite(lastResult.W));
        TEST_ASSERT_TRUE(std::isfinite(lastResult.X));
    }

    // After many iterations, should be close to target
    TEST_ASSERT_FLOAT_WITHIN(0.2f, target.W, lastResult.W);

    // Test with rapidly changing quaternions
    QuaternionKalmanFilter changeFilter(0.5f, 5);
    Quaternion q1(1.0f, 0.0f, 0.0f, 0.0f);
    Quaternion q2(0.707f, 0.707f, 0.0f, 0.0f);
    Quaternion q3(0.0f, 1.0f, 0.0f, 0.0f);

    Quaternion r1 = changeFilter.Filter(q1);
    Quaternion r2 = changeFilter.Filter(q2);
    Quaternion r3 = changeFilter.Filter(q3);

    // All results should be finite
    TEST_ASSERT_TRUE(std::isfinite(r1.W));
    TEST_ASSERT_TRUE(std::isfinite(r2.W));
    TEST_ASSERT_TRUE(std::isfinite(r3.W));

    // Test with very small memory (minimal history)
    QuaternionKalmanFilter minMemFilter(0.3f, 1);
    Quaternion memResult = minMemFilter.Filter(q1);
    TEST_ASSERT_TRUE(std::isfinite(memResult.W));

    // Test with large memory (long history)
    QuaternionKalmanFilter maxMemFilter(0.3f, 100);
    Quaternion maxMemResult = maxMemFilter.Filter(q1);
    TEST_ASSERT_TRUE(std::isfinite(maxMemResult.W));
}

void TestQuaternionKalmanFilter::RunAllTests() {
    RUN_TEST(TestDefaultConstructor);
    RUN_TEST(TestParameterizedConstructor);
    RUN_TEST(TestFilter);

    RUN_TEST(TestEdgeCases);
}
