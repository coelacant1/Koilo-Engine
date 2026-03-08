// SPDX-License-Identifier: GPL-3.0-or-later
#include <koilo/core/signal/filter/rampfilter.hpp>


namespace koilo {

koilo::RampFilter::RampFilter() {
    increment = 0.05f;
    filter = 0.0f;
    epsilon = 0.01f;
}

koilo::RampFilter::RampFilter(int frames, float epsilon) {
    increment = 1.0f / float(frames);
    this->epsilon = epsilon;
    filter = 0.0f;
}

float koilo::RampFilter::Filter(float value) {
    if (Mathematics::IsClose(value, filter, increment / 2.0f)) return filter;

    if (value > filter + epsilon) {
        filter = filter + increment < 1.0f ? filter + increment : 1.0f;
    } else if (value < filter - epsilon) {
        filter = filter - increment > 0.0f ? filter - increment : 0.0f;
    }

    return filter;
}

void koilo::RampFilter::SetIncrement(float increment) {
    this->increment = increment;
}

void koilo::RampFilter::SetFrames(int frames) {
    increment = 1.0f / float(frames);
}

} // namespace koilo
