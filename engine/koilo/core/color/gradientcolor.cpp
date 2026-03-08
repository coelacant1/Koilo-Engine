// SPDX-License-Identifier: GPL-3.0-or-later
#include <algorithm>

#include <koilo/core/color/gradientcolor.hpp>

namespace koilo {

koilo::GradientColor::GradientColor(const Color888* colorStops, std::size_t count, bool stepped)
    : colors(), isStepped(stepped) {
    SetColors(colorStops, count);
}

koilo::GradientColor::GradientColor(std::vector<Color888> colorStops, bool stepped)
    : colors(std::move(colorStops)), isStepped(stepped) {}

Color888 koilo::GradientColor::GetColorAt(float ratio) const {
    if (colors.empty()) {
        return Color888(0.0f, 0.0f, 0.0f);
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
    return Color888::Lerp(colors[startIndex], colors[endIndex], mu);
}

void koilo::GradientColor::SetColors(const Color888* newColorStops, std::size_t count) {
    if (!newColorStops || count == 0) {
        colors.clear();
        return;
    }

    colors.resize(count);
    std::copy(newColorStops, newColorStops + count, colors.begin());
}

void koilo::GradientColor::SetColors(const std::vector<Color888>& newColorStops) {
    colors = newColorStops;
}

std::size_t koilo::GradientColor::GetColorCount() const noexcept {
    return colors.size();
}

bool koilo::GradientColor::IsStepped() const noexcept {
    return isStepped;
}

void koilo::GradientColor::SetStepped(bool stepped) noexcept {
    isStepped = stepped;
}

} // namespace koilo
