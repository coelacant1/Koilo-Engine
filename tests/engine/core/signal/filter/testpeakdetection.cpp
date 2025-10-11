/**
 * @file testpeakdetection.cpp
 * @brief Implementation of PeakDetection unit tests.
 */

#include "testpeakdetection.hpp"
#include <vector>

// ========== Constructor Tests ==========

void TestPeakDetection::TestDefaultConstructor() {
    // PeakDetection requires sampleSize parameter
    PeakDetection detector(100);
    
    TEST_ASSERT_EQUAL(100, detector.GetSampleSize());
    TEST_ASSERT_EQUAL(12, detector.GetLag());
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.75f, detector.GetThreshold());
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.5f, detector.GetInfluence());
}

void TestPeakDetection::TestParameterizedConstructor() {
    // Test with custom parameters
    PeakDetection detector1(50, 10, 1.0f, 0.3f);
    TEST_ASSERT_EQUAL(50, detector1.GetSampleSize());
    TEST_ASSERT_EQUAL(10, detector1.GetLag());
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, detector1.GetThreshold());
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.3f, detector1.GetInfluence());
    
    PeakDetection detector2(200, 20, 0.5f, 0.8f);
    TEST_ASSERT_EQUAL(200, detector2.GetSampleSize());
    TEST_ASSERT_EQUAL(20, detector2.GetLag());
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.5f, detector2.GetThreshold());
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.8f, detector2.GetInfluence());
}

// ========== Method Tests ==========

void TestPeakDetection::TestCalculate() {
    PeakDetection detector(20);
    
    // Create test data with a clear peak
    std::vector<float> data(20, 10.0f);
    data[10] = 50.0f;  // Peak in the middle
    
    std::vector<bool> peaks;
    detector.Calculate(data.data(), peaks);
    
    // Should detect the peak
    TEST_ASSERT_EQUAL(20, peaks.size());
    TEST_ASSERT_TRUE(peaks[10] == true);
}

void TestPeakDetection::TestReset() {
    PeakDetection detector(20);
    
    // Calculate once
    std::vector<float> data1(20, 10.0f);
    std::vector<bool> peaks1;
    detector.Calculate(data1.data(), peaks1);
    
    // Reset
    detector.Reset();
    
    // Calculate again - should work normally
    std::vector<float> data2(20, 15.0f);
    std::vector<bool> peaks2;
    detector.Calculate(data2.data(), peaks2);
    
    TEST_ASSERT_EQUAL(20, peaks2.size());
}

void TestPeakDetection::TestGetSampleSize() {
    PeakDetection detector1(50);
    TEST_ASSERT_EQUAL(50, detector1.GetSampleSize());
    
    PeakDetection detector2(100);
    TEST_ASSERT_EQUAL(100, detector2.GetSampleSize());
    
    PeakDetection detector3(200);
    TEST_ASSERT_EQUAL(200, detector3.GetSampleSize());
}

void TestPeakDetection::TestGetLag() {
    PeakDetection detector1(100, 5);
    TEST_ASSERT_EQUAL(5, detector1.GetLag());
    
    PeakDetection detector2(100, 15);
    TEST_ASSERT_EQUAL(15, detector2.GetLag());
    
    PeakDetection detector3(100);  // Default
    TEST_ASSERT_EQUAL(12, detector3.GetLag());
}

void TestPeakDetection::TestGetThreshold() {
    PeakDetection detector1(100, 12, 1.5f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.5f, detector1.GetThreshold());
    
    PeakDetection detector2(100, 12, 0.5f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.5f, detector2.GetThreshold());
    
    PeakDetection detector3(100);  // Default
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.75f, detector3.GetThreshold());
}

void TestPeakDetection::TestGetInfluence() {
    PeakDetection detector1(100, 12, 0.75f, 0.2f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.2f, detector1.GetInfluence());
    
    PeakDetection detector2(100, 12, 0.75f, 0.9f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.9f, detector2.GetInfluence());
    
    PeakDetection detector3(100);  // Default
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.5f, detector3.GetInfluence());
}

// ========== Peak Detection Tests ==========

// ========== Edge Cases ==========

void TestPeakDetection::TestEdgeCases() {
    PeakDetection detector(20);
    
    // Test with all zeros
    std::vector<float> zeros(20, 0.0f);
    std::vector<bool> peaks1;
    detector.Calculate(zeros.data(), peaks1);
    TEST_ASSERT_EQUAL(20, peaks1.size());
    
    // Test with negative values
    detector.Reset();
    std::vector<float> negatives(20, -5.0f);
    negatives[10] = 5.0f;
    std::vector<bool> peaks2;
    detector.Calculate(negatives.data(), peaks2);
    TEST_ASSERT_EQUAL(20, peaks2.size());
}

// ========== Test Runner ==========

void TestPeakDetection::RunAllTests() {
    RUN_TEST(TestDefaultConstructor);
    RUN_TEST(TestParameterizedConstructor);
    RUN_TEST(TestCalculate);
    RUN_TEST(TestReset);
    RUN_TEST(TestGetSampleSize);
    RUN_TEST(TestGetLag);
    RUN_TEST(TestGetThreshold);
    RUN_TEST(TestGetInfluence);

    RUN_TEST(TestEdgeCases);

}
