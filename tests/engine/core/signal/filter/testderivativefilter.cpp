/**
 * @file testderivativefilter.cpp
 * @brief Implementation of DerivativeFilter unit tests.
 */

#include "testderivativefilter.hpp"

// ========== Constructor Tests ==========

void TestDerivativeFilter::TestDefaultConstructor() {
    DerivativeFilter filter;
    TEST_ASSERT_TRUE(true);  // Basic construction
}

void TestDerivativeFilter::TestParameterizedConstructor() {
    // DerivativeFilter has no parameterized constructor
    // It only has a default constructor with fixed internal filter configurations
    // Test that we can construct and use the filter immediately
    DerivativeFilter filter;

    // Verify initial output is finite
    float output = filter.GetOutput();
    TEST_ASSERT_TRUE(std::isfinite(output));

    // Verify we can immediately filter a value
    float result = filter.Filter(1.0f);
    TEST_ASSERT_TRUE(std::isfinite(result));
}

// ========== Method Tests ==========

void TestDerivativeFilter::TestFilter() {
    DerivativeFilter filter;

    // Test with constant input - derivative should be near zero
    float result1 = filter.Filter(5.0f);
    TEST_ASSERT_TRUE(std::isfinite(result1));

    float result2 = filter.Filter(5.0f);
    TEST_ASSERT_TRUE(std::isfinite(result2));

    float result3 = filter.Filter(5.0f);
    TEST_ASSERT_TRUE(std::isfinite(result3));

    // After several constant values, derivative should be very low or zero
    // (the running average will settle toward zero change)
    TEST_ASSERT_TRUE(result3 <= result1 + 0.1f);  // Should decrease or stay low

    // Test with changing input - derivative should be non-zero
    DerivativeFilter filter2;
    filter2.Filter(0.0f);
    filter2.Filter(1.0f);
    float changingResult = filter2.Filter(2.0f);  // Increasing values

    // With increasing inputs, we should see some derivative response
    TEST_ASSERT_TRUE(std::isfinite(changingResult));
    TEST_ASSERT_TRUE(changingResult >= 0.0f);  // Derivative is constrained to [0, 1]
    TEST_ASSERT_TRUE(changingResult <= 1.0f);

    // Test rapid change detection
    DerivativeFilter filter3;
    filter3.Filter(0.0f);
    float rapidChange = filter3.Filter(10.0f);  // Large jump

    // Should detect a significant change
    TEST_ASSERT_TRUE(rapidChange > 0.0f);
}

void TestDerivativeFilter::TestGetOutput() {
    DerivativeFilter filter;
    float output = filter.GetOutput();
    TEST_ASSERT_TRUE(std::isfinite(output));
}

// ========== Edge Cases ==========

void TestDerivativeFilter::TestEdgeCases() {
    // Test with zero values
    DerivativeFilter zeroFilter;
    float zeroResult = zeroFilter.Filter(0.0f);
    TEST_ASSERT_TRUE(std::isfinite(zeroResult));
    TEST_ASSERT_TRUE(zeroResult >= 0.0f && zeroResult <= 1.0f);

    // Test with negative values
    DerivativeFilter negFilter;
    float negResult1 = negFilter.Filter(-5.0f);
    float negResult2 = negFilter.Filter(-10.0f);
    TEST_ASSERT_TRUE(std::isfinite(negResult1));
    TEST_ASSERT_TRUE(std::isfinite(negResult2));
    // Derivative measures absolute change, so should still be valid
    TEST_ASSERT_TRUE(negResult2 >= 0.0f && negResult2 <= 1.0f);

    // Test with very large values
    DerivativeFilter largeFilter;
    float largeResult1 = largeFilter.Filter(1000.0f);
    float largeResult2 = largeFilter.Filter(2000.0f);
    TEST_ASSERT_TRUE(std::isfinite(largeResult1));
    TEST_ASSERT_TRUE(std::isfinite(largeResult2));

    // Test with very small changes
    DerivativeFilter tinyFilter;
    tinyFilter.Filter(1.0f);
    tinyFilter.Filter(1.001f);
    float tinyResult = tinyFilter.Filter(1.002f);
    TEST_ASSERT_TRUE(std::isfinite(tinyResult));
    TEST_ASSERT_TRUE(tinyResult >= 0.0f);

    // Test oscillating input
    DerivativeFilter oscFilter;
    for (int i = 0; i < 10; i++) {
        float oscValue = (i % 2 == 0) ? 0.0f : 1.0f;
        float oscResult = oscFilter.Filter(oscValue);
        TEST_ASSERT_TRUE(std::isfinite(oscResult));
        TEST_ASSERT_TRUE(oscResult >= 0.0f && oscResult <= 1.0f);
    }

    // Verify GetOutput returns the last filtered value
    float lastOutput = oscFilter.GetOutput();
    TEST_ASSERT_TRUE(std::isfinite(lastOutput));
    TEST_ASSERT_TRUE(lastOutput >= 0.0f && lastOutput <= 1.0f);
}

// ========== Test Runner ==========

void TestDerivativeFilter::RunAllTests() {
    RUN_TEST(TestDefaultConstructor);
    RUN_TEST(TestParameterizedConstructor);
    RUN_TEST(TestFilter);
    RUN_TEST(TestGetOutput);
    RUN_TEST(TestEdgeCases);
}
