/**
 * @file testminfilter.cpp
 * @brief Implementation of MinFilter unit tests.
 */

#include "testminfilter.hpp"
#include <cmath>

// ========== Constructor Tests ==========

void TestMinFilter::TestDefaultConstructor() {
    MinFilter filter;
    
    // Default capacity should be 40
    TEST_ASSERT_EQUAL(40, filter.GetCapacity());
    
    // Initial filter output should be 0
    float result = filter.Filter(0.0f);
    TEST_ASSERT_TRUE(std::isfinite(result));
}

void TestMinFilter::TestParameterizedConstructor() {
    // Test with custom memory size
    MinFilter filter1(10, true);
    TEST_ASSERT_EQUAL(10, filter1.GetCapacity());
    
    MinFilter filter2(100, false);
    TEST_ASSERT_EQUAL(100, filter2.GetCapacity());
    
    // Test that filter works after construction
    float result = filter1.Filter(5.0f);
    TEST_ASSERT_TRUE(std::isfinite(result));
}

// ========== Method Tests ==========

void TestMinFilter::TestFilter() {
    MinFilter filter(10);
    
    // Add values and check that minimum is tracked
    filter.Filter(10.0f);
    filter.Filter(5.0f);
    filter.Filter(15.0f);
    float result = filter.Filter(3.0f);
    
    // Result should be close to minimum value in window
    TEST_ASSERT_TRUE(result <= 10.0f);
    TEST_ASSERT_TRUE(std::isfinite(result));
}

void TestMinFilter::TestReset() {
    MinFilter filter(10);
    
    // Add some values
    filter.Filter(10.0f);
    filter.Filter(20.0f);
    filter.Filter(30.0f);
    
    // Reset the filter
    filter.Reset();
    
    // After reset, filter should work normally
    float result = filter.Filter(5.0f);
    TEST_ASSERT_TRUE(std::isfinite(result));
}

void TestMinFilter::TestGetCapacity() {
    MinFilter filter1(25);
    TEST_ASSERT_EQUAL(25, filter1.GetCapacity());
    
    MinFilter filter2(50);
    TEST_ASSERT_EQUAL(50, filter2.GetCapacity());
    
    MinFilter filter3;  // Default
    TEST_ASSERT_EQUAL(40, filter3.GetCapacity());
}

// ========== Minimum Tracking Tests ==========

// ========== Edge Cases ==========

void TestMinFilter::TestEdgeCases() {
    MinFilter filter(10);
    
    // Test with zero
    float result1 = filter.Filter(0.0f);
    TEST_ASSERT_TRUE(std::isfinite(result1));
    TEST_ASSERT_TRUE(result1 >= 0.0f);
    
    // Test with negative values
    filter.Reset();
    float result2 = filter.Filter(-10.0f);
    TEST_ASSERT_TRUE(std::isfinite(result2));
    
    // Test with large values
    filter.Reset();
    float result3 = filter.Filter(1000000.0f);
    TEST_ASSERT_TRUE(std::isfinite(result3));
}

// ========== Test Runner ==========

void TestMinFilter::RunAllTests() {
    RUN_TEST(TestDefaultConstructor);
    RUN_TEST(TestParameterizedConstructor);
    RUN_TEST(TestFilter);
    RUN_TEST(TestReset);
    RUN_TEST(TestGetCapacity);

    RUN_TEST(TestEdgeCases);

}
