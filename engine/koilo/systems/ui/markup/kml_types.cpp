// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file kml_types.cpp
 * @brief Value parsing utilities for KML/KSS types.
 * @date 03/09/2026
 * @author Coela
 */

#include "kml_types.hpp"
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <sstream>

namespace koilo {
namespace ui {
namespace markup {

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static std::string Trim(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

static std::string ToLower(std::string s) {
    for (auto& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

static int HexDigit(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

// ---------------------------------------------------------------------------
// Color parsing
// ---------------------------------------------------------------------------

bool ParseColor(const std::string& raw, Color& out) {
    std::string str = Trim(raw);
    if (str.empty()) return false;

    // #RGB, #RRGGBB, #RRGGBBAA
    if (str[0] == '#') {
        str = str.substr(1);
        if (str.size() == 3) {
            int r = HexDigit(str[0]), g = HexDigit(str[1]), b = HexDigit(str[2]);
            if (r < 0 || g < 0 || b < 0) return false;
            out = {(uint8_t)(r*17), (uint8_t)(g*17), (uint8_t)(b*17), 255};
            return true;
        }
        if (str.size() == 6 || str.size() == 8) {
            int r = HexDigit(str[0]) * 16 + HexDigit(str[1]);
            int g = HexDigit(str[2]) * 16 + HexDigit(str[3]);
            int b = HexDigit(str[4]) * 16 + HexDigit(str[5]);
            if (r < 0 || g < 0 || b < 0) return false;
            int a = 255;
            if (str.size() == 8) {
                a = HexDigit(str[6]) * 16 + HexDigit(str[7]);
                if (a < 0) return false;
            }
            out = {(uint8_t)r, (uint8_t)g, (uint8_t)b, (uint8_t)a};
            return true;
        }
        return false;
    }

    // rgb(r, g, b) or rgba(r, g, b, a)
    std::string lower = ToLower(str);
    bool isRgba = lower.substr(0, 5) == "rgba(";
    bool isRgb  = !isRgba && lower.substr(0, 4) == "rgb(";
    if (isRgb || isRgba) {
        size_t open = str.find('(');
        size_t close = str.rfind(')');
        if (open == std::string::npos || close == std::string::npos) return false;
        std::string inner = str.substr(open + 1, close - open - 1);
        // Replace commas with spaces for uniform tokenization
        for (auto& c : inner) { if (c == ',') c = ' '; }
        std::istringstream iss(inner);
        float vals[4] = {0, 0, 0, 255};
        int count = 0;
        float v;
        while (iss >> v && count < 4) { vals[count++] = v; }
        if ((isRgb && count < 3) || (isRgba && count < 4)) return false;
        out = {(uint8_t)vals[0], (uint8_t)vals[1], (uint8_t)vals[2], (uint8_t)vals[3]};
        return true;
    }

    // Named colors (minimal set)
    if (lower == "transparent") { out = {0,0,0,0}; return true; }
    if (lower == "white")       { out = {255,255,255,255}; return true; }
    if (lower == "black")       { out = {0,0,0,255}; return true; }
    if (lower == "red")         { out = {255,0,0,255}; return true; }
    if (lower == "green")       { out = {0,128,0,255}; return true; }
    if (lower == "blue")        { out = {0,0,255,255}; return true; }
    if (lower == "gray" || lower == "grey") { out = {128,128,128,255}; return true; }

    return false;
}

// ---------------------------------------------------------------------------
// Size value parsing
// ---------------------------------------------------------------------------

bool ParseSizeValue(const std::string& raw, SizeValue& out) {
    std::string str = Trim(ToLower(raw));
    if (str.empty()) return false;

    if (str == "fill") {
        out = {0.0f, SizeUnit::Fill};
        return true;
    }
    if (str == "fit-content") {
        out = {0.0f, SizeUnit::FitContent};
        return true;
    }
    if (str == "auto") {
        out = {0.0f, SizeUnit::Auto};
        return true;
    }

    // Percentage: "50%"
    if (str.back() == '%') {
        char* end = nullptr;
        float val = std::strtof(str.c_str(), &end);
        if (end == str.c_str()) return false;
        out = {val, SizeUnit::Percent};
        return true;
    }

    // Pixels: "100px" or bare "100"
    // Also handle em, rem, vw, vh
    std::string numStr = str;
    SizeUnit unit = SizeUnit::Px;
    if (str.size() > 2 && str.substr(str.size() - 2) == "px") {
        numStr = str.substr(0, str.size() - 2);
    } else if (str.size() > 2 && str.substr(str.size() - 2) == "em") {
        numStr = str.substr(0, str.size() - 2);
        unit = SizeUnit::Em;
    } else if (str.size() > 3 && str.substr(str.size() - 3) == "rem") {
        numStr = str.substr(0, str.size() - 3);
        unit = SizeUnit::Rem;
    } else if (str.size() > 2 && str.substr(str.size() - 2) == "vw") {
        numStr = str.substr(0, str.size() - 2);
        unit = SizeUnit::Vw;
    } else if (str.size() > 2 && str.substr(str.size() - 2) == "vh") {
        numStr = str.substr(0, str.size() - 2);
        unit = SizeUnit::Vh;
    }
    char* end = nullptr;
    float val = std::strtof(numStr.c_str(), &end);
    if (end == numStr.c_str()) return false;
    out = {val, unit};
    return true;
}

// ---------------------------------------------------------------------------
// Edge shorthand parsing
// ---------------------------------------------------------------------------

bool ParseEdges(const std::string& raw, EdgesValue& out) {
    std::string str = Trim(raw);
    if (str.empty()) return false;

    // Tokenize by spaces (values may have "px" suffix)
    std::vector<float> vals;
    std::istringstream iss(str);
    std::string token;
    while (iss >> token) {
        SizeValue sv;
        if (!ParseSizeValue(token, sv)) return false;
        vals.push_back(sv.number);
    }

    switch (vals.size()) {
        case 1:  // all sides
            out = {vals[0], vals[0], vals[0], vals[0]};
            return true;
        case 2:  // vertical horizontal
            out = {vals[0], vals[1], vals[0], vals[1]};
            return true;
        case 3:  // top horizontal bottom
            out = {vals[0], vals[1], vals[2], vals[1]};
            return true;
        case 4:  // top right bottom left
            out = {vals[0], vals[1], vals[2], vals[3]};
            return true;
        default:
            return false;
    }
}

// ---------------------------------------------------------------------------
// Selector specificity
// ---------------------------------------------------------------------------

int KSSSelector::Specificity() const {
    int spec = 0;
    for (const auto& p : parts) {
        if (!p.id.empty())         spec += 100;
        spec += static_cast<int>(p.classNames.size()) * 10;
        if (!p.pseudoClass.empty()) spec += 10;
        if (!p.element.empty() && p.element != "*") spec += 1;
    }
    return spec;
}

} // namespace markup
} // namespace ui
} // namespace koilo
