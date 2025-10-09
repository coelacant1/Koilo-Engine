#include <ptx/core/signal/filter/kalmanfilter.hpp>

#include <limits>

KalmanFilter::KalmanFilter(float processNoise, float sensorNoise, float errorCovariance)
    : processNoise(processNoise),
      sensorNoise(sensorNoise),
      estimation(0.0f),
      errorCovariance(errorCovariance) {}

void KalmanFilter::Reset(float estimationValue, float errorCovarianceValue) {
    estimation = estimationValue;
    errorCovariance = errorCovarianceValue;
}

float KalmanFilter::Filter(float value) {
    errorCovariance += processNoise;

    const float denominator = errorCovariance + sensorNoise;
    const float threshold = std::numeric_limits<float>::epsilon();
    const float kalmanGain = (denominator > threshold) ? (errorCovariance / denominator) : 0.0f;

    estimation += kalmanGain * (value - estimation);
    errorCovariance *= (1.0f - kalmanGain);

    return estimation;
}
