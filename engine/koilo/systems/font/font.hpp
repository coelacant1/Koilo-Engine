// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file font.hpp
 * @brief High-level font loading, glyph caching, and text measurement.
 *
 * Font wraps the TTF parser, rasterizer, and glyph atlas into a
 * single class that provides glyph lookup and text metrics.
 * FontManager caches loaded fonts and provides the engine-wide API.
 *
 * @date 03/08/2026
 * @author Coela Can't
 */

#pragma once

#include <koilo/systems/font/ttf_parser.hpp>
#include <koilo/systems/font/rasterizer.hpp>
#include <koilo/systems/font/glyph_atlas.hpp>
#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include "../../registry/reflect_macros.hpp"

namespace koilo {
namespace font {

// --- Text measurement result ----------------------------------------

struct TextMetrics {
    float width    = 0.0f;  // total advance width in pixels
    float height   = 0.0f;  // line height (ascent - descent + lineGap)
    float ascent   = 0.0f;  // pixels above baseline
    float descent  = 0.0f;  // pixels below baseline (positive = down)
    float lineGap  = 0.0f;  // extra spacing between lines
    int   numLines = 1;

    KL_BEGIN_FIELDS(TextMetrics)
        KL_FIELD(TextMetrics, width, "Width", 0, 0),
        KL_FIELD(TextMetrics, height, "Height", 0, 0),
        KL_FIELD(TextMetrics, ascent, "Ascent", 0, 0),
        KL_FIELD(TextMetrics, descent, "Descent", 0, 0),
        KL_FIELD(TextMetrics, lineGap, "Line gap", 0, 0),
        KL_FIELD(TextMetrics, numLines, "Num lines", 1, 65535)
    KL_END_FIELDS

    KL_BEGIN_METHODS(TextMetrics)
        /* No reflected methods. */
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(TextMetrics)
        /* No reflected ctors. */
    KL_END_DESCRIBE(TextMetrics)

};

// --- Font -----------------------------------------------------------

class Font {
public:
    Font() = default;
    ~Font() = default;

    // Non-copyable, movable
    Font(const Font&) = delete;
    Font& operator=(const Font&) = delete;
    Font(Font&&) = default;
    Font& operator=(Font&&) = default;

    /// Load TTF from raw bytes. Takes ownership of a copy.
    bool LoadFromMemory(const uint8_t* data, size_t size, float pixelSize);

    /// Load TTF from file path.
    bool LoadFromFile(const char* path, float pixelSize);

    bool IsLoaded() const { return loaded_; }
    float PixelSize() const { return pixelSize_; }
    float Scale() const { return scale_; }
    float Ascent() const { return ascent_; }
    float Descent() const { return descent_; }
    float LineGap() const { return lineGap_; }
    float LineHeight() const { return lineHeight_; }
    const FontMetrics& Metrics() const { return parser_.Metrics(); }

    /// Get (or rasterize and cache) a glyph for a Unicode codepoint.
    const GlyphRegion* GetGlyph(uint32_t codepoint);

    /// Get (or rasterize and cache) a glyph at a specific pixel size.
    const GlyphRegion* GetGlyphAtSize(uint32_t codepoint, float targetPixelSize);

    /// Get kerning between two codepoints in pixels.
    float GetKerning(uint32_t left, uint32_t right) const;

    /// Measure text dimensions without rendering.
    TextMetrics MeasureText(const char* text) const;
    TextMetrics MeasureText(const char* text, size_t len) const;

    /// Access the glyph atlas (for GPU texture upload).
    GlyphAtlas& Atlas() { return atlas_; }
    const GlyphAtlas& Atlas() const { return atlas_; }

    /// Access the parser (for advanced queries).
    const TTFParser& Parser() const { return parser_; }

private:
    std::vector<uint8_t> fontData_;
    TTFParser parser_;
    GlyphRasterizer rasterizer_;
    GlyphAtlas atlas_;

    float pixelSize_  = 0.0f;
    float scale_      = 0.0f;
    float ascent_     = 0.0f;
    float descent_    = 0.0f;
    float lineGap_    = 0.0f;
    float lineHeight_ = 0.0f;
    bool  loaded_     = false;

    /// Decode one UTF-8 codepoint, advance index.
    static uint32_t DecodeUTF8(const char* text, size_t len, size_t& i);

    KL_BEGIN_FIELDS(Font)
        /* No reflected fields. */
    KL_END_FIELDS

    KL_BEGIN_METHODS(Font)
        KL_METHOD_AUTO(Font, IsLoaded, "Is loaded"),
        KL_METHOD_AUTO(Font, PixelSize, "Pixel size"),
        KL_METHOD_AUTO(Font, Scale, "Scale"),
        KL_METHOD_AUTO(Font, Ascent, "Ascent"),
        KL_METHOD_AUTO(Font, Descent, "Descent"),
        KL_METHOD_AUTO(Font, LineGap, "Line gap"),
        KL_METHOD_AUTO(Font, LineHeight, "Line height")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(Font)
        KL_CTOR0(Font)
    KL_END_DESCRIBE(Font)

};

// --- Font cache key -------------------------------------------------

struct FontCacheKey {
    std::string path;
    float pixelSize;

    bool operator==(const FontCacheKey& o) const {
        return path == o.path && pixelSize == o.pixelSize;
    }

    KL_BEGIN_FIELDS(FontCacheKey)
        KL_FIELD(FontCacheKey, path, "Path", 0, 0),
        KL_FIELD(FontCacheKey, pixelSize, "Pixel size", __FLT_MIN__, __FLT_MAX__)
    KL_END_FIELDS

    KL_BEGIN_METHODS(FontCacheKey)
        /* No reflected methods. */
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(FontCacheKey)
        /* No reflected ctors. */
    KL_END_DESCRIBE(FontCacheKey)

};

struct FontCacheKeyHash {
    size_t operator()(const FontCacheKey& k) const {
        size_t h = std::hash<std::string>{}(k.path);
        h ^= std::hash<float>{}(k.pixelSize) + 0x9e3779b9 + (h << 6) + (h >> 2);
        return h;
    }

    KL_BEGIN_FIELDS(FontCacheKeyHash)
        /* No reflected fields. */
    KL_END_FIELDS

    KL_BEGIN_METHODS(FontCacheKeyHash)
        /* No reflected methods. */
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(FontCacheKeyHash)
        /* No reflected ctors. */
    KL_END_DESCRIBE(FontCacheKeyHash)

};

// --- Font manager ---------------------------------------------------

class FontManager {
public:
    FontManager() = default;
    ~FontManager() = default;

    // Non-copyable
    FontManager(const FontManager&) = delete;
    FontManager& operator=(const FontManager&) = delete;

    /// Load a font at a given pixel size. Returns nullptr on failure.
    /// Caches by (path, size) - repeated calls return the same font.
    Font* LoadFont(const char* path, float pixelSize);

    /// Load a font from raw bytes (for embedded fonts).
    Font* LoadFromMemory(const char* name, const uint8_t* data,
                         size_t size, float pixelSize);

    /// Get a previously loaded font.
    Font* GetFont(const char* path, float pixelSize) const;

    /// Release all cached fonts.
    void Clear();

    size_t CacheSize() const { return cache_.size(); }

private:
    std::unordered_map<FontCacheKey, std::unique_ptr<Font>, FontCacheKeyHash> cache_;

    KL_BEGIN_FIELDS(FontManager)
        /* No reflected fields. */
    KL_END_FIELDS

    KL_BEGIN_METHODS(FontManager)
        KL_METHOD_AUTO(FontManager, CacheSize, "Cache size")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(FontManager)
        KL_CTOR0(FontManager)
    KL_END_DESCRIBE(FontManager)

};

} // namespace font
} // namespace koilo
