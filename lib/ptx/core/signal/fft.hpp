#pragma once

#include <cstdint>
#include <cmath>
#include "../../registry/reflect_macros.hpp"

// Interleaved complex buffer layout: data[2*i] = Re, data[2*i+1] = Im
// fftSize must be a power of two.

template<int fftSize>
class FFT {
    static_assert(fftSize >= 2, "fftSize must be >= 2");
    static_assert((fftSize & (fftSize - 1)) == 0, "fftSize must be a power of 2");

public:
    // In-place Cooley-Tukey radix-2 DIT
    static void Forward(float* data);                  // complex -> complex
    static void Inverse(float* data, bool scale=true); // complex -> complex

    // Magnitude from interleaved complex
    static void ComplexMagnitude(const float* complexData, float* magnitude);

    // Utility
    static constexpr int Size() { return fftSize; }

private:
    // Bit utilities
    static constexpr int BitCount();
    static void BitReverseOrder(float* data);

    // Tables (computed once per template instantiation)
    static void EnsureTables(); // twiddles + bitrev LUT

    // Twiddles for k = 0..fftSize/2-1 : e^{-i 2π k / N} = cos - i sin
    static float* CosTable();
    static float* SinTable();

    // Bit-reverse LUT for indices 0..N-1
    static uint16_t* BitrevLUT();

    /* NOTE: FFT is a template; verify macros accept template types. */
    PTX_BEGIN_FIELDS(FFT)
        /* TODO: PTX_FIELD(FFT, member, "Doc", min, max) */
    PTX_END_FIELDS
    
    PTX_BEGIN_METHODS(FFT)
        PTX_SMETHOD_AUTO(FFT::Forward, "Forward"),
        PTX_SMETHOD_AUTO(FFT::Inverse, "Inverse"),
        PTX_SMETHOD_AUTO(FFT::ComplexMagnitude, "Complex magnitude"),
        PTX_SMETHOD_AUTO(FFT::Size, "Size")
    PTX_END_METHODS
    
    PTX_BEGIN_DESCRIBE(FFT)
        /* TODO: PTX_CTOR0(FFT) or PTX_CTOR(FFT, ...) */
    PTX_END_DESCRIBE(FFT)
    
};
