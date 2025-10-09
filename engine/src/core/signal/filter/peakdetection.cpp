#include <ptx/core/signal/filter/peakdetection.hpp>

#include <algorithm>
#include <cmath>

PeakDetection::PeakDetection(size_t sampleSizeValue, uint8_t lagValue, float thresholdValue, float influenceValue)
    : sampleSize(sampleSizeValue > 0 ? sampleSizeValue : 1),
      lag(std::max<uint8_t>(lagValue, static_cast<uint8_t>(1))),
      threshold(thresholdValue),
      influence(influenceValue),
      filteredData(sampleSize, 0.0f),
      averages(sampleSize, 0.0f),
      deviations(sampleSize, 0.0f) {}

void PeakDetection::Reset() {
    std::fill(filteredData.begin(), filteredData.end(), 0.0f);
    std::fill(averages.begin(), averages.end(), 0.0f);
    std::fill(deviations.begin(), deviations.end(), 0.0f);
}

void PeakDetection::GetStdDev(size_t start, size_t length, const float* data, float& avgRet, float& stdRet) const {
    if (data == nullptr || length == 0 || sampleSize == 0) {
        avgRet = 0.0f;
        stdRet = 0.0f;
        return;
    }

    const size_t clampedStart = std::min(start, sampleSize - 1);
    const size_t endIndex = std::min(clampedStart + length, sampleSize);
    const size_t count = (endIndex > clampedStart) ? (endIndex - clampedStart) : 0;

    if (count == 0) {
        avgRet = 0.0f;
        stdRet = 0.0f;
        return;
    }

    float sum = 0.0f;
    for (size_t i = clampedStart; i < endIndex; ++i) {
        sum += data[i];
    }

    const float average = sum / static_cast<float>(count);

    float variance = 0.0f;
    for (size_t i = clampedStart; i < endIndex; ++i) {
        const float diff = data[i] - average;
        variance += diff * diff;
    }

    avgRet = average;
    stdRet = std::sqrt(variance / static_cast<float>(count));
}

void PeakDetection::Calculate(const float* data, std::vector<bool>& peaks) {
    if (data == nullptr || sampleSize == 0) {
        peaks.clear();
        return;
    }

    if (peaks.size() != sampleSize) {
        peaks.assign(sampleSize, false);
    } else {
        std::fill(peaks.begin(), peaks.end(), false);
    }

    Reset();

    float maxData = 0.0f;
    for (size_t i = 0; i < sampleSize; ++i) {
        maxData = std::max(maxData, data[i]);
    }

    if (maxData <= threshold || lag == 0 || lag >= sampleSize) {
        return;
    }

    float average = 0.0f;
    float stdDev = 0.0f;

    GetStdDev(0, lag, data, average, stdDev);

    averages[lag - 1] = average;
    deviations[lag - 1] = stdDev;

    for (size_t i = lag; i < sampleSize - lag; ++i) {
        if (std::fabs(data[i] - averages[i - 1]) > threshold * deviations[i - 1]) {
            peaks[i] = data[i] > averages[i - 1];
            filteredData[i] = influence * data[i] + (1.0f - influence) * filteredData[i - 1];
        } else {
            peaks[i] = false;
            filteredData[i] = data[i];
        }

        GetStdDev(i - lag + 1, lag, data, average, stdDev);

        averages[i] = average;
        deviations[i] = stdDev;
    }
}
