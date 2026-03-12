// SPDX-License-Identifier: GPL-3.0-or-later
/// @file font.cpp
/// Out-of-class definitions for Font and FontManager.

#include <koilo/systems/font/font.hpp>

#include <cstring>
#include <fstream>

namespace koilo {
namespace font {

// --- Font -----------------------------------------------------------

bool Font::LoadFromMemory(const uint8_t* data, size_t size, float pixelSize) {
    fontData_.assign(data, data + size);
    pixelSize_ = pixelSize;

    if (!parser_.Parse(fontData_.data(), fontData_.size())) {
        return false;
    }

    const auto& m = parser_.Metrics();
    scale_ = pixelSize_ / static_cast<float>(m.unitsPerEm);
    ascent_  = static_cast<float>(m.ascent) * scale_;
    descent_ = static_cast<float>(m.descent) * scale_;
    lineGap_ = static_cast<float>(m.lineGap) * scale_;
    lineHeight_ = ascent_ - descent_ + lineGap_;

    loaded_ = true;
    return true;
}

bool Font::LoadFromFile(const char* path, float pixelSize) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) return false;

    auto fileSize = file.tellg();
    if (fileSize <= 0) return false;
    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> data(static_cast<size_t>(fileSize));
    file.read(reinterpret_cast<char*>(data.data()), fileSize);
    if (!file) return false;

    return LoadFromMemory(data.data(), data.size(), pixelSize);
}

const GlyphRegion* Font::GetGlyph(uint32_t codepoint) {
    return GetGlyphAtSize(codepoint, pixelSize_);
}

const GlyphRegion* Font::GetGlyphAtSize(uint32_t codepoint, float targetPixelSize) {
    if (!loaded_) return nullptr;

    uint16_t sizeKey = static_cast<uint16_t>(targetPixelSize + 0.5f);
    GlyphKey key{codepoint, sizeKey};
    const GlyphRegion* cached = atlas_.Find(key);
    if (cached) return cached;

    float targetScale = targetPixelSize /
        static_cast<float>(parser_.Metrics().unitsPerEm);

    uint16_t glyphId = parser_.GetGlyphId(codepoint);
    GlyphOutline outline = parser_.GetGlyphOutline(glyphId);

    GlyphBitmap bmp;
    if (outline.valid) {
        bmp = rasterizer_.Rasterize(outline, targetScale);
    }

    float bearingX = -static_cast<float>(bmp.originX);
    float bearingY = static_cast<float>(bmp.originY);
    float advance  = outline.advanceWidth * targetScale;

    return atlas_.Add(key, bmp, bearingX, bearingY, advance);
}

float Font::GetKerning(uint32_t left, uint32_t right) const {
    if (!loaded_) return 0.0f;
    uint16_t l = parser_.GetGlyphId(left);
    uint16_t r = parser_.GetGlyphId(right);
    return static_cast<float>(parser_.GetKernAdvance(l, r)) * scale_;
}

TextMetrics Font::MeasureText(const char* text) const {
    return MeasureText(text, std::strlen(text));
}

TextMetrics Font::MeasureText(const char* text, size_t len) const {
    TextMetrics tm;
    if (!loaded_ || len == 0) return tm;

    tm.ascent  = ascent_;
    tm.descent = descent_;
    tm.lineGap = lineGap_;
    tm.height  = lineHeight_;
    tm.numLines = 1;

    float lineWidth = 0.0f;
    float maxWidth = 0.0f;
    uint32_t prevCp = 0;

    size_t i = 0;
    while (i < len) {
        uint32_t cp = DecodeUTF8(text, len, i);
        if (cp == '\n') {
            if (lineWidth > maxWidth) maxWidth = lineWidth;
            lineWidth = 0.0f;
            tm.numLines++;
            prevCp = 0;
            continue;
        }

        uint16_t glyphId = parser_.GetGlyphId(cp);
        HMetric hm = parser_.GetHMetric(glyphId);
        float advance = static_cast<float>(hm.advanceWidth) * scale_;

        if (prevCp != 0) {
            float kern = static_cast<float>(
                parser_.GetKernAdvance(
                    parser_.GetGlyphId(prevCp), glyphId)) * scale_;
            lineWidth += kern;
        }

        lineWidth += advance;
        prevCp = cp;
    }

    if (lineWidth > maxWidth) maxWidth = lineWidth;
    tm.width = maxWidth;
    tm.height = lineHeight_ * tm.numLines;
    return tm;
}

uint32_t Font::DecodeUTF8(const char* text, size_t len, size_t& i) {
    uint8_t c = static_cast<uint8_t>(text[i]);
    uint32_t cp = 0;

    if (c < 0x80) {
        cp = c;
        i += 1;
    } else if ((c & 0xE0) == 0xC0) {
        if (i + 1 >= len) { i = len; return 0xFFFD; }
        cp = (static_cast<uint32_t>(c & 0x1F) << 6) |
             (static_cast<uint32_t>(text[i+1] & 0x3F));
        i += 2;
    } else if ((c & 0xF0) == 0xE0) {
        if (i + 2 >= len) { i = len; return 0xFFFD; }
        cp = (static_cast<uint32_t>(c & 0x0F) << 12) |
             (static_cast<uint32_t>(text[i+1] & 0x3F) << 6) |
             (static_cast<uint32_t>(text[i+2] & 0x3F));
        i += 3;
    } else if ((c & 0xF8) == 0xF0) {
        if (i + 3 >= len) { i = len; return 0xFFFD; }
        cp = (static_cast<uint32_t>(c & 0x07) << 18) |
             (static_cast<uint32_t>(text[i+1] & 0x3F) << 12) |
             (static_cast<uint32_t>(text[i+2] & 0x3F) << 6) |
             (static_cast<uint32_t>(text[i+3] & 0x3F));
        i += 4;
    } else {
        i += 1; // invalid, skip
        return 0xFFFD;
    }
    return cp;
}

// --- FontManager ----------------------------------------------------

Font* FontManager::LoadFont(const char* path, float pixelSize) {
    FontCacheKey key{path, pixelSize};
    auto it = cache_.find(key);
    if (it != cache_.end()) return it->second.get();

    auto font = std::make_unique<Font>();
    if (!font->LoadFromFile(path, pixelSize)) {
        return nullptr;
    }

    Font* ptr = font.get();
    cache_.emplace(key, std::move(font));
    return ptr;
}

Font* FontManager::LoadFromMemory(const char* name, const uint8_t* data,
                                  size_t size, float pixelSize) {
    FontCacheKey key{name, pixelSize};
    auto it = cache_.find(key);
    if (it != cache_.end()) return it->second.get();

    auto font = std::make_unique<Font>();
    if (!font->LoadFromMemory(data, size, pixelSize)) {
        return nullptr;
    }

    Font* ptr = font.get();
    cache_.emplace(key, std::move(font));
    return ptr;
}

Font* FontManager::GetFont(const char* path, float pixelSize) const {
    FontCacheKey key{path, pixelSize};
    auto it = cache_.find(key);
    return (it != cache_.end()) ? it->second.get() : nullptr;
}

void FontManager::Clear() { cache_.clear(); }

} // namespace font
} // namespace koilo
