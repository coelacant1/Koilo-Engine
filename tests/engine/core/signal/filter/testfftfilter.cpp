/**
 * @file testfftfilter.cpp
 * @brief Implementation of FFTFilter unit tests.
 */

#include "testfftfilter.hpp"

// ========== Constructor Tests ==========

void TestFFTFilter::TestDefaultConstructor() {
    FFTFilter filter;

    // Verify filter can be constructed
    float output = filter.GetOutput();
    TEST_ASSERT_TRUE(std::isfinite(output));

    // Initial output should be constrained to [0, 1]
    TEST_ASSERT_TRUE(output >= 0.0f && output <= 1.0f);
}

void TestFFTFilter::TestParameterizedConstructor() {
    // FFTFilter has no parameterized constructor
    // It only has a default constructor with fixed internal filter configurations
    // Test that we can construct and use the filter immediately
    FFTFilter filter;

    // Verify we can immediately filter a value
    float result = filter.Filter(0.5f);
    TEST_ASSERT_TRUE(std::isfinite(result));
    TEST_ASSERT_TRUE(result >= 0.0f && result <= 1.0f);
}

// ========== Method Tests ==========

void TestFFTFilter::TestFilter() {
    FFTFilter filter;

    // Test with positive values
    float result1 = filter.Filter(0.5f);
    TEST_ASSERT_TRUE(std::isfinite(result1));
    TEST_ASSERT_TRUE(result1 >= 0.0f && result1 <= 1.0f);

    // Test with sequence of values
    float result2 = filter.Filter(0.7f);
    float result3 = filter.Filter(0.6f);
    float result4 = filter.Filter(0.8f);

    // All results should be constrained to [0, 1]
    TEST_ASSERT_TRUE(std::isfinite(result2));
    TEST_ASSERT_TRUE(std::isfinite(result3));
    TEST_ASSERT_TRUE(std::isfinite(result4));
    TEST_ASSERT_TRUE(result2 >= 0.0f && result2 <= 1.0f);
    TEST_ASSERT_TRUE(result3 >= 0.0f && result3 <= 1.0f);
    TEST_ASSERT_TRUE(result4 >= 0.0f && result4 <= 1.0f);

    // Test with large FFT value
    FFTFilter filter2;
    float largeResult = filter2.Filter(10.0f);
    TEST_ASSERT_TRUE(std::isfinite(largeResult));
    // Should be clamped to 1.0 due to constraint
    TEST_ASSERT_TRUE(largeResult >= 0.0f && largeResult <= 1.0f);

    // Test filter takes absolute value (negative inputs become positive)
    FFTFilter filter3;
    float negResult = filter3.Filter(-0.5f);
    TEST_ASSERT_TRUE(std::isfinite(negResult));
    TEST_ASSERT_TRUE(negResult >= 0.0f && negResult <= 1.0f);
}

void TestFFTFilter::TestGetOutput() {
    FFTFilter filter;

    // Initial output
    float initialOutput = filter.GetOutput();
    TEST_ASSERT_TRUE(std::isfinite(initialOutput));
    TEST_ASSERT_TRUE(initialOutput >= 0.0f && initialOutput <= 1.0f);

    // After filtering, GetOutput should return the last filtered value
    float filtered = filter.Filter(0.5f);
    float output = filter.GetOutput();

    TEST_ASSERT_FLOAT_WITHIN(0.0001f, filtered, output);

    // Filter another value and verify GetOutput updates
    float filtered2 = filter.Filter(0.8f);
    float output2 = filter.GetOutput();

    TEST_ASSERT_FLOAT_WITHIN(0.0001f, filtered2, output2);
}

// ========== Edge Cases ==========

void TestFFTFilter::TestEdgeCases() {
    // Test with zero
    FFTFilter zeroFilter;
    float zeroResult = zeroFilter.Filter(0.0f);
    TEST_ASSERT_TRUE(std::isfinite(zeroResult));
    TEST_ASSERT_TRUE(zeroResult >= 0.0f && zeroResult <= 1.0f);

    // Test with very small values
    FFTFilter tinyFilter;
    float tinyResult = tinyFilter.Filter(0.001f);
    TEST_ASSERT_TRUE(std::isfinite(tinyResult));
    TEST_ASSERT_TRUE(tinyResult >= 0.0f && tinyResult <= 1.0f);

    // Test with very large values (should clamp to 1.0)
    FFTFilter largeFilter;
    float largeResult1 = largeFilter.Filter(100.0f);
    float largeResult2 = largeFilter.Filter(1000.0f);
    TEST_ASSERT_TRUE(std::isfinite(largeResult1));
    TEST_ASSERT_TRUE(std::isfinite(largeResult2));
    TEST_ASSERT_TRUE(largeResult1 >= 0.0f && largeResult1 <= 1.0f);
    TEST_ASSERT_TRUE(largeResult2 >= 0.0f && largeResult2 <= 1.0f);

    // Test with negative values (should convert to absolute value)
    FFTFilter negFilter;
    float negResult1 = negFilter.Filter(-0.5f);
    float negResult2 = negFilter.Filter(-1.5f);
    TEST_ASSERT_TRUE(std::isfinite(negResult1));
    TEST_ASSERT_TRUE(std::isfinite(negResult2));
    TEST_ASSERT_TRUE(negResult1 >= 0.0f && negResult1 <= 1.0f);
    TEST_ASSERT_TRUE(negResult2 >= 0.0f && negResult2 <= 1.0f);

    // Test with constant input - baseline should adapt
    FFTFilter constFilter;
    for (int i = 0; i < 30; i++) {
        float result = constFilter.Filter(0.5f);
        TEST_ASSERT_TRUE(std::isfinite(result));
        TEST_ASSERT_TRUE(result >= 0.0f && result <= 1.0f);
    }

    // Test with alternating values
    FFTFilter altFilter;
    for (int i = 0; i < 20; i++) {
        float value = (i % 2 == 0) ? 0.3f : 0.7f;
        float result = altFilter.Filter(value);
        TEST_ASSERT_TRUE(std::isfinite(result));
        TEST_ASSERT_TRUE(result >= 0.0f && result <= 1.0f);
    }

    // Verify final output is accessible
    float finalOutput = altFilter.GetOutput();
    TEST_ASSERT_TRUE(std::isfinite(finalOutput));
    TEST_ASSERT_TRUE(finalOutput >= 0.0f && finalOutput <= 1.0f);
}

// ========== Test Runner ==========

void TestFFTFilter::RunAllTests() {
    RUN_TEST(TestDefaultConstructor);
    RUN_TEST(TestParameterizedConstructor);
    RUN_TEST(TestFilter);
    RUN_TEST(TestGetOutput);
    RUN_TEST(TestEdgeCases);
}
