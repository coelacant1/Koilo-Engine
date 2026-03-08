// SPDX-License-Identifier: GPL-3.0-or-later
#include <koilo/core/signal/filter/fftfilter.hpp>
#include <cmath>


namespace koilo {

koilo::FFTFilter::FFTFilter() {}

float koilo::FFTFilter::GetOutput() {
    return outputValue;
}

float koilo::FFTFilter::Filter(float value) {
    float valueAbs = fabs(value);
    float normalized = valueAbs - minKF.Filter(valueAbs);
    
    outputValue = Mathematics::Constrain(normalized * 2.0f, 0.0f, 1.0f);
    
    return outputValue;
}

} // namespace koilo
