// SPDX-License-Identifier: GPL-3.0-or-later
#include <koilo/systems/font/glyph_atlas.hpp>

#include <algorithm>
#include <cstring>

namespace koilo {
namespace font {

// --- GlyphAtlas public ----------------------------------------------

const GlyphRegion* GlyphAtlas::Find(const GlyphKey& key) const {
    auto it = regions_.find(key);
    return (it != regions_.end()) ? &it->second : nullptr;
}

const GlyphRegion* GlyphAtlas::Add(const GlyphKey& key, const GlyphBitmap& bmp,
                                   float bearingX, float bearingY, float advance) {
    // Already cached?
    auto existing = regions_.find(key);
    if (existing != regions_.end()) return &existing->second;

    int gw = bmp.width + GLYPH_PADDING;
    int gh = bmp.height + GLYPH_PADDING;

    // Try to place in current shelf
    if (!TryPlace(gw, gh)) {
        // Try growing atlas
        if (!Grow()) return nullptr;
        if (!TryPlace(gw, gh)) return nullptr;
    }

    // Copy bitmap into atlas
    int px = cursorX_;
    int py = cursorY_;
    for (int y = 0; y < bmp.height; ++y) {
        for (int x = 0; x < bmp.width; ++x) {
            pixels_[(py + y) * width_ + (px + x)] = bmp.pixels[y * bmp.width + x];
        }
    }

    // Build region
    GlyphRegion reg;
    reg.atlasX = static_cast<uint16_t>(px);
    reg.atlasY = static_cast<uint16_t>(py);
    reg.width  = static_cast<uint16_t>(bmp.width);
    reg.height = static_cast<uint16_t>(bmp.height);
    reg.u0 = static_cast<float>(px) / static_cast<float>(width_);
    reg.v0 = static_cast<float>(py) / static_cast<float>(height_);
    reg.u1 = static_cast<float>(px + bmp.width) / static_cast<float>(width_);
    reg.v1 = static_cast<float>(py + bmp.height) / static_cast<float>(height_);
    reg.bearingX = bearingX;
    reg.bearingY = bearingY;
    reg.advance  = advance;

    cursorX_ += gw;
    if (py + gh > shelfHeight_) shelfHeight_ = py + gh;

    dirty_ = true;
    auto result = regions_.emplace(key, reg);
    return &result.first->second;
}

void GlyphAtlas::Clear() {
    std::memset(pixels_.data(), 0, pixels_.size());
    regions_.clear();
    cursorX_ = 0;
    cursorY_ = 0;
    shelfHeight_ = 0;
    dirty_ = true;
}

// --- GlyphAtlas private ---------------------------------------------

bool GlyphAtlas::TryPlace(int w, int h) {
    // Fits on current shelf?
    if (cursorX_ + w <= width_ && cursorY_ + h <= height_) {
        return true;
    }
    // Start new shelf
    if (shelfHeight_ + h <= height_ && w <= width_) {
        cursorX_ = 0;
        cursorY_ = shelfHeight_;
        return true;
    }
    return false;
}

bool GlyphAtlas::Grow() {
    int newWidth  = std::min(width_ * 2, MAX_SIZE);
    int newHeight = std::min(height_ * 2, MAX_SIZE);
    if (newWidth == width_ && newHeight == height_) return false;

    std::vector<uint8_t> newPixels(newWidth * newHeight, 0);
    // Copy old data
    for (int y = 0; y < height_; ++y) {
        std::memcpy(&newPixels[y * newWidth],
                    &pixels_[y * width_],
                    width_);
    }

    // Update UV coordinates for all existing regions
    float scaleU = static_cast<float>(width_)  / static_cast<float>(newWidth);
    float scaleV = static_cast<float>(height_) / static_cast<float>(newHeight);
    for (auto& pair : regions_) {
        pair.second.u0 *= scaleU;
        pair.second.u1 *= scaleU;
        pair.second.v0 *= scaleV;
        pair.second.v1 *= scaleV;
    }

    pixels_ = std::move(newPixels);
    width_  = newWidth;
    height_ = newHeight;
    dirty_  = true;
    ++generation_;
    return true;
}

} // namespace font
} // namespace koilo
