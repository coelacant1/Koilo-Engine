#include <ptx/core/signal/filter/quaternionkalmanfilter.hpp>

#include <algorithm>

QuaternionKalmanFilter::QuaternionKalmanFilter() {
    gain = 0.25f;
    memory = 25;
    values.assign(static_cast<size_t>(memory), Quaternion());
}

QuaternionKalmanFilter::QuaternionKalmanFilter(float gain, int memory) {
    this->gain = gain;
    this->memory = std::max(1, memory);
    values.assign(static_cast<size_t>(this->memory), Quaternion());
}

void QuaternionKalmanFilter::ShiftArray() {
    if (values.empty()) {
        return;
    }

    std::rotate(values.begin(), values.begin() + 1, values.end());
    values.back() = Quaternion();
}

Quaternion QuaternionKalmanFilter::Filter(Quaternion value) {
    if (currentAmount < memory) {
        values[currentAmount++] = value;
    } else {
        ShiftArray();  // pop first
        values[static_cast<size_t>(memory) - 1] = value;
    }

    Quaternion out = Quaternion(0, 0, 0, 0);

    for (int i = 0; i < currentAmount; i++) {
        out = out.Add(values[i].Divide(currentAmount));
    }

    out = out.UnitQuaternion();

    return Quaternion::SphericalInterpolation(value, out, 1 - gain);
}

// Trivial destructor definition to satisfy linker (was previously only declared)
QuaternionKalmanFilter::~QuaternionKalmanFilter() = default;
