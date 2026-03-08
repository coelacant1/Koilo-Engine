// SPDX-License-Identifier: GPL-3.0-or-later
#include <koilo/core/signal/functiongenerator.hpp>
#include <cmath>


namespace koilo {

koilo::FunctionGenerator::FunctionGenerator(Function function, float minimum, float maximum, float period) {
    this->function = function;
    this->minimum = minimum;
    this->maximum = maximum;
    this->period = period;
}

void koilo::FunctionGenerator::SetPeriod(float period) {
    this->period = period;
}

void koilo::FunctionGenerator::SetFunction(Function function) {
    this->function = function;
}

float koilo::FunctionGenerator::Update() {
    float currentTime = fmod(koilo::Time::Micros() / 1000000.0f, period);
    float ratio = currentTime / period;

    switch (function) {
        case Triangle:
            return TriangleWave(ratio);
        case Square:
            return SquareWave(ratio);
        case Sine:
            return SineWave(ratio);
        case Sawtooth:
            return SawtoothWave(ratio);
        case Gravity:
            return GravityFunction(ratio);
        default:
            return 0.0f;
    }
}

float koilo::FunctionGenerator::TriangleWave(float ratio) {
    float wave = (ratio > 0.5f ? 1.0f - ratio : ratio) * 2.0f;
    return Mathematics::Map(wave, 0.0f, 1.0f, minimum, maximum);
}

float koilo::FunctionGenerator::SquareWave(float ratio) {
    float wave = ratio > 0.5f ? 1.0f : 0.0f;
    return Mathematics::Map(wave, 0.0f, 1.0f, minimum, maximum);
}

float koilo::FunctionGenerator::SineWave(float ratio) {
    float wave = sinf(ratio * 360.0f * 3.14159f / 180.0f);
    return Mathematics::Map(wave, -1.0f, 1.0f, minimum, maximum);
}

float koilo::FunctionGenerator::SawtoothWave(float ratio) {
    return Mathematics::Map(ratio, 0.0f, 1.0f, minimum, maximum);
}

float koilo::FunctionGenerator::GravityFunction(float ratio) {//drop for first half, then stay at the bottom for the second half
    return (ratio * 2.0f) < 1.0f ? ratio * ratio * 4.0f : 1.0f;
}

} // namespace koilo
