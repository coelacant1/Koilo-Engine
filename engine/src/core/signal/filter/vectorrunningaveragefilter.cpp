#include <ptx/core/signal/filter/vectorrunningaveragefilter.hpp>

#include <algorithm>

VectorRunningAverageFilter::VectorRunningAverageFilter(size_t memory, float gainValue)
    : X(memory, gainValue),
      Y(memory, gainValue),
      Z(memory, gainValue),
      capacity(std::max<size_t>(1, memory))
{}

Vector3D VectorRunningAverageFilter::Filter(Vector3D input) {
    return Vector3D{
        X.Filter(input.X),
        Y.Filter(input.Y),
        Z.Filter(input.Z)
    };
}

void VectorRunningAverageFilter::SetGain(float gainValue) {
    X.SetGain(gainValue);
    Y.SetGain(gainValue);
    Z.SetGain(gainValue);
}

void VectorRunningAverageFilter::Reset() {
    X.Reset();
    Y.Reset();
    Z.Reset();
}
