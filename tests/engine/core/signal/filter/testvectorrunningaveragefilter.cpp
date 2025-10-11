/**
 * @file testvectorrunningaveragefilter.cpp
 * @brief Implementation of VectorRunningAverageFilter unit tests.
 */

#include "testvectorrunningaveragefilter.hpp"

// ========== Constructor Tests ==========

void TestVectorRunningAverageFilter::TestParameterizedConstructor() {
    // Test with various memory sizes and gains
    VectorRunningAverageFilter filter1(5, 0.1f);
    TEST_ASSERT_EQUAL_size_t(5, filter1.GetCapacity());

    VectorRunningAverageFilter filter2(10, 0.5f);
    TEST_ASSERT_EQUAL_size_t(10, filter2.GetCapacity());

    VectorRunningAverageFilter filter3(20, 1.0f);
    TEST_ASSERT_EQUAL_size_t(20, filter3.GetCapacity());
}

// ========== Method Tests ==========

void TestVectorRunningAverageFilter::TestFilter() {
    VectorRunningAverageFilter filter(5, 0.2f);

    // Test basic filtering operation
    Vector3D input(10.0f, 20.0f, 30.0f);
    Vector3D result1 = filter.Filter(input);

    // Each component should be between 0 and input value
    TEST_ASSERT_TRUE(result1.X > 0.0f && result1.X <= input.X);
    TEST_ASSERT_TRUE(result1.Y > 0.0f && result1.Y <= input.Y);
    TEST_ASSERT_TRUE(result1.Z > 0.0f && result1.Z <= input.Z);

    // Subsequent values should converge towards input
    Vector3D result2 = filter.Filter(input);
    TEST_ASSERT_TRUE(result2.X >= result1.X);
    TEST_ASSERT_TRUE(result2.Y >= result1.Y);
    TEST_ASSERT_TRUE(result2.Z >= result1.Z);
}

void TestVectorRunningAverageFilter::TestSetGain() {
    VectorRunningAverageFilter filter(5, 0.1f);

    // Change gain and verify it affects filtering behavior
    filter.SetGain(0.8f);

    Vector3D input(100.0f, 100.0f, 100.0f);
    Vector3D result = filter.Filter(input);

    // Higher gain should respond faster
    TEST_ASSERT_TRUE(result.X > 50.0f);
    TEST_ASSERT_TRUE(result.Y > 50.0f);
    TEST_ASSERT_TRUE(result.Z > 50.0f);
}

void TestVectorRunningAverageFilter::TestReset() {
    VectorRunningAverageFilter filter(5, 0.2f);

    // Feed some values
    Vector3D input(10.0f, 20.0f, 30.0f);
    for (int i = 0; i < 10; i++) {
        filter.Filter(input);
    }

    // Reset and verify behavior returns to initial state
    filter.Reset();
    Vector3D newInput(40.0f, 50.0f, 60.0f);
    Vector3D firstResult = filter.Filter(newInput);

    // After reset, first filter call should behave like a new filter
    VectorRunningAverageFilter freshFilter(5, 0.2f);
    Vector3D freshResult = freshFilter.Filter(newInput);

    TEST_ASSERT_VECTOR3D_WITHIN(0.1f, freshResult, firstResult);
}

void TestVectorRunningAverageFilter::TestGetCapacity() {
    VectorRunningAverageFilter filter1(5, 0.1f);
    TEST_ASSERT_EQUAL_size_t(5, filter1.GetCapacity());

    VectorRunningAverageFilter filter2(100, 0.1f);
    TEST_ASSERT_EQUAL_size_t(100, filter2.GetCapacity());
}

// ========== Vector Filtering Tests ==========

// ========== Edge Cases ==========

void TestVectorRunningAverageFilter::TestEdgeCases() {
    // Test with very low gain
    VectorRunningAverageFilter filter1(10, 0.01f);
    Vector3D input(100.0f, 100.0f, 100.0f);
    Vector3D result = filter1.Filter(input);

    // With very low gain, response should be very slow
    TEST_ASSERT_TRUE(result.X < 10.0f);
    TEST_ASSERT_TRUE(result.Y < 10.0f);
    TEST_ASSERT_TRUE(result.Z < 10.0f);

    // Test with high gain
    VectorRunningAverageFilter filter2(10, 0.95f);
    result = filter2.Filter(input);
    result = filter2.Filter(input);

    // With high gain, should respond quickly
    TEST_ASSERT_TRUE(result.X > 80.0f);
    TEST_ASSERT_TRUE(result.Y > 80.0f);
    TEST_ASSERT_TRUE(result.Z > 80.0f);
}

// ========== Test Runner ==========

void TestVectorRunningAverageFilter::TestDefaultConstructor() {
    // TODO: Implement test for default constructor
    // VectorRunningAverageFilter obj; // Requires constructor parameters
    TEST_ASSERT_TRUE(false);  // Not implemented
}

void TestVectorRunningAverageFilter::RunAllTests() {
    RUN_TEST(TestParameterizedConstructor);
    RUN_TEST(TestFilter);
    RUN_TEST(TestSetGain);
    RUN_TEST(TestReset);
    RUN_TEST(TestGetCapacity);

    RUN_TEST(TestEdgeCases);

    RUN_TEST(TestDefaultConstructor);
}
