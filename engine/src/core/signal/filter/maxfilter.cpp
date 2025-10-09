#include <ptx/core/signal/filter/maxfilter.hpp>

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

MaxFilter::MaxFilter(size_t memory)
    : capacity(std::max<size_t>(1, memory)),
      blockCount(ComputeBlockCount(std::max<size_t>(1, memory))),
      values(capacity, 0.0f),
      maxValues(blockCount, 0.0f) {
}

void MaxFilter::Reset() {
    std::fill(values.begin(), values.end(), 0.0f);
    std::fill(maxValues.begin(), maxValues.end(), 0.0f);
    currentAmount = 0;
}

void MaxFilter::ShiftArray(std::vector<float>& arr) {
    if (arr.empty()) {
        return;
    }
    for (size_t i = 0; i + 1 < arr.size(); ++i) {
        arr[i] = arr[i + 1];
    }
    arr.back() = 0.0f;
}

float MaxFilter::Filter(float value) {
    if (capacity == 0) {
        return value;
    }

    if (currentAmount < capacity) {
        values[currentAmount++] = value;
    } else {
        ShiftArray(values);
        values.back() = value;
    }

    float currentMax = values[0];
    for (size_t i = 1; i < currentAmount; ++i) {
        currentMax = std::max(currentMax, values[i]);
    }

    if (maxValues.empty()) {
        return currentMax;
    }

    if (maxValues.back() != currentMax) {
        ShiftArray(maxValues);
        maxValues.back() = currentMax;
    }

    const float sum = std::accumulate(maxValues.begin(), maxValues.end(), 0.0f);
    return sum / static_cast<float>(blockCount);
}
