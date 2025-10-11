/**
 * @file testfftvoicedetection.cpp
 * @brief Implementation of FFTVoiceDetection unit tests.
 */

#include "testfftvoicedetection.hpp"

// ========== Constructor Tests ==========

void TestFFTVoiceDetection::TestDefaultConstructor() {
    // FFTVoiceDetection has no default constructor - use default parameter values
    FFTVoiceDetection detector;  // Uses default parameters (64 peaks, bandwidth 5)

    // Verify the detector is in a valid initial state
    TEST_ASSERT_EQUAL_size_t(64, detector.GetPeakCount());

    // Initial viseme ratios should all be zero
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.0f, detector.GetViseme(Viseme::EE));
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.0f, detector.GetViseme(Viseme::AE));
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.0f, detector.GetViseme(Viseme::UH));
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.0f, detector.GetViseme(Viseme::AR));
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.0f, detector.GetViseme(Viseme::ER));
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.0f, detector.GetViseme(Viseme::AH));
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.0f, detector.GetViseme(Viseme::OO));
}

// ========== Method Tests ==========
void TestFFTVoiceDetection::TestSetThreshold() {
    FFTVoiceDetection detector;

    // Set different threshold values
    detector.SetThreshold(500.0f);
    // We can't directly get the threshold, but we can verify the method doesn't crash
    TEST_ASSERT_TRUE(true);

    detector.SetThreshold(100.0f);
    TEST_ASSERT_TRUE(true);

    detector.SetThreshold(1000.0f);
    TEST_ASSERT_TRUE(true);

    // Test with zero threshold
    detector.SetThreshold(0.0f);
    TEST_ASSERT_TRUE(true);
}

void TestFFTVoiceDetection::TestGetViseme() {
    FFTVoiceDetection detector;

    // Test getting all viseme types
    float eeRatio = detector.GetViseme(Viseme::EE);
    float aeRatio = detector.GetViseme(Viseme::AE);
    float uhRatio = detector.GetViseme(Viseme::UH);
    float arRatio = detector.GetViseme(Viseme::AR);
    float erRatio = detector.GetViseme(Viseme::ER);
    float ahRatio = detector.GetViseme(Viseme::AH);
    float ooRatio = detector.GetViseme(Viseme::OO);

    // All should be valid floats initially at zero
    TEST_ASSERT_TRUE(std::isfinite(eeRatio));
    TEST_ASSERT_TRUE(std::isfinite(aeRatio));
    TEST_ASSERT_TRUE(std::isfinite(uhRatio));
    TEST_ASSERT_TRUE(std::isfinite(arRatio));
    TEST_ASSERT_TRUE(std::isfinite(erRatio));
    TEST_ASSERT_TRUE(std::isfinite(ahRatio));
    TEST_ASSERT_TRUE(std::isfinite(ooRatio));

    // All ratios should be in valid range [0, 1] or at least non-negative
    TEST_ASSERT_TRUE(eeRatio >= 0.0f);
    TEST_ASSERT_TRUE(aeRatio >= 0.0f);
    TEST_ASSERT_TRUE(uhRatio >= 0.0f);
    TEST_ASSERT_TRUE(arRatio >= 0.0f);
    TEST_ASSERT_TRUE(erRatio >= 0.0f);
    TEST_ASSERT_TRUE(ahRatio >= 0.0f);
    TEST_ASSERT_TRUE(ooRatio >= 0.0f);
}

void TestFFTVoiceDetection::TestToString() {
    FFTVoiceDetection detector;

    ptx::UString result = detector.ToString();

    // Verify that the string is not empty
    TEST_ASSERT_FALSE(result.IsEmpty());

    // Verify we can get the C string
    const char* str = result.CStr();
    TEST_ASSERT_TRUE(str != nullptr);

    // String should have some content
    TEST_ASSERT_TRUE(result.Length() > 0);
}

void TestFFTVoiceDetection::TestResetVisemes() {
    FFTVoiceDetection detector;

    // Create some FFT peak data
    std::vector<float> peaks(64, 0.5f);
    detector.Update(peaks.data(), 8000.0f);

    // After update, some visemes might be non-zero
    // Reset them
    detector.ResetVisemes();

    // After reset, all visemes should be zero (or very close)
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.0f, detector.GetViseme(Viseme::EE));
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.0f, detector.GetViseme(Viseme::AE));
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.0f, detector.GetViseme(Viseme::UH));
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.0f, detector.GetViseme(Viseme::AR));
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.0f, detector.GetViseme(Viseme::ER));
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.0f, detector.GetViseme(Viseme::AH));
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.0f, detector.GetViseme(Viseme::OO));
}

void TestFFTVoiceDetection::TestUpdate() {
    FFTVoiceDetection detector;

    // Create test FFT peak data (64 peaks)
    std::vector<float> peaks(64);

    // Initialize with some varying peak data
    for (size_t i = 0; i < 64; i++) {
        peaks[i] = 0.1f + (i % 10) * 0.05f;
    }

    // Update with peak data and a typical max frequency
    detector.Update(peaks.data(), 8000.0f);

    // After update, at least one viseme should have been calculated
    // All viseme ratios should still be finite and non-negative
    TEST_ASSERT_TRUE(std::isfinite(detector.GetViseme(Viseme::EE)));
    TEST_ASSERT_TRUE(std::isfinite(detector.GetViseme(Viseme::AE)));
    TEST_ASSERT_TRUE(std::isfinite(detector.GetViseme(Viseme::UH)));
    TEST_ASSERT_TRUE(std::isfinite(detector.GetViseme(Viseme::AR)));
    TEST_ASSERT_TRUE(std::isfinite(detector.GetViseme(Viseme::ER)));
    TEST_ASSERT_TRUE(std::isfinite(detector.GetViseme(Viseme::AH)));
    TEST_ASSERT_TRUE(std::isfinite(detector.GetViseme(Viseme::OO)));

    // All ratios should be non-negative
    TEST_ASSERT_TRUE(detector.GetViseme(Viseme::EE) >= 0.0f);
    TEST_ASSERT_TRUE(detector.GetViseme(Viseme::AE) >= 0.0f);
    TEST_ASSERT_TRUE(detector.GetViseme(Viseme::UH) >= 0.0f);
    TEST_ASSERT_TRUE(detector.GetViseme(Viseme::AR) >= 0.0f);
    TEST_ASSERT_TRUE(detector.GetViseme(Viseme::ER) >= 0.0f);
    TEST_ASSERT_TRUE(detector.GetViseme(Viseme::AH) >= 0.0f);
    TEST_ASSERT_TRUE(detector.GetViseme(Viseme::OO) >= 0.0f);
}

void TestFFTVoiceDetection::TestGetPeakCount() {
    // Test with default peak count
    FFTVoiceDetection detector1;
    TEST_ASSERT_EQUAL_size_t(64, detector1.GetPeakCount());

    // Test with custom peak count
    FFTVoiceDetection detector2(128, 5);
    TEST_ASSERT_EQUAL_size_t(128, detector2.GetPeakCount());

    // Test with different peak count
    FFTVoiceDetection detector3(32, 10);
    TEST_ASSERT_EQUAL_size_t(32, detector3.GetPeakCount());
}
// ========== Edge Cases ==========

// ========== Test Runner ==========

void TestFFTVoiceDetection::TestParameterizedConstructor() {
    // Test with various peak counts and bandwidths
    FFTVoiceDetection detector1(32, 3);
    TEST_ASSERT_EQUAL_size_t(32, detector1.GetPeakCount());

    FFTVoiceDetection detector2(128, 10);
    TEST_ASSERT_EQUAL_size_t(128, detector2.GetPeakCount());

    FFTVoiceDetection detector3(256, 20);
    TEST_ASSERT_EQUAL_size_t(256, detector3.GetPeakCount());

    // Verify all detectors start with zero viseme ratios
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.0f, detector1.GetViseme(Viseme::EE));
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.0f, detector2.GetViseme(Viseme::AE));
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.0f, detector3.GetViseme(Viseme::UH));
}

void TestFFTVoiceDetection::TestEdgeCases() {
    // Test with zero peak data
    FFTVoiceDetection detector1;
    std::vector<float> zeroPeaks(64, 0.0f);
    detector1.Update(zeroPeaks.data(), 8000.0f);

    // Should handle zero data gracefully
    TEST_ASSERT_TRUE(std::isfinite(detector1.GetViseme(Viseme::EE)));
    TEST_ASSERT_TRUE(std::isfinite(detector1.GetViseme(Viseme::AE)));

    // Test with very small peak count
    FFTVoiceDetection detector2(8, 2);
    std::vector<float> smallPeaks(8, 0.2f);
    detector2.Update(smallPeaks.data(), 4000.0f);

    TEST_ASSERT_TRUE(std::isfinite(detector2.GetViseme(Viseme::UH)));
    TEST_ASSERT_TRUE(std::isfinite(detector2.GetViseme(Viseme::AR)));

    // Test with large peak count
    FFTVoiceDetection detector3(512, 15);
    std::vector<float> largePeaks(512);
    for (size_t i = 0; i < 512; i++) {
        largePeaks[i] = 0.1f + (i % 20) * 0.02f;
    }
    detector3.Update(largePeaks.data(), 22050.0f);

    TEST_ASSERT_TRUE(std::isfinite(detector3.GetViseme(Viseme::ER)));
    TEST_ASSERT_TRUE(std::isfinite(detector3.GetViseme(Viseme::AH)));

    // Test with very high peak values
    FFTVoiceDetection detector4;
    std::vector<float> highPeaks(64, 100.0f);
    detector4.Update(highPeaks.data(), 8000.0f);

    for (int i = 0; i < 7; i++) {
        TEST_ASSERT_TRUE(std::isfinite(detector4.GetViseme(static_cast<Viseme::MouthShape>(i))));
    }

    // Test multiple sequential updates
    FFTVoiceDetection detector5;
    for (int j = 0; j < 10; j++) {
        std::vector<float> seqPeaks(64);
        for (size_t i = 0; i < 64; i++) {
            seqPeaks[i] = 0.1f * j + 0.05f * (i % 5);
        }
        detector5.Update(seqPeaks.data(), 8000.0f);
    }

    // After multiple updates, visemes should still be valid
    TEST_ASSERT_TRUE(std::isfinite(detector5.GetViseme(Viseme::OO)));
    TEST_ASSERT_TRUE(std::isfinite(detector5.GetViseme(Viseme::EE)));

    // Test reset after multiple updates
    detector5.ResetVisemes();
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.0f, detector5.GetViseme(Viseme::EE));

    // Test with different frequency ranges
    FFTVoiceDetection detector6;
    std::vector<float> freqPeaks(64, 0.3f);

    detector6.Update(freqPeaks.data(), 4000.0f);   // Low frequency range
    TEST_ASSERT_TRUE(std::isfinite(detector6.GetViseme(Viseme::EE)));

    detector6.ResetVisemes();
    detector6.Update(freqPeaks.data(), 16000.0f);  // High frequency range
    TEST_ASSERT_TRUE(std::isfinite(detector6.GetViseme(Viseme::AE)));

    // Test ToString doesn't crash with various states
    ptx::UString str1 = detector1.ToString();
    ptx::UString str2 = detector6.ToString();
    TEST_ASSERT_FALSE(str1.IsEmpty());
    TEST_ASSERT_FALSE(str2.IsEmpty());

    // Test threshold extremes
    FFTVoiceDetection detector7;
    detector7.SetThreshold(0.0f);
    detector7.Update(freqPeaks.data(), 8000.0f);
    TEST_ASSERT_TRUE(std::isfinite(detector7.GetViseme(Viseme::UH)));

    detector7.SetThreshold(10000.0f);
    detector7.Update(freqPeaks.data(), 8000.0f);
    TEST_ASSERT_TRUE(std::isfinite(detector7.GetViseme(Viseme::AR)));
}

void TestFFTVoiceDetection::RunAllTests() {
    RUN_TEST(TestDefaultConstructor);
    RUN_TEST(TestParameterizedConstructor);
    RUN_TEST(TestSetThreshold);
    RUN_TEST(TestGetViseme);
    RUN_TEST(TestToString);
    RUN_TEST(TestResetVisemes);
    RUN_TEST(TestUpdate);
    RUN_TEST(TestGetPeakCount);
    RUN_TEST(TestEdgeCases);
}
