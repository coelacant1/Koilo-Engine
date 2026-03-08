#include <ptx/core/signal/fft.hpp>

#include <algorithm>
#include <cmath>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <unordered_map>

namespace {

constexpr float kTwoPi = 6.28318530717958647692f;

}  // namespace

FFT& FFT::Instance(int fftSize) {
    if (!IsValidSize(fftSize)) {
        throw std::invalid_argument("FFT::Instance: size must be >= 2 and a power of two");
    }

    static std::mutex mutex;
    static std::unordered_map<int, std::unique_ptr<FFT>> cache;

    std::lock_guard<std::mutex> lock(mutex);
    auto it = cache.find(fftSize);
    if (it == cache.end()) {
        auto fft = std::make_unique<FFT>(fftSize);
        it = cache.emplace(fftSize, std::move(fft)).first;
    }
    return *it->second;
}

FFT::FFT(int fftSize) {
    if (!IsValidSize(fftSize)) {
        throw std::invalid_argument("FFT::FFT: size must be >= 2 and a power of two");
    }

    size_ = fftSize;
    bitCount_ = ComputeBitCount(size_);
    cosTable_.resize(static_cast<std::size_t>(size_) / 2u, 0.0f);
    sinTable_.resize(static_cast<std::size_t>(size_) / 2u, 0.0f);
    bitrevLUT_.resize(static_cast<std::size_t>(size_), 0u);
}

bool FFT::IsValidSize(int fftSize) noexcept {
    return fftSize >= 2 && (fftSize & (fftSize - 1)) == 0;
}

void FFT::Forward(float* data) const {
    if (!data) {
        return;
    }

    EnsureTables();
    BitReverseOrder(data);

    const float* cosTable = cosTable_.data();
    const float* sinTable = sinTable_.data();

    for (int m = 2; m <= size_; m <<= 1) {
        const int m2 = m >> 1;
        const int step = size_ / m;

        for (int k = 0; k < size_; k += m) {
            for (int j = 0; j < m2; ++j) {
                const int tw = j * step;
                const float wr = cosTable[tw];
                const float wi = -sinTable[tw];

                const int i0 = k + j;
                const int i1 = i0 + m2;

                const float r0 = data[2 * i0];
                const float i0i = data[2 * i0 + 1];
                const float r1 = data[2 * i1];
                const float i1i = data[2 * i1 + 1];

                const float tr = wr * r1 - wi * i1i;
                const float ti = wr * i1i + wi * r1;

                data[2 * i1]     = r0 - tr;
                data[2 * i1 + 1] = i0i - ti;
                data[2 * i0]     = r0 + tr;
                data[2 * i0 + 1] = i0i + ti;
            }
        }
    }
}

void FFT::Inverse(float* data, bool scale) const {
    if (!data) {
        return;
    }

    EnsureTables();
    BitReverseOrder(data);

    const float* cosTable = cosTable_.data();
    const float* sinTable = sinTable_.data();

    for (int m = 2; m <= size_; m <<= 1) {
        const int m2 = m >> 1;
        const int step = size_ / m;

        for (int k = 0; k < size_; k += m) {
            for (int j = 0; j < m2; ++j) {
                const int tw = j * step;
                const float wr = cosTable[tw];
                const float wi = sinTable[tw];

                const int i0 = k + j;
                const int i1 = i0 + m2;

                const float r0 = data[2 * i0];
                const float i0i = data[2 * i0 + 1];
                const float r1 = data[2 * i1];
                const float i1i = data[2 * i1 + 1];

                const float tr = wr * r1 - wi * i1i;
                const float ti = wr * i1i + wi * r1;

                data[2 * i1]     = r0 - tr;
                data[2 * i1 + 1] = i0i - ti;
                data[2 * i0]     = r0 + tr;
                data[2 * i0 + 1] = i0i + ti;
            }
        }
    }

    if (scale) {
        const float invN = 1.0f / static_cast<float>(size_);
        for (int i = 0; i < size_; ++i) {
            data[2 * i]     *= invN;
            data[2 * i + 1] *= invN;
        }
    }
}

void FFT::ComplexMagnitude(const float* complexData, float* magnitude) const {
    if (!complexData || !magnitude) {
        return;
    }

    for (int i = 0; i < size_; ++i) {
        const float re = complexData[2 * i];
        const float im = complexData[2 * i + 1];
        magnitude[i] = std::sqrt(re * re + im * im);
    }
}

void FFT::EnsureTables() const {
    std::call_once(tablesInitFlag_, [this]() { InitializeTables(); });
}

void FFT::InitializeTables() const {
    const float twoPiOverN = kTwoPi / static_cast<float>(size_);
    for (int k = 0; k < size_ / 2; ++k) {
        cosTable_[static_cast<std::size_t>(k)] = std::cos(twoPiOverN * static_cast<float>(k));
        sinTable_[static_cast<std::size_t>(k)] = std::sin(twoPiOverN * static_cast<float>(k));
    }

    for (int i = 0; i < size_; ++i) {
        unsigned value = static_cast<unsigned>(i);
        unsigned reversed = 0U;
        for (int bit = 0; bit < bitCount_; ++bit) {
            reversed = (reversed << 1U) | (value & 1U);
            value >>= 1U;
        }
        bitrevLUT_[static_cast<std::size_t>(i)] = reversed;
    }
}

void FFT::BitReverseOrder(float* data) const {
    const auto* lut = bitrevLUT_.data();
    for (int i = 0; i < size_; ++i) {
        const auto j = static_cast<int>(lut[static_cast<std::size_t>(i)]);
        if (i < j) {
            std::swap(data[2 * i],     data[2 * j]);
            std::swap(data[2 * i + 1], data[2 * j + 1]);
        }
    }
}

int FFT::ComputeBitCount(int size) {
    int count = 0;
    while (size > 1) {
        size >>= 1;
        ++count;
    }
    return count;
}
