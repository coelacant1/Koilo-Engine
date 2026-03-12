// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file rasterizer.hpp
 * @brief Glyph outline rasterizer with anti-aliasing.
 *
 * Converts quadratic Bézier glyph contours to alpha bitmaps using
 * scanline rendering with vertical supersampling. The edge list and
 * coverage approach produces smooth anti-aliased text without
 * requiring an external library.
 *
 * @date 03/08/2026
 * @author Coela Can't
 */

#pragma once

#include <koilo/systems/font/ttf_parser.hpp>
#include <cstdint>
#include <vector>
#include "../../registry/reflect_macros.hpp"

namespace koilo {
namespace font {

// --- Rasterized glyph bitmap ----------------------------------------

struct GlyphBitmap {
    std::vector<uint8_t> pixels;  // alpha values (0-255)
    int width  = 0;
    int height = 0;
    int originX = 0;  // pixel offset from glyph origin to bitmap left
    int originY = 0;  // pixel offset from glyph origin to bitmap top

    uint8_t Sample(int x, int y) const {
        if (x < 0 || x >= width || y < 0 || y >= height) return 0;
        return pixels[y * width + x];
    }

    KL_BEGIN_FIELDS(GlyphBitmap)
        KL_FIELD(GlyphBitmap, pixels, "Pixels", 0, 0)
    KL_END_FIELDS

    KL_BEGIN_METHODS(GlyphBitmap)
        /* No reflected methods. */
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(GlyphBitmap)
        /* No reflected ctors. */
    KL_END_DESCRIBE(GlyphBitmap)

};

// --- Edge representation for scanline fill --------------------------

struct Edge {
    float x0, y0, x1, y1;

    KL_BEGIN_FIELDS(Edge)
        KL_FIELD(Edge, y1, "Y1", 0, 0)
    KL_END_FIELDS

    KL_BEGIN_METHODS(Edge)
        /* No reflected methods. */
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(Edge)
        /* No reflected ctors. */
    KL_END_DESCRIBE(Edge)

};

// --- Glyph rasterizer -----------------------------------------------

class GlyphRasterizer {
public:
    static constexpr int SUPERSAMPLE = 8;  // vertical supersampling factor
    static constexpr int MAX_GLYPH_SIZE = 512;

    /// Rasterize a glyph outline at the given scale.
    /// @param outline Parsed glyph contours in font units.
    /// @param scale   Pixels per font unit (pixelSize / unitsPerEm).
    /// @param padding Extra pixels around glyph for atlas spacing.
    GlyphBitmap Rasterize(const GlyphOutline& outline, float scale,
                          int padding = 1) const;

private:
    mutable std::vector<float> intersections_;

    /// Build edge list by flattening Bézier contours to line segments.
    void BuildEdges(const GlyphOutline& outline, float scale,
                    std::vector<Edge>& edges) const;

    /// Flatten a quadratic Bézier into line segment edges.
    void FlattenQuadratic(float x0, float y0, float cx, float cy,
                          float x1, float y1,
                          std::vector<Edge>& edges,
                          int depth = 0) const;

    KL_BEGIN_FIELDS(GlyphRasterizer)
        /* No reflected fields. */
    KL_END_FIELDS

    KL_BEGIN_METHODS(GlyphRasterizer)
        /* No reflected methods. */
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(GlyphRasterizer)
        /* No reflected ctors. */
    KL_END_DESCRIBE(GlyphRasterizer)

};

} // namespace font
} // namespace koilo
