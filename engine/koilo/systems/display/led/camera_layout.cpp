// SPDX-License-Identifier: GPL-3.0-or-later
#include <koilo/systems/display/led/camera_layout.hpp>

#include <koilo/systems/render/core/pixelgroup.hpp>
#include <koilo/core/math/vector2d.hpp>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>

namespace koilo {

// ---------------------------------------------------------------------------
// Minimal JSON parser (just enough for .klcam)
// ---------------------------------------------------------------------------

namespace {

/// Skip whitespace in a string starting at pos.
void SkipWS(const std::string& s, size_t& pos) {
    while (pos < s.size() && (s[pos] == ' ' || s[pos] == '\t' ||
                               s[pos] == '\n' || s[pos] == '\r')) {
        ++pos;
    }
}

/// Parse a JSON string literal (consumes quotes).
bool ParseString(const std::string& s, size_t& pos, std::string& out) {
    SkipWS(s, pos);
    if (pos >= s.size() || s[pos] != '"') return false;
    ++pos;
    out.clear();
    while (pos < s.size() && s[pos] != '"') {
        if (s[pos] == '\\' && pos + 1 < s.size()) {
            ++pos;
        }
        out += s[pos++];
    }
    if (pos >= s.size()) return false;
    ++pos; // closing quote
    return true;
}

/// Parse a JSON number as a double.
bool ParseNumber(const std::string& s, size_t& pos, double& out) {
    SkipWS(s, pos);
    size_t start = pos;
    if (pos < s.size() && (s[pos] == '-' || s[pos] == '+')) ++pos;
    while (pos < s.size() && ((s[pos] >= '0' && s[pos] <= '9') || s[pos] == '.' ||
                               s[pos] == 'e' || s[pos] == 'E' ||
                               s[pos] == '+' || s[pos] == '-')) {
        ++pos;
    }
    if (pos == start) return false;
    out = std::stod(s.substr(start, pos - start));
    return true;
}

/// Expect and consume a specific character.
bool Expect(const std::string& s, size_t& pos, char c) {
    SkipWS(s, pos);
    if (pos >= s.size() || s[pos] != c) return false;
    ++pos;
    return true;
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// Loading
// ---------------------------------------------------------------------------

bool LEDCameraLayout::LoadFromFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) return false;

    std::ostringstream ss;
    ss << file.rdbuf();
    return LoadFromString(ss.str());
}

bool LEDCameraLayout::LoadFromString(const std::string& json) {
    // Clear previous state.
    name_.clear();
    rawPositions_.clear();
    normalizedPositions_.clear();
    physicalSize_ = {};
    normExtent_ = 0.0f;

    // Expect top-level object.
    size_t pos = 0;
    if (!Expect(json, pos, '{')) return false;

    int parsedCount = -1;

    // Parse key-value pairs.
    while (true) {
        SkipWS(json, pos);
        if (pos >= json.size()) return false;
        if (json[pos] == '}') break;

        // Consume comma between entries.
        if (!rawPositions_.empty() || !name_.empty() || parsedCount >= 0) {
            if (json[pos] == ',') ++pos;
        }

        std::string key;
        if (!ParseString(json, pos, key)) return false;
        if (!Expect(json, pos, ':')) return false;

        if (key == "name") {
            if (!ParseString(json, pos, name_)) return false;
        } else if (key == "units") {
            std::string units;
            if (!ParseString(json, pos, units)) return false;
            // We only support mm currently; accept any string.
        } else if (key == "count") {
            double c = 0;
            if (!ParseNumber(json, pos, c)) return false;
            parsedCount = static_cast<int>(c);
        } else if (key == "positions") {
            // Parse array of [x, y] pairs.
            if (!Expect(json, pos, '[')) return false;

            while (true) {
                SkipWS(json, pos);
                if (pos >= json.size()) return false;
                if (json[pos] == ']') { ++pos; break; }
                if (!rawPositions_.empty()) {
                    if (json[pos] == ',') ++pos;
                }

                // Each position: [x, y]
                if (!Expect(json, pos, '[')) return false;
                double x = 0, y = 0;
                if (!ParseNumber(json, pos, x)) return false;
                if (!Expect(json, pos, ',')) return false;
                if (!ParseNumber(json, pos, y)) return false;
                if (!Expect(json, pos, ']')) return false;

                rawPositions_.emplace_back(static_cast<float>(x),
                                           static_cast<float>(y));
            }
        } else {
            // Skip unknown value (string, number, or nested).
            SkipWS(json, pos);
            if (pos < json.size() && json[pos] == '"') {
                std::string dummy;
                ParseString(json, pos, dummy);
            } else {
                double dummy = 0;
                ParseNumber(json, pos, dummy);
            }
        }
    }

    if (rawPositions_.empty()) return false;

    Normalize();
    return true;
}

// ---------------------------------------------------------------------------
// Normalization
// ---------------------------------------------------------------------------

void LEDCameraLayout::Normalize() {
    if (rawPositions_.empty()) return;

    // Find bounding box.
    float minX = rawPositions_[0].X, maxX = minX;
    float minY = rawPositions_[0].Y, maxY = minY;

    for (const auto& p : rawPositions_) {
        if (p.X < minX) minX = p.X;
        if (p.X > maxX) maxX = p.X;
        if (p.Y < minY) minY = p.Y;
        if (p.Y > maxY) maxY = p.Y;
    }

    const float width  = maxX - minX;
    const float height = maxY - minY;
    physicalSize_ = Vector2D(width, height);
    normExtent_ = std::max(width, height);

    normalizedPositions_.resize(rawPositions_.size());

    if (normExtent_ < 1e-6f) {
        // Degenerate case: all points at the same location.
        for (size_t i = 0; i < rawPositions_.size(); ++i) {
            normalizedPositions_[i] = Vector2D(0.5f, 0.5f);
        }
        return;
    }

    // Stretch-to-fill: independently normalize each axis to [0,1]
    // so LEDs sample the full framebuffer regardless of aspect ratio.
    // V is flipped because screen Y=0 is top but LED layout Y=0 is bottom.
    const float scaleX = (width  > 1e-6f) ? width  : 1.0f;
    const float scaleY = (height > 1e-6f) ? height : 1.0f;

    for (size_t i = 0; i < rawPositions_.size(); ++i) {
        normalizedPositions_[i] = Vector2D(
            (rawPositions_[i].X - minX) / scaleX,
            1.0f - (rawPositions_[i].Y - minY) / scaleY
        );
    }
}

// ---------------------------------------------------------------------------
// Axis flips
// ---------------------------------------------------------------------------

void LEDCameraLayout::FlipX() {
    for (auto& p : normalizedPositions_) {
        p.X = 1.0f - p.X;
    }
}

void LEDCameraLayout::FlipY() {
    for (auto& p : normalizedPositions_) {
        p.Y = 1.0f - p.Y;
    }
}

// ---------------------------------------------------------------------------
// Accessors
// ---------------------------------------------------------------------------

const Vector2D* LEDCameraLayout::GetNormalizedPositions() const {
    if (normalizedPositions_.empty()) return nullptr;
    return normalizedPositions_.data();
}

const Vector2D* LEDCameraLayout::GetRawPositions() const {
    if (rawPositions_.empty()) return nullptr;
    return rawPositions_.data();
}

PixelGroup* LEDCameraLayout::CreatePixelGroup() const {
    if (normalizedPositions_.empty()) return nullptr;
    return new PixelGroup(
        normalizedPositions_.data(),
        static_cast<uint32_t>(normalizedPositions_.size())
    );
}

} // namespace koilo
