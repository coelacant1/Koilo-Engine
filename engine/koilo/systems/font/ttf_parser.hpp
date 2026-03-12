// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file ttf_parser.hpp
 * @brief TrueType font binary format parser.
 *
 * Parses TTF files from raw bytes: offset table, cmap (format 4),
 * head, maxp, hhea, hmtx, loca, glyf tables.  Extracts glyph
 * outlines as quadratic Bézier contours and horizontal metrics.
 *
 * @date 03/08/2026
 * @author Coela Can't
 */

#pragma once

#include <koilo/core/math/vector2d.hpp>
#include <cstdint>
#include <cstring>
#include <vector>
#include "../../registry/reflect_macros.hpp"

namespace koilo {
namespace font {

// --- Binary reader (big-endian TTF) ---------------------------------

class BinaryReader {
public:
    BinaryReader(const uint8_t* data, size_t size)
        : data_(data), size_(size), pos_(0) {}

    bool Valid() const { return data_ != nullptr && pos_ <= size_; }
    size_t Position() const { return pos_; }
    void Seek(size_t offset) { pos_ = offset; }

    uint8_t ReadU8() {
        if (pos_ + 1 > size_) return 0;
        return data_[pos_++];
    }
    int8_t ReadI8() { return static_cast<int8_t>(ReadU8()); }

    uint16_t ReadU16() {
        if (pos_ + 2 > size_) return 0;
        uint16_t v = (static_cast<uint16_t>(data_[pos_]) << 8) |
                      static_cast<uint16_t>(data_[pos_ + 1]);
        pos_ += 2;
        return v;
    }
    int16_t ReadI16() { return static_cast<int16_t>(ReadU16()); }

    uint32_t ReadU32() {
        if (pos_ + 4 > size_) return 0;
        uint32_t v = (static_cast<uint32_t>(data_[pos_])     << 24) |
                     (static_cast<uint32_t>(data_[pos_ + 1]) << 16) |
                     (static_cast<uint32_t>(data_[pos_ + 2]) << 8)  |
                      static_cast<uint32_t>(data_[pos_ + 3]);
        pos_ += 4;
        return v;
    }
    int32_t ReadI32() { return static_cast<int32_t>(ReadU32()); }

    void Skip(size_t n) { pos_ += n; }

    const uint8_t* DataAt(size_t offset) const {
        return (offset < size_) ? data_ + offset : nullptr;
    }
    size_t Size() const { return size_; }

private:
    const uint8_t* data_;
    size_t size_;
    size_t pos_;

    KL_BEGIN_FIELDS(BinaryReader)
        KL_FIELD(BinaryReader, size_, "Size", 0, 0),
        KL_FIELD(BinaryReader, pos_, "Position", 0, 0)
    KL_END_FIELDS

    KL_BEGIN_METHODS(BinaryReader)
        KL_METHOD_AUTO(BinaryReader, Position, "Position"),
        KL_METHOD_AUTO(BinaryReader, Seek, "Seek"),
        KL_METHOD_AUTO(BinaryReader, ReadU8, "Read u8"),
        KL_METHOD_AUTO(BinaryReader, ReadI8, "Read i8"),
        KL_METHOD_AUTO(BinaryReader, ReadU16, "Read u16"),
        KL_METHOD_AUTO(BinaryReader, ReadI16, "Read i16"),
        KL_METHOD_AUTO(BinaryReader, ReadU32, "Read u32"),
        KL_METHOD_AUTO(BinaryReader, ReadI32, "Read i32"),
        KL_METHOD_AUTO(BinaryReader, Skip, "Skip"),
        KL_METHOD_AUTO(BinaryReader, DataAt, "Data at"),
        KL_METHOD_AUTO(BinaryReader, Size, "Size")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(BinaryReader)
        /* Constructor takes pointer - not reflectable. */
    KL_END_DESCRIBE(BinaryReader)

};

// --- Glyph outline types --------------------------------------------

struct GlyphPoint {
    float x = 0.0f;
    float y = 0.0f;
    bool onCurve = true;

    KL_BEGIN_FIELDS(GlyphPoint)
        KL_FIELD(GlyphPoint, x, "X", 0, 0),
        KL_FIELD(GlyphPoint, y, "Y", 0, 0),
        KL_FIELD(GlyphPoint, onCurve, "On curve", 0, 1)
    KL_END_FIELDS

    KL_BEGIN_METHODS(GlyphPoint)
        /* No reflected methods. */
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(GlyphPoint)
        /* No reflected ctors. */
    KL_END_DESCRIBE(GlyphPoint)

};

struct GlyphContour {
    std::vector<GlyphPoint> points;

    KL_BEGIN_FIELDS(GlyphContour)
        KL_FIELD(GlyphContour, points, "Points", 0, 0)
    KL_END_FIELDS

    KL_BEGIN_METHODS(GlyphContour)
        /* No reflected methods. */
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(GlyphContour)
        /* No reflected ctors. */
    KL_END_DESCRIBE(GlyphContour)

};

struct GlyphOutline {
    std::vector<GlyphContour> contours;
    float xMin = 0.0f, yMin = 0.0f;
    float xMax = 0.0f, yMax = 0.0f;
    float advanceWidth = 0.0f;
    float leftSideBearing = 0.0f;
    bool valid = false;

    KL_BEGIN_FIELDS(GlyphOutline)
        KL_FIELD(GlyphOutline, xMin, "X min", 0, 0),
        KL_FIELD(GlyphOutline, yMin, "Y min", 0, 0),
        KL_FIELD(GlyphOutline, xMax, "X max", 0, 0),
        KL_FIELD(GlyphOutline, yMax, "Y max", 0, 0),
        KL_FIELD(GlyphOutline, advanceWidth, "Advance width", 0, 0),
        KL_FIELD(GlyphOutline, leftSideBearing, "Left side bearing", 0, 0),
        KL_FIELD(GlyphOutline, valid, "Valid", 0, 1)
    KL_END_FIELDS

    KL_BEGIN_METHODS(GlyphOutline)
        /* No reflected methods. */
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(GlyphOutline)
        /* No reflected ctors. */
    KL_END_DESCRIBE(GlyphOutline)

};

// --- Parsed font data -----------------------------------------------

struct FontMetrics {
    uint16_t unitsPerEm = 0;
    int16_t  ascent = 0;
    int16_t  descent = 0;
    int16_t  lineGap = 0;
    int16_t  xMin = 0, yMin = 0, xMax = 0, yMax = 0;

    KL_BEGIN_FIELDS(FontMetrics)
        /* No reflected fields. */
    KL_END_FIELDS

    KL_BEGIN_METHODS(FontMetrics)
        /* No reflected methods. */
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(FontMetrics)
        /* No reflected ctors. */
    KL_END_DESCRIBE(FontMetrics)

};

struct HMetric {
    uint16_t advanceWidth = 0;
    int16_t  leftSideBearing = 0;

    KL_BEGIN_FIELDS(HMetric)
        /* No reflected fields. */
    KL_END_FIELDS

    KL_BEGIN_METHODS(HMetric)
        /* No reflected methods. */
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(HMetric)
        /* No reflected ctors. */
    KL_END_DESCRIBE(HMetric)

};

// --- Table directory ------------------------------------------------

struct TableRecord {
    uint32_t tag = 0;
    uint32_t checksum = 0;
    uint32_t offset = 0;
    uint32_t length = 0;

    KL_BEGIN_FIELDS(TableRecord)
        /* No reflected fields. */
    KL_END_FIELDS

    KL_BEGIN_METHODS(TableRecord)
        /* No reflected methods. */
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(TableRecord)
        /* No reflected ctors. */
    KL_END_DESCRIBE(TableRecord)

};

// --- TTF Parser -----------------------------------------------------

class TTFParser {
public:
    /// Parse raw TTF data. Returns false on invalid/unsupported data.
    bool Parse(const uint8_t* data, size_t size);

    bool IsParsed() const { return parsed_; }
    const FontMetrics& Metrics() const { return metrics_; }
    uint16_t NumGlyphs() const { return numGlyphs_; }

    /// Map Unicode codepoint -> glyph ID (0 = .notdef / missing).
    uint16_t GetGlyphId(uint32_t codepoint) const;

    /// Get horizontal metrics for a glyph ID.
    HMetric GetHMetric(uint16_t glyphId) const;

    /// Parse glyph outline by glyph ID.
    GlyphOutline GetGlyphOutline(uint16_t glyphId) const;

    /// Get kern advance between two glyph IDs (from kern table if present).
    int16_t GetKernAdvance(uint16_t left, uint16_t right) const;

private:
    const uint8_t* data_ = nullptr;
    size_t size_ = 0;
    bool parsed_ = false;

    std::vector<TableRecord> tables_;
    FontMetrics metrics_;
    uint16_t numGlyphs_ = 0;
    uint16_t numOfLongHorMetrics_ = 0;

    // hmtx
    std::vector<HMetric> hmetrics_;
    std::vector<int16_t> extraLSBs_;

    // loca
    std::vector<uint32_t> glyphOffsets_;
    uint32_t glyfTableOffset_ = 0;
    int16_t indexToLocFormat_ = 0;

    // cmap format 4
    uint16_t cmapSegCount_ = 0;
    std::vector<uint16_t> cmapEndCode_;
    std::vector<uint16_t> cmapStartCode_;
    std::vector<int16_t>  cmapIdDelta_;
    std::vector<uint16_t> cmapIdRangeOffset_;
    std::vector<uint16_t> cmapGlyphIdArray_;

    // kern
    struct KernPair {
        uint16_t left, right;
        int16_t value;

        KL_BEGIN_FIELDS(KernPair)
            KL_FIELD(KernPair, right, "Right", 0, 0),
            KL_FIELD(KernPair, value, "Value", -32768, 32767)
        KL_END_FIELDS

        KL_BEGIN_METHODS(KernPair)
            /* No reflected methods. */
        KL_END_METHODS

        KL_BEGIN_DESCRIBE(KernPair)
            /* No reflected ctors. */
        KL_END_DESCRIBE(KernPair)

    };
    std::vector<KernPair> kernPairs_;

    // --- Table lookup -------------------------------------------

    const TableRecord* FindTable(uint32_t tag) const;

    static constexpr uint32_t MakeTag(char a, char b, char c, char d) {
        return (static_cast<uint32_t>(a) << 24) |
               (static_cast<uint32_t>(b) << 16) |
               (static_cast<uint32_t>(c) << 8)  |
                static_cast<uint32_t>(d);
    }

    // --- Table parsers ------------------------------------------

    bool ParseHead();
    bool ParseMaxp();
    bool ParseHhea();
    bool ParseHmtx();
    bool ParseLoca();
    bool ParseCmap();
    void ParseKern();

    // --- Glyph parsing ------------------------------------------

    // Glyf flag bits
    static constexpr uint8_t ON_CURVE       = 0x01;
    static constexpr uint8_t X_SHORT        = 0x02;
    static constexpr uint8_t Y_SHORT        = 0x04;
    static constexpr uint8_t REPEAT_FLAG    = 0x08;
    static constexpr uint8_t X_SAME_OR_POS  = 0x10;
    static constexpr uint8_t Y_SAME_OR_POS  = 0x20;

    void ParseSimpleGlyph(BinaryReader& r, int16_t numberOfContours,
                          GlyphOutline& outline) const;

    // Composite glyph flags
    static constexpr uint16_t ARG_1_AND_2_ARE_WORDS  = 0x0001;
    static constexpr uint16_t ARGS_ARE_XY_VALUES      = 0x0002;
    static constexpr uint16_t WE_HAVE_A_SCALE         = 0x0008;
    static constexpr uint16_t MORE_COMPONENTS          = 0x0020;
    static constexpr uint16_t WE_HAVE_AN_XY_SCALE     = 0x0040;
    static constexpr uint16_t WE_HAVE_A_TWO_BY_TWO    = 0x0080;

    void ParseCompositeGlyph(BinaryReader& r, GlyphOutline& outline) const;

    KL_BEGIN_FIELDS(TTFParser)
        /* No reflected fields. */
    KL_END_FIELDS

    KL_BEGIN_METHODS(TTFParser)
        KL_METHOD_AUTO(TTFParser, IsParsed, "Is parsed"),
        KL_METHOD_AUTO(TTFParser, NumGlyphs, "Num glyphs")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(TTFParser)
        /* No reflected ctors. */
    KL_END_DESCRIBE(TTFParser)

};

} // namespace font
} // namespace koilo
