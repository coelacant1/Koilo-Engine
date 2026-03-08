// SPDX-License-Identifier: GPL-3.0-or-later
#include <koilo/core/math/mathematics.hpp>
#include <cmath>
#include <math.h>


namespace koilo {

const float koilo::Mathematics::EPSILON = 0.001f;
const float koilo::Mathematics::MPI = 3.14159265358979323846f;
const float koilo::Mathematics::MPID180 = 0.01745329251994329576f;
const float koilo::Mathematics::M180DPI = 57.29577951308232087684f;
const float koilo::Mathematics::FLTMAX = __FLT_MAX__;
const float koilo::Mathematics::FLTMIN = __FLT_MIN__;

koilo::UString koilo::Mathematics::DoubleToCleanString(float value) {
    return koilo::UString::FromFloat(value, 3);
}

bool koilo::Mathematics::IsNaN(float value) {
    return value != value;
}

bool koilo::Mathematics::IsInfinite(float value) {
    return std::isinf(value);
}

bool koilo::Mathematics::IsFinite(float value) {
    return std::isfinite(value);
}

bool koilo::Mathematics::IsClose(float v1, float v2, float epsilon) {
    return std::fabs(v1 - v2) < epsilon;
}

int koilo::Mathematics::Sign(float value) {
    return (0 < value) - (value < 0);
}

float koilo::Mathematics::Pow(float value, float exponent) {
    return powf(value, exponent);
}

float koilo::Mathematics::Sqrt(float value) {
    return sqrtf(value);
}

float koilo::Mathematics::Fract(float value) {
    return value - std::floor(value);
}

float koilo::Mathematics::CosineInterpolation(float beg, float fin, float ratio) {
    float mu2 = (1.0f - cosf(ratio * MPI)) / 2.0f;

    return (beg * (1.0f - mu2) + fin * mu2);
}

float koilo::Mathematics::CosineTransition(float beg, float fin, float ratio) {
    float mid = (beg - fin) / 2.0f;

    if (ratio < mid) return CosineInterpolation(beg, fin, 1.0f - ratio * 2.0f);
    else return CosineInterpolation(fin, beg, (ratio - 0.5f) * 2.0f);
}

float koilo::Mathematics::BounceInterpolation(float beg, float fin, float ratio) {
    float baseLog = log10f(10.0f * ratio + 1.0f);
    float baseSine = sinf(16.0f * ratio) * powf((2.0f * ratio - 2.0f), 2.0f) / 4.0f / 4.0f;
    float bounce = baseLog + baseSine;

    return Map(ratio, 0.0f, bounce, beg, fin);
}

float koilo::Mathematics::FFloor(float f) { 
    int fi = (int)f; return f < fi ? fi - 1 : fi; 
}

float koilo::Mathematics::FAbs(float f) { 
    return f < 0 ? -f : f; 
}

float koilo::Mathematics::FSqrt(float f) { 
    return sqrtf(f); 
}

float koilo::Mathematics::HermiteInterpolation(float t) { 
    return t * t * (3 - 2 * t); 
}

float koilo::Mathematics::QuinticInterpolation(float t) {
    return t * t * t * (t * (t * 6 - 15) + 10); 
}

float koilo::Mathematics::Lerp(float a, float b, float t) {
    return a + t * (b - a); 
}

float koilo::Mathematics::SmoothStep(float edge0, float edge1, float t) {
    float x = Constrain((t - edge0) / (edge1 - edge0), 0.0f, 1.0f);
    return x * x * (3.0f - 2.0f * x);
}

float koilo::Mathematics::SmootherStep(float edge0, float edge1, float t) {
    float x = Constrain((t - edge0) / (edge1 - edge0), 0.0f, 1.0f);
    return x * x * x * (x * (x * 6.0f - 15.0f) + 10.0f);
}

float koilo::Mathematics::CubicLerp(float a, float b, float c, float d, float t){
    float p = (d - c) - (a - b);
    float q = (a - b) - p;
    float r = c - a;
    float s = b;

    return p * t * t * t + q * t * t + r * t + s;
}

float koilo::Mathematics::BilinearInterpolation(float scaleX, float scaleY, float x1, float y1, float x2, float y2, float v11, float v12, float v21, float v22) {
    // Calculate the interpolation weights
    float s1 = x2 - scaleX;
    float s2 = scaleX - x1;
    float t1 = y2 - scaleY;
    float t2 = scaleY - y1;

    // Perform bilinear interpolation
    float interpolatedValue = (s1 * t1 * v11 + s2 * t1 * v21 + s1 * t2 * v12 + s2 * t2 * v22) / ((x2 - x1) * (y2 - y1));

    return interpolatedValue;
}


float koilo::Mathematics::PingPong(float t){
    t -= (int)(t * 0.5f) * 2;
    return t < 1 ? t : 2 - t;
}

int koilo::Mathematics::RoundUpWindow(int value, int multiple) {
    if (multiple == 0)
        return value;

    int remainder = abs(value) % multiple;
    if (remainder == 0)
        return value;

    if (value < 0)
        return -(std::abs(value) - remainder);
    else
        return value + multiple - remainder;
}

float koilo::Mathematics::Sin(float radians) { return std::sin(radians); }
float koilo::Mathematics::Cos(float radians) { return std::cos(radians); }
float koilo::Mathematics::Tan(float radians) { return std::tan(radians); }
float koilo::Mathematics::ATan2(float y, float x) { return std::atan2(y, x); }
float koilo::Mathematics::FMod(float x, float y) { return std::fmod(x, y); }
float koilo::Mathematics::FRound(float x) { return std::round(x); }

} // namespace koilo
