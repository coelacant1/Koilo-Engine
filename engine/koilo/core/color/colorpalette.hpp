// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file colorpalette.hpp
 * @brief Script-friendly color array for material color APIs.
 *
 * Wraps a vector<Color888> with reflected Add/Set/Get/Clear methods so
 * scripts can build palettes without C-style pointer+count APIs.
 *
 * @date 22/02/2026
 * @version 1.0
 * @author Coela Can't
 */

#pragma once

#include "color888.hpp"
#include <koilo/registry/reflect_macros.hpp>
#include <vector>

namespace koilo {

class ColorPalette {
public:
    ColorPalette() = default;
    ColorPalette(std::size_t capacity) { colors_.reserve(capacity); }

    void Add(int r, int g, int b) { colors_.emplace_back(r, g, b); }
    void AddColor(Color888 c) { colors_.push_back(c); }
    void SetAt(int index, int r, int g, int b) {
        if (index >= 0 && index < static_cast<int>(colors_.size()))
            colors_[index] = Color888(r, g, b);
    }
    Color888 GetAt(int index) const {
        if (index >= 0 && index < static_cast<int>(colors_.size()))
            return colors_[index];
        return Color888(0, 0, 0);
    }
    int GetCount() const { return static_cast<int>(colors_.size()); }
    void Clear() { colors_.clear(); }

    // C++ API for passing to existing (Color888*, count) methods
    const Color888* Data() const { return colors_.data(); }
    std::size_t Size() const { return colors_.size(); }

    KL_BEGIN_FIELDS(ColorPalette)
    KL_END_FIELDS

    KL_BEGIN_METHODS(ColorPalette)
        KL_METHOD_AUTO(ColorPalette, Add, "Add color (r,g,b)"),
        KL_METHOD_AUTO(ColorPalette, AddColor, "Add Color888"),
        KL_METHOD_AUTO(ColorPalette, SetAt, "Set color at index"),
        KL_METHOD_AUTO(ColorPalette, GetAt, "Get color at index"),
        KL_METHOD_AUTO(ColorPalette, GetCount, "Get number of colors"),
        KL_METHOD_AUTO(ColorPalette, Clear, "Clear all colors")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(ColorPalette)
        KL_CTOR0(ColorPalette),
        KL_CTOR(ColorPalette, std::size_t)
    KL_END_DESCRIBE(ColorPalette)

private:
    std::vector<Color888> colors_;
};

} // namespace koilo
