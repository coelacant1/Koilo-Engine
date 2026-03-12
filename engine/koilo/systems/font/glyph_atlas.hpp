// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file glyph_atlas.hpp
 * @brief Glyph atlas texture packing for font rendering.
 *
 * Shelf-based packing of rasterized glyph bitmaps into a single
 * RGBA8 texture atlas.  Provides UV coordinates and metrics for
 * each cached glyph so the GPU renderer can batch text draws.
 *
 * @date 03/08/2026
 * @author Coela Can't
 */

#pragma once

#include <koilo/systems/font/rasterizer.hpp>
#include <cstdint>
#include <vector>
#include <unordered_map>
#include "../../registry/reflect_macros.hpp"

namespace koilo {
namespace font {

// --- Atlas glyph region ---------------------------------------------

struct GlyphRegion {
    uint16_t atlasX = 0;      // pixel position in atlas
    uint16_t atlasY = 0;
    uint16_t width  = 0;      // glyph bitmap size
    uint16_t height = 0;

    // UV coordinates (normalized 0-1)
    float u0 = 0.0f, v0 = 0.0f;
    float u1 = 0.0f, v1 = 0.0f;

    // Layout metrics (in pixels at rendered size)
    float bearingX = 0.0f;    // offset from pen position to left edge
    float bearingY = 0.0f;    // offset from baseline to top edge
    float advance  = 0.0f;    // pen advance after this glyph

    KL_BEGIN_FIELDS(GlyphRegion)
        KL_FIELD(GlyphRegion, width, "Width", 0, 65535),
        KL_FIELD(GlyphRegion, height, "Height", 0, 65535),
        KL_FIELD(GlyphRegion, u0, "U0", 0, 0),
        KL_FIELD(GlyphRegion, v0, "V0", 0, 0),
        KL_FIELD(GlyphRegion, u1, "U1", 0, 0),
        KL_FIELD(GlyphRegion, v1, "V1", 0, 0),
        KL_FIELD(GlyphRegion, bearingX, "Bearing X", 0, 0),
        KL_FIELD(GlyphRegion, bearingY, "Bearing Y", 0, 0),
        KL_FIELD(GlyphRegion, advance, "Advance", 0, 0)
    KL_END_FIELDS

    KL_BEGIN_METHODS(GlyphRegion)
        /* No reflected methods. */
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(GlyphRegion)
        /* No reflected ctors. */
    KL_END_DESCRIBE(GlyphRegion)

};

// --- Atlas key (codepoint + size) -----------------------------------

struct GlyphKey {
    uint32_t codepoint = 0;
    uint16_t pixelSize = 0;

    bool operator==(const GlyphKey& o) const {
        return codepoint == o.codepoint && pixelSize == o.pixelSize;
    }

    KL_BEGIN_FIELDS(GlyphKey)
        /* No reflected fields. */
    KL_END_FIELDS

    KL_BEGIN_METHODS(GlyphKey)
        /* No reflected methods. */
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(GlyphKey)
        /* No reflected ctors. */
    KL_END_DESCRIBE(GlyphKey)

};

struct GlyphKeyHash {
    size_t operator()(const GlyphKey& k) const {
        return static_cast<size_t>(k.codepoint) ^
               (static_cast<size_t>(k.pixelSize) << 16);
    }

    KL_BEGIN_FIELDS(GlyphKeyHash)
        /* No reflected fields. */
    KL_END_FIELDS

    KL_BEGIN_METHODS(GlyphKeyHash)
        /* No reflected methods. */
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(GlyphKeyHash)
        /* No reflected ctors. */
    KL_END_DESCRIBE(GlyphKeyHash)

};

// --- Glyph atlas ----------------------------------------------------

class GlyphAtlas {
public:
    static constexpr int INITIAL_SIZE = 512;
    static constexpr int MAX_SIZE = 4096;
    static constexpr int GLYPH_PADDING = 1;

    GlyphAtlas() : width_(INITIAL_SIZE), height_(INITIAL_SIZE) {
        pixels_.resize(width_ * height_, 0);
    }

    explicit GlyphAtlas(int size) : width_(size), height_(size) {
        pixels_.resize(width_ * height_, 0);
    }

    int Width()  const { return width_; }
    int Height() const { return height_; }
    bool IsDirty() const { return dirty_; }
    void ClearDirty() { dirty_ = false; }
    int Generation() const { return generation_; }

    /// Raw alpha pixel data for GPU texture upload.
    const uint8_t* Pixels() const { return pixels_.data(); }

    /// Check if a glyph is already cached.
    const GlyphRegion* Find(const GlyphKey& key) const;

    /// Add a rasterized glyph bitmap to the atlas.
    /// Returns pointer to the region, or nullptr if atlas is full.
    const GlyphRegion* Add(const GlyphKey& key, const GlyphBitmap& bmp,
                           float bearingX, float bearingY, float advance);

    /// Clear all cached glyphs and reset atlas.
    void Clear();

    size_t GlyphCount() const { return regions_.size(); }

private:
    int width_;
    int height_;
    std::vector<uint8_t> pixels_;
    std::unordered_map<GlyphKey, GlyphRegion, GlyphKeyHash> regions_;

    int cursorX_ = 0;      // current X position on shelf
    int cursorY_ = 0;      // top of current shelf
    int shelfHeight_ = 0;  // bottom of current shelf

    bool dirty_ = false;
    int generation_ = 0;

    /// Try to place a glyph of size (w,h) in the atlas.
    bool TryPlace(int w, int h);

    /// Double atlas size (up to MAX_SIZE). Returns false if already max.
    bool Grow();

    KL_BEGIN_FIELDS(GlyphAtlas)
        /* No reflected fields. */
    KL_END_FIELDS

    KL_BEGIN_METHODS(GlyphAtlas)
        KL_METHOD_AUTO(GlyphAtlas, Width, "Width"),
        KL_METHOD_AUTO(GlyphAtlas, Height, "Height"),
        KL_METHOD_AUTO(GlyphAtlas, IsDirty, "Is dirty"),
        KL_METHOD_AUTO(GlyphAtlas, ClearDirty, "Clear dirty"),
        KL_METHOD_AUTO(GlyphAtlas, Pixels, "Pixels"),
        KL_METHOD_AUTO(GlyphAtlas, Find, "Find"),
        KL_METHOD_AUTO(GlyphAtlas, GlyphCount, "Glyph count")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(GlyphAtlas)
        KL_CTOR0(GlyphAtlas)
    KL_END_DESCRIBE(GlyphAtlas)

};

} // namespace font
} // namespace koilo
