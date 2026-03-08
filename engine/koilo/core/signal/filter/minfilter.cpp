#include <ptx/core/signal/filter/minfilter.hpp>

#include <algorithm>
#include <numeric>

namespace {
constexpr size_t kDefaultBlockDivisor = 10;

size_t ComputeBlockCount(size_t capacity) {
    if (capacity == 0) {
        return 1;
    }
    const size_t raw = capacity / kDefaultBlockDivisor;
    return std::max<size_t>(1, raw);
}
}

MinFilter::MinFilter(size_t memory, bool ignoreSameValue)
    : capacity(std::max<size_t>(1, memory)),
      blockCount(ComputeBlockCount(std::max<size_t>(1, memory))),
      values(capacity, 0.0f),
      minValues(blockCount, 0.0f),
      ignoreSame(ignoreSameValue) {
}

void MinFilter::Reset() {
    std::fill(values.begin(), values.end(), 0.0f);
    std::fill(minValues.begin(), minValues.end(), 0.0f);
    currentAmount = 0;
}

void MinFilter::ShiftArray(std::vector<float>& arr) {
    if (arr.empty()) {
        return;
    }
    for (size_t i = 0; i + 1 < arr.size(); ++i) {
        arr[i] = arr[i + 1];
    }
    arr.back() = 0.0f;
}

float MinFilter::Filter(float value) {
    if (capacity == 0) {
        return value;
    }

    if (currentAmount < capacity) {
        values[currentAmount++] = value;
    } else {
        ShiftArray(values);
        values.back() = value;
    }

    float currentMin = values[0];
    for (size_t i = 1; i < currentAmount; ++i) {
        currentMin = std::min(currentMin, values[i]);
    }

    if (minValues.empty()) {
        return currentMin;
    }

    const bool valueChanged = minValues.back() != currentMin;
    if (valueChanged || !ignoreSame) {
        ShiftArray(minValues);
        minValues.back() = currentMin;
    }

    const float sum = std::accumulate(minValues.begin(), minValues.end(), 0.0f);
    return sum / static_cast<float>(blockCount);
}
