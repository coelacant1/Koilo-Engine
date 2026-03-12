// SPDX-License-Identifier: GPL-3.0-or-later
#include <koilo/systems/font/ttf_parser.hpp>

namespace koilo {
namespace font {

// --- TTFParser public -----------------------------------------------

bool TTFParser::Parse(const uint8_t* data, size_t size) {
    data_ = data;
    size_ = size;
    BinaryReader r(data, size);

    // Offset table
    uint32_t scalerType = r.ReadU32();
    (void)scalerType;
    uint16_t numTables = r.ReadU16();
    r.Skip(6); // searchRange, entrySelector, rangeShift

    // Read table directory
    tables_.clear();
    for (uint16_t i = 0; i < numTables; ++i) {
        TableRecord tr;
        tr.tag = r.ReadU32();
        tr.checksum = r.ReadU32();
        tr.offset = r.ReadU32();
        tr.length = r.ReadU32();
        tables_.push_back(tr);
    }

    if (!ParseHead()) return false;
    if (!ParseMaxp()) return false;
    if (!ParseHhea()) return false;
    if (!ParseHmtx()) return false;
    if (!ParseLoca()) return false;
    if (!ParseCmap()) return false;

    parsed_ = true;
    return true;
}

uint16_t TTFParser::GetGlyphId(uint32_t codepoint) const {
    // Binary search the cmap format 4 segments
    if (cmapSegCount_ == 0 || codepoint > 0xFFFF) return 0;
    uint16_t cp = static_cast<uint16_t>(codepoint);

    int lo = 0, hi = static_cast<int>(cmapSegCount_) - 1;
    while (lo <= hi) {
        int mid = (lo + hi) / 2;
        if (cp > cmapEndCode_[mid]) {
            lo = mid + 1;
        } else if (mid > 0 && cp < cmapStartCode_[mid]) {
            hi = mid - 1;
        } else {
            if (cp < cmapStartCode_[mid]) return 0;
            // Found segment
            if (cmapIdRangeOffset_[mid] == 0) {
                return static_cast<uint16_t>(
                    (cp + cmapIdDelta_[mid]) & 0xFFFF);
            } else {
                size_t idx = cmapIdRangeOffset_[mid] / 2 +
                             (cp - cmapStartCode_[mid]) -
                             (cmapSegCount_ - mid);
                if (idx < cmapGlyphIdArray_.size()) {
                    uint16_t gid = cmapGlyphIdArray_[idx];
                    if (gid != 0) {
                        return static_cast<uint16_t>(
                            (gid + cmapIdDelta_[mid]) & 0xFFFF);
                    }
                }
                return 0;
            }
        }
    }
    return 0;
}

HMetric TTFParser::GetHMetric(uint16_t glyphId) const {
    if (glyphId < hmetrics_.size()) return hmetrics_[glyphId];
    if (!hmetrics_.empty()) {
        HMetric m;
        m.advanceWidth = hmetrics_.back().advanceWidth;
        if (glyphId < extraLSBs_.size() + hmetrics_.size()) {
            m.leftSideBearing = extraLSBs_[glyphId - hmetrics_.size()];
        }
        return m;
    }
    return {};
}

GlyphOutline TTFParser::GetGlyphOutline(uint16_t glyphId) const {
    GlyphOutline outline;
    if (!parsed_ || glyphId >= numGlyphs_) return outline;
    if (static_cast<size_t>(glyphId + 1) >= glyphOffsets_.size()) return outline;

    uint32_t off = glyfTableOffset_ + glyphOffsets_[glyphId];
    uint32_t nextOff = glyfTableOffset_ + glyphOffsets_[glyphId + 1];
    if (off == nextOff) {
        // Empty glyph (e.g. space)
        outline.valid = true;
        HMetric hm = GetHMetric(glyphId);
        outline.advanceWidth = static_cast<float>(hm.advanceWidth);
        outline.leftSideBearing = static_cast<float>(hm.leftSideBearing);
        return outline;
    }

    BinaryReader r(data_, size_);
    r.Seek(off);

    int16_t numberOfContours = r.ReadI16();
    outline.xMin = static_cast<float>(r.ReadI16());
    outline.yMin = static_cast<float>(r.ReadI16());
    outline.xMax = static_cast<float>(r.ReadI16());
    outline.yMax = static_cast<float>(r.ReadI16());

    if (numberOfContours >= 0) {
        ParseSimpleGlyph(r, numberOfContours, outline);
    } else {
        ParseCompositeGlyph(r, outline);
    }

    HMetric hm = GetHMetric(glyphId);
    outline.advanceWidth = static_cast<float>(hm.advanceWidth);
    outline.leftSideBearing = static_cast<float>(hm.leftSideBearing);
    outline.valid = true;
    return outline;
}

int16_t TTFParser::GetKernAdvance(uint16_t left, uint16_t right) const {
    for (size_t i = 0; i < kernPairs_.size(); ++i) {
        if (kernPairs_[i].left == left && kernPairs_[i].right == right) {
            return kernPairs_[i].value;
        }
    }
    return 0;
}

// --- TTFParser private ----------------------------------------------

const TableRecord* TTFParser::FindTable(uint32_t tag) const {
    for (size_t i = 0; i < tables_.size(); ++i) {
        if (tables_[i].tag == tag) return &tables_[i];
    }
    return nullptr;
}

bool TTFParser::ParseHead() {
    const TableRecord* rec = FindTable(MakeTag('h','e','a','d'));
    if (!rec) return false;
    BinaryReader r(data_, size_);
    r.Seek(rec->offset);

    r.Skip(18); // version, fontRevision, checksumAdj, magicNumber, flags
    metrics_.unitsPerEm = r.ReadU16();
    r.Skip(16); // created, modified
    metrics_.xMin = r.ReadI16();
    metrics_.yMin = r.ReadI16();
    metrics_.xMax = r.ReadI16();
    metrics_.yMax = r.ReadI16();
    r.Skip(6); // macStyle, lowestRecPPEM, fontDirectionHint
    indexToLocFormat_ = r.ReadI16();
    return metrics_.unitsPerEm > 0;
}

bool TTFParser::ParseMaxp() {
    const TableRecord* rec = FindTable(MakeTag('m','a','x','p'));
    if (!rec) return false;
    BinaryReader r(data_, size_);
    r.Seek(rec->offset);
    r.Skip(4); // version
    numGlyphs_ = r.ReadU16();
    return numGlyphs_ > 0;
}

bool TTFParser::ParseHhea() {
    const TableRecord* rec = FindTable(MakeTag('h','h','e','a'));
    if (!rec) return false;
    BinaryReader r(data_, size_);
    r.Seek(rec->offset);
    r.Skip(4); // version
    metrics_.ascent  = r.ReadI16();
    metrics_.descent = r.ReadI16();
    metrics_.lineGap = r.ReadI16();
    r.Skip(24); // advanceWidthMax through reserved fields
    numOfLongHorMetrics_ = r.ReadU16();
    return true;
}

bool TTFParser::ParseHmtx() {
    const TableRecord* rec = FindTable(MakeTag('h','m','t','x'));
    if (!rec) return false;
    BinaryReader r(data_, size_);
    r.Seek(rec->offset);

    hmetrics_.resize(numOfLongHorMetrics_);
    for (uint16_t i = 0; i < numOfLongHorMetrics_; ++i) {
        hmetrics_[i].advanceWidth = r.ReadU16();
        hmetrics_[i].leftSideBearing = r.ReadI16();
    }

    uint16_t remaining = numGlyphs_ - numOfLongHorMetrics_;
    extraLSBs_.resize(remaining);
    for (uint16_t i = 0; i < remaining; ++i) {
        extraLSBs_[i] = r.ReadI16();
    }
    return true;
}

bool TTFParser::ParseLoca() {
    const TableRecord* rec = FindTable(MakeTag('l','o','c','a'));
    if (!rec) return false;
    const TableRecord* glyf = FindTable(MakeTag('g','l','y','f'));
    if (!glyf) return false;
    glyfTableOffset_ = glyf->offset;

    BinaryReader r(data_, size_);
    r.Seek(rec->offset);

    glyphOffsets_.resize(numGlyphs_ + 1);
    if (indexToLocFormat_ == 0) {
        // Short format: uint16 offsets, multiply by 2
        for (uint16_t i = 0; i <= numGlyphs_; ++i) {
            glyphOffsets_[i] = static_cast<uint32_t>(r.ReadU16()) * 2;
        }
    } else {
        // Long format: uint32 offsets
        for (uint16_t i = 0; i <= numGlyphs_; ++i) {
            glyphOffsets_[i] = r.ReadU32();
        }
    }
    return true;
}

bool TTFParser::ParseCmap() {
    const TableRecord* rec = FindTable(MakeTag('c','m','a','p'));
    if (!rec) return false;
    BinaryReader r(data_, size_);
    r.Seek(rec->offset);

    r.Skip(2); // version
    uint16_t numSubtables = r.ReadU16();

    // Find Unicode BMP subtable (platform 3/encoding 1 or platform 0)
    uint32_t subtableOffset = 0;
    for (uint16_t i = 0; i < numSubtables; ++i) {
        uint16_t platformID = r.ReadU16();
        uint16_t encodingID = r.ReadU16();
        uint32_t offset = r.ReadU32();
        if ((platformID == 3 && encodingID == 1) ||
            (platformID == 0 && subtableOffset == 0)) {
            subtableOffset = rec->offset + offset;
        }
    }
    if (subtableOffset == 0) return false;

    r.Seek(subtableOffset);
    uint16_t format = r.ReadU16();
    if (format != 4) return false; // Only format 4 (BMP) supported

    r.Skip(4); // length, language
    uint16_t segCountX2 = r.ReadU16();
    cmapSegCount_ = segCountX2 / 2;
    r.Skip(6); // searchRange, entrySelector, rangeShift

    cmapEndCode_.resize(cmapSegCount_);
    for (uint16_t i = 0; i < cmapSegCount_; ++i) {
        cmapEndCode_[i] = r.ReadU16();
    }
    r.Skip(2); // reservedPad

    cmapStartCode_.resize(cmapSegCount_);
    for (uint16_t i = 0; i < cmapSegCount_; ++i) {
        cmapStartCode_[i] = r.ReadU16();
    }

    cmapIdDelta_.resize(cmapSegCount_);
    for (uint16_t i = 0; i < cmapSegCount_; ++i) {
        cmapIdDelta_[i] = r.ReadI16();
    }

    cmapIdRangeOffset_.resize(cmapSegCount_);
    for (uint16_t i = 0; i < cmapSegCount_; ++i) {
        cmapIdRangeOffset_[i] = r.ReadU16();
    }

    // Read remaining glyph ID array
    size_t tableEnd = rec->offset + rec->length;
    size_t arrayStart = r.Position();
    size_t arrayCount = 0;
    if (tableEnd > arrayStart) {
        // Calculate from the subtable length field
        r.Seek(subtableOffset + 2);
        uint16_t subtableLen = r.ReadU16();
        size_t subtableEnd = subtableOffset + subtableLen;
        if (subtableEnd > arrayStart) {
            arrayCount = (subtableEnd - arrayStart) / 2;
        }
    }
    cmapGlyphIdArray_.resize(arrayCount);
    r.Seek(arrayStart);
    for (size_t i = 0; i < arrayCount; ++i) {
        cmapGlyphIdArray_[i] = r.ReadU16();
    }

    // Try to parse kern table (optional)
    ParseKern();

    return true;
}

void TTFParser::ParseKern() {
    const TableRecord* rec = FindTable(MakeTag('k','e','r','n'));
    if (!rec) return;
    BinaryReader r(data_, size_);
    r.Seek(rec->offset);

    uint16_t version = r.ReadU16();
    if (version != 0) return; // Only version 0 supported
    uint16_t numSubtables = r.ReadU16();

    for (uint16_t s = 0; s < numSubtables; ++s) {
        r.Skip(2); // version
        uint16_t length = r.ReadU16();
        uint16_t coverage = r.ReadU16();
        bool horizontal = (coverage & 1) != 0;
        uint8_t format = static_cast<uint8_t>(coverage >> 8);

        if (!horizontal || format != 0) {
            r.Skip(length - 6);
            continue;
        }

        uint16_t nPairs = r.ReadU16();
        r.Skip(6); // searchRange, entrySelector, rangeShift

        for (uint16_t i = 0; i < nPairs; ++i) {
            KernPair kp;
            kp.left = r.ReadU16();
            kp.right = r.ReadU16();
            kp.value = r.ReadI16();
            kernPairs_.push_back(kp);
        }
    }
}

// --- Glyph parsing -------------------------------------------------

void TTFParser::ParseSimpleGlyph(BinaryReader& r, int16_t numberOfContours,
                                  GlyphOutline& outline) const {
    // Read contour end points
    std::vector<uint16_t> endPts(numberOfContours);
    for (int16_t i = 0; i < numberOfContours; ++i) {
        endPts[i] = r.ReadU16();
    }

    uint16_t numPoints = (numberOfContours > 0) ? endPts.back() + 1 : 0;

    // Skip instructions
    uint16_t instrLen = r.ReadU16();
    r.Skip(instrLen);

    // Read flags
    std::vector<uint8_t> flags(numPoints);
    for (uint16_t i = 0; i < numPoints; ) {
        uint8_t flag = r.ReadU8();
        flags[i++] = flag;
        if (flag & REPEAT_FLAG) {
            uint8_t repeatCount = r.ReadU8();
            for (uint8_t j = 0; j < repeatCount && i < numPoints; ++j) {
                flags[i++] = flag;
            }
        }
    }

    // Read X coordinates (delta-encoded)
    std::vector<int16_t> xCoords(numPoints);
    int16_t xVal = 0;
    for (uint16_t i = 0; i < numPoints; ++i) {
        if (flags[i] & X_SHORT) {
            uint8_t delta = r.ReadU8();
            xVal += (flags[i] & X_SAME_OR_POS) ?
                    static_cast<int16_t>(delta) :
                   -static_cast<int16_t>(delta);
        } else {
            if (!(flags[i] & X_SAME_OR_POS)) {
                xVal += r.ReadI16();
            }
            // else: same as previous (delta = 0)
        }
        xCoords[i] = xVal;
    }

    // Read Y coordinates (delta-encoded)
    std::vector<int16_t> yCoords(numPoints);
    int16_t yVal = 0;
    for (uint16_t i = 0; i < numPoints; ++i) {
        if (flags[i] & Y_SHORT) {
            uint8_t delta = r.ReadU8();
            yVal += (flags[i] & Y_SAME_OR_POS) ?
                    static_cast<int16_t>(delta) :
                   -static_cast<int16_t>(delta);
        } else {
            if (!(flags[i] & Y_SAME_OR_POS)) {
                yVal += r.ReadI16();
            }
        }
        yCoords[i] = yVal;
    }

    // Build contours
    outline.contours.resize(numberOfContours);
    uint16_t ptIdx = 0;
    for (int16_t c = 0; c < numberOfContours; ++c) {
        uint16_t end = endPts[c];
        auto& contour = outline.contours[c];
        for (; ptIdx <= end; ++ptIdx) {
            GlyphPoint gp;
            gp.x = static_cast<float>(xCoords[ptIdx]);
            gp.y = static_cast<float>(yCoords[ptIdx]);
            gp.onCurve = (flags[ptIdx] & ON_CURVE) != 0;
            contour.points.push_back(gp);
        }
    }
}

void TTFParser::ParseCompositeGlyph(BinaryReader& r, GlyphOutline& outline) const {
    uint16_t flags;
    do {
        flags = r.ReadU16();
        uint16_t glyphIndex = r.ReadU16();

        float dx = 0.0f, dy = 0.0f;
        float a = 1.0f, b = 0.0f, c = 0.0f, d = 1.0f;

        if (flags & ARG_1_AND_2_ARE_WORDS) {
            if (flags & ARGS_ARE_XY_VALUES) {
                dx = static_cast<float>(r.ReadI16());
                dy = static_cast<float>(r.ReadI16());
            } else {
                r.Skip(4); // point indices, not supported
            }
        } else {
            if (flags & ARGS_ARE_XY_VALUES) {
                dx = static_cast<float>(r.ReadI8());
                dy = static_cast<float>(r.ReadI8());
            } else {
                r.Skip(2); // point indices
            }
        }

        if (flags & WE_HAVE_A_SCALE) {
            a = d = static_cast<float>(r.ReadI16()) / 16384.0f;
        } else if (flags & WE_HAVE_AN_XY_SCALE) {
            a = static_cast<float>(r.ReadI16()) / 16384.0f;
            d = static_cast<float>(r.ReadI16()) / 16384.0f;
        } else if (flags & WE_HAVE_A_TWO_BY_TWO) {
            a = static_cast<float>(r.ReadI16()) / 16384.0f;
            b = static_cast<float>(r.ReadI16()) / 16384.0f;
            c = static_cast<float>(r.ReadI16()) / 16384.0f;
            d = static_cast<float>(r.ReadI16()) / 16384.0f;
        }

        // Recursively get component outline
        GlyphOutline component = GetGlyphOutline(glyphIndex);
        for (auto& contour : component.contours) {
            for (auto& pt : contour.points) {
                float nx = a * pt.x + b * pt.y + dx;
                float ny = c * pt.x + d * pt.y + dy;
                pt.x = nx;
                pt.y = ny;
            }
            outline.contours.push_back(std::move(contour));
        }
    } while (flags & MORE_COMPONENTS);
}

} // namespace font
} // namespace koilo
