#include <algorithm>

#include <ptx/core/color/gradientcolor.hpp>

GradientColor::GradientColor(const RGBColor* colorStops, std::size_t count, bool stepped)
    : colors(), isStepped(stepped) {
    SetColors(colorStops, count);
}

GradientColor::GradientColor(std::vector<RGBColor> colorStops, bool stepped)
    : colors(std::move(colorStops)), isStepped(stepped) {}

RGBColor GradientColor::GetColorAt(float ratio) const {
    if (colors.empty()) {
        return RGBColor(0.0f, 0.0f, 0.0f);
    }
    if (colors.size() == 1) {
        return colors.front();
    }

    const float clamped = Mathematics::Max(0.0f, Mathematics::Min(1.0f, ratio));
    const float rawPosition = clamped * static_cast<float>(colors.size() - 1);
    std::size_t startIndex = static_cast<std::size_t>(rawPosition);
    startIndex = std::min(startIndex, colors.size() - 2);

    if (isStepped) {
        return colors[startIndex];
    }

    const std::size_t endIndex = startIndex + 1;
    const float mu = rawPosition - static_cast<float>(startIndex);
    return RGBColor::InterpolateColors(colors[startIndex], colors[endIndex], mu);
}

void GradientColor::SetColors(const RGBColor* newColorStops, std::size_t count) {
    if (!newColorStops || count == 0) {
        colors.clear();
        return;
    }

    colors.resize(count);
    std::copy(newColorStops, newColorStops + count, colors.begin());
}

void GradientColor::SetColors(const std::vector<RGBColor>& newColorStops) {
    colors = newColorStops;
}

std::size_t GradientColor::GetColorCount() const noexcept {
    return colors.size();
}

bool GradientColor::IsStepped() const noexcept {
    return isStepped;
}

void GradientColor::SetStepped(bool stepped) noexcept {
    isStepped = stepped;
}
