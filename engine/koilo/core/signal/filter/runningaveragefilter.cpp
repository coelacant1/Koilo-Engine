// SPDX-License-Identifier: GPL-3.0-or-later
#include <koilo/core/signal/filter/runningaveragefilter.hpp>

#include <algorithm>


namespace koilo {

koilo::RunningAverageFilter::RunningAverageFilter(size_t memory, float gainValue)
        : gain(gainValue),
            data(std::max<size_t>(1, memory), 0.0f),
            capacity(std::max<size_t>(1, memory)),
      currentAmount(0),
      nextIndex(0),
      runningSum(0.0f) {
}

void koilo::RunningAverageFilter::SetGain(float gainValue) {
    gain = gainValue;
}

float koilo::RunningAverageFilter::Filter(float value) {
    if (capacity == 0) {
        return value;
    }

    if (currentAmount < capacity) {
        runningSum += value;
        data[nextIndex] = value;
        ++currentAmount;
    } else {
        runningSum -= data[nextIndex];
        runningSum += value;
        data[nextIndex] = value;
    }

    nextIndex = (nextIndex + 1) % capacity;

    float average = currentAmount > 0 ? (runningSum / static_cast<float>(currentAmount)) : value;
    return (gain * value) + ((1.0f - gain) * average);
}

void koilo::RunningAverageFilter::Reset() {
    std::fill(data.begin(), data.end(), 0.0f);
    currentAmount = 0;
    nextIndex = 0;
    runningSum = 0.0f;
}

} // namespace koilo
