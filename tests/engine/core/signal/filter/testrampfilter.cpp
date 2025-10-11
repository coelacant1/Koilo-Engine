/**
 * @file testrampfilter.cpp
 * @brief Implementation of RampFilter unit tests.
 */

#include "testrampfilter.hpp"
#include <cmath>

// ========== Constructor Tests ==========

void TestRampFilter::TestDefaultConstructor() {
    RampFilter filter;
    
    // Filter should be created successfully
    float result = filter.Filter(10.0f);
    TEST_ASSERT_TRUE(std::isfinite(result));
}

void TestRampFilter::TestParameterizedConstructor() {
    // Test with specific frame count
    RampFilter filter1(10);
    float result1 = filter1.Filter(10.0f);
    TEST_ASSERT_TRUE(std::isfinite(result1));
    
    // Test with custom epsilon
    RampFilter filter2(20, 0.001f);
    float result2 = filter2.Filter(5.0f);
    TEST_ASSERT_TRUE(std::isfinite(result2));
}

// ========== Method Tests ==========

void TestRampFilter::TestFilter() {
    RampFilter filter(10);
    
    // Start at 0, ramp to 100
    float target = 100.0f;
    float previous = 0.0f;
    
    // Apply filter multiple times
    for (int i = 0; i < 15; i++) {
        float current = filter.Filter(target);
        TEST_ASSERT_TRUE(std::isfinite(current));
        
        // Value should increase towards target
        if (i > 0) {
            TEST_ASSERT_TRUE(current >= previous || std::abs(current - target) < 1.0f);
        }
        previous = current;
    }
    
    // After enough iterations, should be close to target
    TEST_ASSERT_FLOAT_WITHIN(15.0f, target, previous);
}

void TestRampFilter::TestSetIncrement() {
    RampFilter filter;
    
    // Set a large increment for fast transitions
    filter.SetIncrement(10.0f);
    
    float result1 = filter.Filter(100.0f);
    TEST_ASSERT_TRUE(std::isfinite(result1));
    
    // Set a small increment for slow transitions
    filter.SetIncrement(0.1f);
    
    float result2 = filter.Filter(100.0f);
    TEST_ASSERT_TRUE(std::isfinite(result2));
}

void TestRampFilter::TestSetFrames() {
    RampFilter filter;
    
    // Set fast transition (few frames)
    filter.SetFrames(5);
    
    for (int i = 0; i < 10; i++) {
        filter.Filter(50.0f);
    }
    
    float result1 = filter.Filter(50.0f);
    TEST_ASSERT_TRUE(std::isfinite(result1));
    
    // Set slow transition (many frames)
    filter.SetFrames(100);
    
    for (int i = 0; i < 10; i++) {
        filter.Filter(0.0f);
    }
    
    float result2 = filter.Filter(0.0f);
    TEST_ASSERT_TRUE(std::isfinite(result2));
}

// ========== Ramp Behavior Tests ==========

// ========== Edge Cases ==========

void TestRampFilter::TestEdgeCases() {
    RampFilter filter(10);
    
    // Test with zero
    float result1 = filter.Filter(0.0f);
    TEST_ASSERT_TRUE(std::isfinite(result1));
    TEST_ASSERT_TRUE(result1 >= 0.0f);
    
    // Test with negative values
    float result2 = filter.Filter(-50.0f);
    TEST_ASSERT_TRUE(std::isfinite(result2));
    
    // Test with large values
    float result3 = filter.Filter(10000.0f);
    TEST_ASSERT_TRUE(std::isfinite(result3));
}

// ========== Test Runner ==========

void TestRampFilter::RunAllTests() {
    RUN_TEST(TestDefaultConstructor);
    RUN_TEST(TestParameterizedConstructor);
    RUN_TEST(TestFilter);
    RUN_TEST(TestSetIncrement);
    RUN_TEST(TestSetFrames);

    RUN_TEST(TestEdgeCases);

}
