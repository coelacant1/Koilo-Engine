/**
 * @file testrunningaveragefilter.cpp
 * @brief Implementation of RunningAverageFilter unit tests.
 */

#include "testrunningaveragefilter.hpp"
#include <cmath>

// ========== Constructor Tests ==========

void TestRunningAverageFilter::TestParameterizedConstructor() {
    // Test with various memory sizes and gains
    RunningAverageFilter filter1(5, 0.1f);
    TEST_ASSERT_EQUAL_size_t(5, filter1.GetCapacity());
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.1f, filter1.GetGain());

    RunningAverageFilter filter2(10, 0.5f);
    TEST_ASSERT_EQUAL_size_t(10, filter2.GetCapacity());
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.5f, filter2.GetGain());

    RunningAverageFilter filter3(20, 1.0f);
    TEST_ASSERT_EQUAL_size_t(20, filter3.GetCapacity());
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 1.0f, filter3.GetGain());
}

// ========== Method Tests ==========

void TestRunningAverageFilter::TestSetGain() {
    RunningAverageFilter filter(5, 0.1f);

    // Test gain modification
    filter.SetGain(0.5f);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.5f, filter.GetGain());

    filter.SetGain(0.2f);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.2f, filter.GetGain());

    // Test with extreme gain values
    filter.SetGain(0.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.0f, filter.GetGain());

    filter.SetGain(1.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 1.0f, filter.GetGain());
}

void TestRunningAverageFilter::TestFilter() {
    RunningAverageFilter filter(5, 0.1f);

    // Test basic filtering operation
    float result1 = filter.Filter(10.0f);
    TEST_ASSERT_TRUE(result1 > 0.0f && result1 <= 10.0f);

    // Subsequent values should converge towards input
    float result2 = filter.Filter(10.0f);
    TEST_ASSERT_TRUE(result2 >= result1);

    float result3 = filter.Filter(10.0f);
    TEST_ASSERT_TRUE(result3 >= result2);
}

void TestRunningAverageFilter::TestReset() {
    RunningAverageFilter filter(5, 0.1f);

    // Feed some values
    for (int i = 0; i < 10; i++) {
        filter.Filter(10.0f);
    }

    // Reset and verify behavior returns to initial state
    filter.Reset();
    float firstResult = filter.Filter(20.0f);

    // After reset, first filter call should behave like a new filter
    RunningAverageFilter freshFilter(5, 0.1f);
    float freshResult = freshFilter.Filter(20.0f);

    TEST_ASSERT_FLOAT_WITHIN(0.01f, freshResult, firstResult);
}

void TestRunningAverageFilter::TestGetCapacity() {
    RunningAverageFilter filter1(5, 0.1f);
    TEST_ASSERT_EQUAL_size_t(5, filter1.GetCapacity());

    RunningAverageFilter filter2(100, 0.1f);
    TEST_ASSERT_EQUAL_size_t(100, filter2.GetCapacity());
}

void TestRunningAverageFilter::TestGetGain() {
    RunningAverageFilter filter(5, 0.25f);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.25f, filter.GetGain());

    filter.SetGain(0.75f);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.75f, filter.GetGain());
}

// ========== Signal Processing Tests ==========

// ========== Edge Cases ==========

void TestRunningAverageFilter::TestEdgeCases() {
    // Test with zero gain (should heavily smooth)
    RunningAverageFilter filter1(10, 0.0f);
    float result = filter1.Filter(100.0f);
    // With zero gain, response should be very slow
    TEST_ASSERT_TRUE(result < 10.0f);

    // Test with gain of 1.0 (minimal smoothing)
    RunningAverageFilter filter2(10, 1.0f);
    result = filter2.Filter(100.0f);
    result = filter2.Filter(100.0f);
    // With gain of 1.0, should respond quickly
    TEST_ASSERT_TRUE(result > 50.0f);
}

// ========== Test Runner ==========

void TestRunningAverageFilter::TestDefaultConstructor() {
    // RunningAverageFilter has no default constructor - it requires memory and gain parameters
    // Test that we can construct with minimal/default-like parameters
    RunningAverageFilter filter(10, 0.1f);

    // Verify the filter is in a valid initial state
    TEST_ASSERT_EQUAL_size_t(10, filter.GetCapacity());
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.1f, filter.GetGain());

    // Should be able to filter immediately
    float result = filter.Filter(5.0f);
    TEST_ASSERT_TRUE(std::isfinite(result));
    TEST_ASSERT_TRUE(result >= 0.0f);
}

void TestRunningAverageFilter::RunAllTests() {
    RUN_TEST(TestParameterizedConstructor);
    RUN_TEST(TestSetGain);
    RUN_TEST(TestFilter);
    RUN_TEST(TestReset);
    RUN_TEST(TestGetCapacity);
    RUN_TEST(TestGetGain);

    RUN_TEST(TestEdgeCases);

    RUN_TEST(TestDefaultConstructor);
}
