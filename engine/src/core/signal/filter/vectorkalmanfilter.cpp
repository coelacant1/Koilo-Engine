#include <ptx/core/signal/filter/vectorkalmanfilter.hpp>

VectorKalmanFilter::VectorKalmanFilter(float processNoise, float sensorNoise, float errorCovariance)
    : X(processNoise, sensorNoise, errorCovariance),
      Y(processNoise, sensorNoise, errorCovariance),
      Z(processNoise, sensorNoise, errorCovariance) {}

Vector3D VectorKalmanFilter::Filter(const Vector3D& input) {
    return Vector3D{
        X.Filter(input.X),
        Y.Filter(input.Y),
        Z.Filter(input.Z)
    };
}

void VectorKalmanFilter::Reset(Vector3D estimation, float errorCovariance) {
    X.Reset(estimation.X, errorCovariance);
    Y.Reset(estimation.Y, errorCovariance);
    Z.Reset(estimation.Z, errorCovariance);
}
