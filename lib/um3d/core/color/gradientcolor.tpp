#pragma once

template<size_t colorCount>
GradientColor<colorCount>::GradientColor(const RGBColor* colorStops, bool stepped)
    : isStepped(stepped) {
    if (colorStops) {
        for (size_t i = 0; i < colorCount; ++i) {
            colors[i] = colorStops[i];
        }
    }
}

template<size_t colorCount>
GradientColor<colorCount>::GradientColor() : isStepped(false) {}

template<size_t colorCount>
RGBColor GradientColor<colorCount>::GetColorAt(float ratio) const {
    if (colorCount == 0) {
        return RGBColor(0, 0, 0); // Return black if no colors
    }
    if (colorCount == 1) {
        return colors[0]; // Return the only color
    }

    // Clamp ratio to the valid [0.0, 1.0] range
    ratio = Mathematics::Max(0.0f, Mathematics::Min(1.0f, ratio));

    // Calculate a "raw" position in the color array.
    // For N colors, there are N-1 segments.
    float rawPosition = ratio * (colorCount - 1);
    
    // Determine the index of the starting color
    size_t startIndex = static_cast<size_t>(rawPosition);
    
    // Ensure startIndex doesn't exceed the second to last element
    startIndex = std::min(startIndex, colorCount - 2);

    if (isStepped) {
        // For stepped gradients, just return the color of the segment we're in.
        return colors[startIndex];
    } else {
        // For smooth gradients, interpolate between the start and end colors.
        size_t endIndex = startIndex + 1;

        // 'mu' is the fractional part between the start and end index (0.0 to 1.0)
        float mu = rawPosition - static_cast<float>(startIndex);

        return RGBColor::InterpolateColors(colors[startIndex], colors[endIndex], mu);
    }
}

template<size_t colorCount>
void GradientColor<colorCount>::SetColors(const RGBColor* newColorStops) {
    if (newColorStops) {
        for (size_t i = 0; i < colorCount; ++i) {
            colors[i] = newColorStops[i];
        }
    }
}
