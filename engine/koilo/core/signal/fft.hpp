// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <cstdint>
#include <vector>
#include <koilo/registry/reflect_macros.hpp>
#include <mutex>



namespace koilo {

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

    KL_BEGIN_FIELDS(FFT)
        /* No reflected fields. */
    KL_END_FIELDS

    KL_BEGIN_METHODS(FFT)
        KL_SMETHOD_AUTO(FFT::Instance, "Instance"),
        KL_METHOD_AUTO(FFT, Size, "Size"),
        KL_METHOD_AUTO(FFT, Forward, "Forward"),
        KL_METHOD_AUTO(FFT, Inverse, "Inverse"),
        KL_METHOD_AUTO(FFT, ComplexMagnitude, "Complex magnitude"),
        KL_SMETHOD_AUTO(FFT::IsValidSize, "Is valid size")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(FFT)
        KL_CTOR(FFT, int)
    KL_END_DESCRIBE(FFT)

};

} // namespace koilo
