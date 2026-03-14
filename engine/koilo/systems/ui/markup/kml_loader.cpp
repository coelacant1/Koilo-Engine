// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file kml_loader.cpp
 * @brief KML/KSS file loading and build orchestration.
 * @date 03/08/2026
 * @author Coela Can't
 */

#include "kml_loader.hpp"
#include <fstream>
#include <sstream>
#include <cstdio>

namespace koilo {
namespace ui {
namespace markup {

// Construct loader with UI context and theme references
KMLLoader::KMLLoader(UIContext& ctx, Theme& theme)
    : ctx_(ctx), theme_(theme) {}

// ============================================================================
// File I/O
// ============================================================================

// Read entire file contents into a string
bool KMLLoader::ReadFile(const std::string& path, std::string& out) {
    std::ifstream file(path, std::ios::in | std::ios::binary);
    if (!file.is_open()) return false;
    std::ostringstream ss;
    ss << file.rdbuf();
    out = ss.str();
    return true;
}

// ============================================================================
// Public API
// ============================================================================

// Parse markup + stylesheet strings and build the widget tree
int KMLLoader::LoadFromString(const std::string& kml, const std::string& kss) {
    errors_.clear();
    widgetsCreated_ = 0;

    // Parse markup
    KMLParser markupParser;
    if (!markupParser.Parse(kml)) {
        errors_ = markupParser.Errors();
        return -1;
    }

    // Parse stylesheet (if provided)
    KSSStylesheet styles;
    if (!kss.empty()) {
        KSSParser styleParser;
        if (!styleParser.Parse(kss)) {
            errors_ = styleParser.Errors();
            return -1;
        }
        styles = styleParser.Stylesheet();
    }

    // Build widget tree
    KMLBuilder builder(ctx_, theme_);
    BuildResult result = builder.Build(markupParser.Elements(), styles);

    errors_ = result.errors;
    widgetsCreated_ = result.widgetsCreated;
    return result.rootWidgetIdx;
}

// Load KML/KSS from file paths and build the widget tree
int KMLLoader::LoadFromFile(const std::string& kmlPath, const std::string& kssPath) {
    std::string kml, kss;

    if (!ReadFile(kmlPath, kml)) {
        errors_.push_back({0, 0, "Failed to open KML file: " + kmlPath});
        return -1;
    }

    if (!kssPath.empty()) {
        if (!ReadFile(kssPath, kss)) {
            errors_.push_back({0, 0, "Failed to open KSS file: " + kssPath});
            return -1;
        }
    }

    return LoadFromString(kml, kss);
}

// Parse a standalone stylesheet string for later reuse
bool KMLLoader::LoadStylesheet(const std::string& kss) {
    KSSParser parser;
    if (!parser.Parse(kss)) {
        errors_ = parser.Errors();
        return false;
    }
    stylesheet_ = parser.Stylesheet();
    return true;
}

} // namespace markup
} // namespace ui
} // namespace koilo
