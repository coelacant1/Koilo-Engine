/**
 * @file testfft.cpp
 * @brief Implementation of FFT unit tests.
 */

#include "testfft.hpp"

// ========== Constructor Tests ==========

void TestFFT::TestDefaultConstructor() {
    // FFT has no default constructor - it requires an fftSize parameter
    // Test that we can construct with a valid power-of-2 size
    FFT fft(8);  // Minimal practical FFT size

    // Verify the FFT was created with correct size
    TEST_ASSERT_EQUAL(8, fft.Size());

    // Test with other valid sizes
    FFT fft16(16);
    TEST_ASSERT_EQUAL(16, fft16.Size());

    FFT fft64(64);
    TEST_ASSERT_EQUAL(64, fft64.Size());
}

// ========== Method Tests ==========
void TestFFT::TestSize() {
    FFT fft8(8);
    TEST_ASSERT_EQUAL(8, fft8.Size());

    FFT fft32(32);
    TEST_ASSERT_EQUAL(32, fft32.Size());

    FFT fft128(128);
    TEST_ASSERT_EQUAL(128, fft128.Size());

    FFT fft256(256);
    TEST_ASSERT_EQUAL(256, fft256.Size());
}

void TestFFT::TestForward() {
    FFT fft(8);

    // Create a test signal: interleaved complex data [Re0, Im0, Re1, Im1, ...]
    // Simple test: all real values, no imaginary component
    float data[16] = {1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f,
                      1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f};

    // Perform forward FFT
    fft.Forward(data);

    // After FFT, all values should be finite
    for (int i = 0; i < 16; i++) {
        TEST_ASSERT_TRUE(std::isfinite(data[i]));
    }

    // DC component (data[0]) should be the sum of inputs (8.0f for constant signal)
    TEST_ASSERT_FLOAT_WITHIN(1.0f, 8.0f, data[0]);
}

void TestFFT::TestInverse() {
    FFT fft(8);

    // Create test data
    float data[16] = {1.0f, 0.0f, 2.0f, 0.0f, 3.0f, 0.0f, 4.0f, 0.0f,
                      5.0f, 0.0f, 6.0f, 0.0f, 7.0f, 0.0f, 8.0f, 0.0f};

    // Store original for comparison
    float original[16];
    for (int i = 0; i < 16; i++) {
        original[i] = data[i];
    }

    // Forward then inverse should approximately recover the original
    fft.Forward(data);
    fft.Inverse(data, true);  // With scaling

    // After round-trip, values should be close to original
    for (int i = 0; i < 16; i++) {
        TEST_ASSERT_TRUE(std::isfinite(data[i]));
        TEST_ASSERT_FLOAT_WITHIN(0.1f, original[i], data[i]);
    }
}

void TestFFT::TestComplexMagnitude() {
    FFT fft(8);

    // Complex data with known magnitudes
    float complexData[16] = {
        3.0f, 4.0f,  // Magnitude = 5.0
        1.0f, 0.0f,  // Magnitude = 1.0
        0.0f, 1.0f,  // Magnitude = 1.0
        -3.0f, -4.0f, // Magnitude = 5.0
        5.0f, 12.0f, // Magnitude = 13.0
        0.0f, 0.0f,  // Magnitude = 0.0
        1.0f, 1.0f,  // Magnitude = sqrt(2) ≈ 1.414
        2.0f, -2.0f  // Magnitude = sqrt(8) ≈ 2.828
    };

    float magnitude[8];
    fft.ComplexMagnitude(complexData, magnitude);

    // Verify magnitudes
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 5.0f, magnitude[0]);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, magnitude[1]);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, magnitude[2]);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 5.0f, magnitude[3]);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 13.0f, magnitude[4]);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, magnitude[5]);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.414f, magnitude[6]);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 2.828f, magnitude[7]);

    // All magnitudes should be non-negative and finite
    for (int i = 0; i < 8; i++) {
        TEST_ASSERT_TRUE(std::isfinite(magnitude[i]));
        TEST_ASSERT_TRUE(magnitude[i] >= 0.0f);
    }
}
// ========== Edge Cases ==========

// ========== Test Runner ==========

void TestFFT::TestParameterizedConstructor() {
    // Test construction with various power-of-2 sizes
    FFT fft2(2);
    TEST_ASSERT_EQUAL(2, fft2.Size());

    FFT fft4(4);
    TEST_ASSERT_EQUAL(4, fft4.Size());

    FFT fft16(16);
    TEST_ASSERT_EQUAL(16, fft16.Size());

    FFT fft512(512);
    TEST_ASSERT_EQUAL(512, fft512.Size());

    FFT fft1024(1024);
    TEST_ASSERT_EQUAL(1024, fft1024.Size());

    // Verify IsValidSize static method
    TEST_ASSERT_TRUE(FFT::IsValidSize(2));
    TEST_ASSERT_TRUE(FFT::IsValidSize(4));
    TEST_ASSERT_TRUE(FFT::IsValidSize(8));
    TEST_ASSERT_TRUE(FFT::IsValidSize(16));
    TEST_ASSERT_TRUE(FFT::IsValidSize(512));
    TEST_ASSERT_TRUE(FFT::IsValidSize(1024));

    // Non-power-of-2 sizes should be invalid
    TEST_ASSERT_FALSE(FFT::IsValidSize(3));
    TEST_ASSERT_FALSE(FFT::IsValidSize(5));
    TEST_ASSERT_FALSE(FFT::IsValidSize(10));
    TEST_ASSERT_FALSE(FFT::IsValidSize(100));

    // Zero and one should be invalid
    TEST_ASSERT_FALSE(FFT::IsValidSize(0));
    TEST_ASSERT_FALSE(FFT::IsValidSize(1));
}

void TestFFT::TestEdgeCases() {
    // Test with smallest valid size (2)
    FFT fft2(2);
    float data2[4] = {1.0f, 0.0f, 2.0f, 0.0f};
    fft2.Forward(data2);

    for (int i = 0; i < 4; i++) {
        TEST_ASSERT_TRUE(std::isfinite(data2[i]));
    }

    // Test with zero data
    FFT fft8(8);
    float zeroData[16] = {0.0f};
    fft8.Forward(zeroData);

    // All outputs should be zero (or very close)
    for (int i = 0; i < 16; i++) {
        TEST_ASSERT_TRUE(std::isfinite(zeroData[i]));
        TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.0f, zeroData[i]);
    }

    // Test with very large values
    FFT fft16(16);
    float largeData[32];
    for (int i = 0; i < 32; i++) {
        largeData[i] = (i % 2 == 0) ? 1000.0f : 0.0f;
    }
    fft16.Forward(largeData);

    for (int i = 0; i < 32; i++) {
        TEST_ASSERT_TRUE(std::isfinite(largeData[i]));
    }

    // Test with negative values
    FFT fft8neg(8);
    float negData[16] = {-1.0f, 0.0f, -2.0f, 0.0f, -3.0f, 0.0f, -4.0f, 0.0f,
                         -5.0f, 0.0f, -6.0f, 0.0f, -7.0f, 0.0f, -8.0f, 0.0f};
    fft8neg.Forward(negData);

    for (int i = 0; i < 16; i++) {
        TEST_ASSERT_TRUE(std::isfinite(negData[i]));
    }

    // Test ComplexMagnitude with zero data
    float zeroComplex[16] = {0.0f};
    float zeroMag[8];
    fft8.ComplexMagnitude(zeroComplex, zeroMag);

    for (int i = 0; i < 8; i++) {
        TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.0f, zeroMag[i]);
    }

    // Test inverse without scaling
    FFT fftNoScale(8);
    float dataNoScale[16] = {1.0f, 0.0f, 2.0f, 0.0f, 3.0f, 0.0f, 4.0f, 0.0f,
                              5.0f, 0.0f, 6.0f, 0.0f, 7.0f, 0.0f, 8.0f, 0.0f};

    fftNoScale.Forward(dataNoScale);
    fftNoScale.Inverse(dataNoScale, false);  // Without scaling

    // Without scaling, values will be multiplied by size (8)
    for (int i = 0; i < 16; i++) {
        TEST_ASSERT_TRUE(std::isfinite(dataNoScale[i]));
    }

    // Test Instance caching
    FFT& instance1 = FFT::Instance(32);
    FFT& instance2 = FFT::Instance(32);

    // Both should refer to the same cached instance
    TEST_ASSERT_EQUAL(32, instance1.Size());
    TEST_ASSERT_EQUAL(32, instance2.Size());
    TEST_ASSERT_EQUAL(&instance1, &instance2);  // Same address
}

void TestFFT::RunAllTests() {
    RUN_TEST(TestDefaultConstructor);
    RUN_TEST(TestParameterizedConstructor);
    RUN_TEST(TestSize);
    RUN_TEST(TestForward);
    RUN_TEST(TestInverse);
    RUN_TEST(TestComplexMagnitude);
    RUN_TEST(TestEdgeCases);
}
