#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>
#include "../../registry/reflect_macros.hpp"

#include <mutex>

// Interleaved complex buffer layout: data[2*i] = Re, data[2*i+1] = Im.
// The FFT size must be a power of two and at least two.

class FFT {
public:
    /**
     * @brief Retrieve (or lazily create) a cached FFT instance for a given size.
     * @param fftSize Transform size (power of two, >= 2).
     * @return Reference to a cached FFT instance.
     * @throws std::invalid_argument if @p fftSize is not a supported power of two.
     */
    static FFT& Instance(int fftSize);

    /**
     * @brief Construct an FFT instance for the requested size.
     * @param fftSize Transform size (power of two, >= 2).
     * @throws std::invalid_argument if @p fftSize is not a supported power of two.
     */
    explicit FFT(int fftSize);

    /** @brief Size of the transform (number of complex samples). */
    int Size() const noexcept { return size_; }

    /**
     * @brief In-place Cooley–Tukey radix-2 FFT (complex -> complex).
     * @param data Interleaved complex buffer of length 2*Size().
     */
    void Forward(float* data) const;

    /**
     * @brief In-place inverse FFT (complex -> complex).
     * @param data  Interleaved complex buffer of length 2*Size().
     * @param scale If true, divide the result by Size().
     */
    void Inverse(float* data, bool scale = true) const;

    /**
     * @brief Compute magnitudes from an interleaved complex buffer.
     * @param complexData Input array of length 2*Size().
     * @param magnitude   Output array of length Size().
     */
    void ComplexMagnitude(const float* complexData, float* magnitude) const;

    /** @brief Validate that @p fftSize is a power of two and >= 2. */
    static bool IsValidSize(int fftSize) noexcept;

private:
    int size_ = 0;
    int bitCount_ = 0;
    mutable std::once_flag tablesInitFlag_;
    mutable std::vector<float> cosTable_;
    mutable std::vector<float> sinTable_;
    mutable std::vector<uint32_t> bitrevLUT_;

    void EnsureTables() const;
    void InitializeTables() const;
    void BitReverseOrder(float* data) const;

    static int ComputeBitCount(int size);

    PTX_BEGIN_FIELDS(FFT)
        /* No reflected fields. */
    PTX_END_FIELDS

    PTX_BEGIN_METHODS(FFT)
        PTX_SMETHOD_AUTO(FFT::Instance, "Instance"),
        PTX_METHOD_AUTO(FFT, Size, "Size"),
        PTX_METHOD_AUTO(FFT, Forward, "Forward"),
        PTX_METHOD_AUTO(FFT, Inverse, "Inverse"),
        PTX_METHOD_AUTO(FFT, ComplexMagnitude, "Complex magnitude"),
        PTX_SMETHOD_AUTO(FFT::IsValidSize, "Is valid size")
    PTX_END_METHODS

    PTX_BEGIN_DESCRIBE(FFT)
        PTX_CTOR(FFT, int)
    PTX_END_DESCRIBE(FFT)

};
