// SPDX-License-Identifier: GPL-3.0-or-later
#include <koilo/core/signal/filter/derivativefilter.hpp>
#include <cmath>


namespace koilo {

koilo::DerivativeFilter::DerivativeFilter() {}

float koilo::DerivativeFilter::GetOutput() {
    return outputValue;
}

float koilo::DerivativeFilter::Filter(float value) {
    float amplitude = fabs(value - previousReading);
    float normalized = output.Filter(amplitude);
    float minimum = minFilter.Filter(normalized);

    previousReading = value;
    outputValue = Mathematics::Constrain(normalized - minimum, 0.0f, 1.0f);

    return outputValue;
}

} // namespace koilo
