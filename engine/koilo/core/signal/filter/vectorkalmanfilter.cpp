// SPDX-License-Identifier: GPL-3.0-or-later
#include <koilo/core/signal/filter/vectorkalmanfilter.hpp>


namespace koilo {

koilo::VectorKalmanFilter::VectorKalmanFilter(float processNoise, float sensorNoise, float errorCovariance)
    : X(processNoise, sensorNoise, errorCovariance),
      Y(processNoise, sensorNoise, errorCovariance),
      Z(processNoise, sensorNoise, errorCovariance) {}

Vector3D koilo::VectorKalmanFilter::Filter(const Vector3D& input) {
    return Vector3D{
        X.Filter(input.X),
        Y.Filter(input.Y),
        Z.Filter(input.Z)
    };
}

void koilo::VectorKalmanFilter::Reset(Vector3D estimation, float errorCovariance) {
    X.Reset(estimation.X, errorCovariance);
    Y.Reset(estimation.Y, errorCovariance);
    Z.Reset(estimation.Z, errorCovariance);
}

} // namespace koilo
