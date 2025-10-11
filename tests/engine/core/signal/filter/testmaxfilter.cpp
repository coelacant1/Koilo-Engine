/**
 * @file testmaxfilter.cpp
 * @brief Implementation of MaxFilter unit tests.
 */

#include "testmaxfilter.hpp"

// ========== Constructor Tests ==========

void TestMaxFilter::TestDefaultConstructor() {
    MaxFilter filter;  // Default is 40
    TEST_ASSERT_EQUAL_size_t(40, filter.GetCapacity());
}

void TestMaxFilter::TestParameterizedConstructor() {
    MaxFilter filter1(5);
    TEST_ASSERT_EQUAL_size_t(5, filter1.GetCapacity());

    MaxFilter filter2(10);
    TEST_ASSERT_EQUAL_size_t(10, filter2.GetCapacity());

    MaxFilter filter3(100);
    TEST_ASSERT_EQUAL_size_t(100, filter3.GetCapacity());
}

// ========== Method Tests ==========

void TestMaxFilter::TestFilter() {
    MaxFilter filter(5);

    // First value sets the max
    float result1 = filter.Filter(10.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 10.0f, result1);

    // Smaller value doesn't change max
    float result2 = filter.Filter(5.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 10.0f, result2);

    // Larger value updates max
    float result3 = filter.Filter(20.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 20.0f, result3);

    // Smaller value still returns current max
    float result4 = filter.Filter(15.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 20.0f, result4);
}

void TestMaxFilter::TestReset() {
    MaxFilter filter(5);

    // Build up some max values
    filter.Filter(10.0f);
    filter.Filter(20.0f);
    filter.Filter(15.0f);
    float beforeReset = filter.Filter(12.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 20.0f, beforeReset);

    // Reset to zeros
    filter.Reset();

    // After reset, new value should become the max
    float afterReset = filter.Filter(5.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 5.0f, afterReset);
}

void TestMaxFilter::TestGetCapacity() {
    MaxFilter filter1(10);
    TEST_ASSERT_EQUAL_size_t(10, filter1.GetCapacity());

    MaxFilter filter2(50);
    TEST_ASSERT_EQUAL_size_t(50, filter2.GetCapacity());
}

// ========== Max Filter Behavior Tests ==========

// ========== Edge Cases ==========

void TestMaxFilter::TestEdgeCases() {
    MaxFilter filter(5);

    // Test with zero
    float result1 = filter.Filter(0.0f);

    // Test with same values
    filter.Filter(25.0f);
    filter.Filter(25.0f);
    filter.Filter(25.0f);
    float result2 = filter.Filter(25.0f);

    // Test with very small differences
    filter.Reset();
    filter.Filter(10.0f);
    filter.Filter(10.001f);
    float result3 = filter.Filter(10.0005f);
}

// ========== Test Runner ==========

void TestMaxFilter::RunAllTests() {
    RUN_TEST(TestDefaultConstructor);
    RUN_TEST(TestParameterizedConstructor);
    RUN_TEST(TestFilter);
    RUN_TEST(TestReset);
    RUN_TEST(TestGetCapacity);

    RUN_TEST(TestEdgeCases);

}
