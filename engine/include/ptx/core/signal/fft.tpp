#pragma once
#include "fft.hpp"

// ===== Internal tables (one-time init per template instantiation) ===========
template<int N>
float* FFT<N>::CosTable() {
    static float COS[N/2];
    static bool  init = false;
    if (!init) {
        const float twoPiOverN = 6.28318530717958647692f / float(N);
        for (int k = 0; k < N/2; ++k) {
            COS[k] = std::cosf(twoPiOverN * k);
        }
        init = true;
    }
    return COS;
}

template<int N>
float* FFT<N>::SinTable() {
    static float SIN[N/2];
    static bool  init = false;
    if (!init) {
        const float twoPiOverN = 6.28318530717958647692f / float(N);
        for (int k = 0; k < N/2; ++k) {
            SIN[k] = std::sinf(twoPiOverN * k);
        }
        init = true;
    }
    return SIN;
}

template<int N>
uint16_t* FFT<N>::BitrevLUT() {
    static uint16_t LUT[N];
    static bool     init = false;
    if (!init) {
        const int bits = BitCount();
        for (int i = 0; i < N; ++i) {
            unsigned v = unsigned(i);
            unsigned r = 0;
            for (int b = 0; b < bits; ++b) {
                r = (r << 1) | (v & 1u);
                v >>= 1u;
            }
            LUT[i] = uint16_t(r);
        }
        init = true;
    }
    return LUT;
}

template<int N>
void FFT<N>::EnsureTables() {
    (void)CosTable();
    (void)SinTable();
    (void)BitrevLUT();
}

// ===== Bit utilities ========================================================

template<int N>
constexpr int FFT<N>::BitCount() {
    int c = 0, s = N;
    while (s > 1) { s >>= 1; ++c; }
    return c;
}

template<int N>
void FFT<N>::BitReverseOrder(float* data) {
    auto* lut = BitrevLUT();
    for (int i = 0; i < N; ++i) {
        int j = lut[i];
        if (i < j) {
            // swap complex(i) <-> complex(j)
            float tr = data[2*i    ];
            float ti = data[2*i + 1];
            data[2*i    ] = data[2*j    ];
            data[2*i + 1] = data[2*j + 1];
            data[2*j    ] = tr;
            data[2*j + 1] = ti;
        }
    }
}

// ===== Core FFTs ============================================================

template<int N>
void FFT<N>::Forward(float* data) {
    EnsureTables();
    BitReverseOrder(data);

    auto* COS = CosTable();
    auto* SIN = SinTable();

    // Stage loop: m = size of sub-FFT
    for (int m = 2; m <= N; m <<= 1) {
        const int m2   = m >> 1;           // half-size
        const int step = N / m;            // twiddle step

        for (int k = 0; k < N; k += m) {
            // j = butterfly index inside this block
            for (int j = 0; j < m2; ++j) {
                const int tw = j * step;   // twiddle index
                const float wr = COS[tw];  // cos(2π*tw/N)
                const float wi = -SIN[tw]; // -sin(...)  (forward FFT)

                const int i0 = k + j;
                const int i1 = i0 + m2;

                const float r0 = data[2*i0    ];
                const float i0i = data[2*i0 + 1];
                const float r1 = data[2*i1    ];
                const float i1i = data[2*i1 + 1];

                // t = w * (r1 + i i1)
                const float tr = wr * r1 - wi * i1i;
                const float ti = wr * i1i + wi * r1;

                // butterfly
                data[2*i1    ] = r0 - tr;
                data[2*i1 + 1] = i0i - ti;
                data[2*i0    ] = r0 + tr;
                data[2*i0 + 1] = i0i + ti;
            }
        }
    }
}

template<int N>
void FFT<N>::Inverse(float* data, bool scale) {
    EnsureTables();
    BitReverseOrder(data);

    auto* COS = CosTable();
    auto* SIN = SinTable();

    // Same as Forward but with +sin (i.e., conjugated twiddles)
    for (int m = 2; m <= N; m <<= 1) {
        const int m2   = m >> 1;
        const int step = N / m;

        for (int k = 0; k < N; k += m) {
            for (int j = 0; j < m2; ++j) {
                const int tw = j * step;
                const float wr = COS[tw];
                const float wi = +SIN[tw]; // inverse FFT uses +sin

                const int i0 = k + j;
                const int i1 = i0 + m2;

                const float r0 = data[2*i0    ];
                const float i0i = data[2*i0 + 1];
                const float r1 = data[2*i1    ];
                const float i1i = data[2*i1 + 1];

                const float tr = wr * r1 - wi * i1i;
                const float ti = wr * i1i + wi * r1;

                data[2*i1    ] = r0 - tr;
                data[2*i1 + 1] = i0i - ti;
                data[2*i0    ] = r0 + tr;
                data[2*i0 + 1] = i0i + ti;
            }
        }
    }

    if (scale) {
        const float invN = 1.0f / float(N);
        for (int i = 0; i < N; ++i) {
            data[2*i    ] *= invN;
            data[2*i + 1] *= invN;
        }
    }
}

template<int N>
void FFT<N>::ComplexMagnitude(const float* complexData, float* magnitude) {
    for (int i = 0; i < N; ++i) {
        const float re = complexData[2*i    ];
        const float im = complexData[2*i + 1];
        magnitude[i] = std::sqrt(re*re + im*im);
    }
}
