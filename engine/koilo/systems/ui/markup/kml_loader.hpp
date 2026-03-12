// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file kml_loader.hpp
 * @brief Top-level API for loading KML layouts and KSS stylesheets.
 *
 * Provides file I/O and a single-call interface for parsing markup +
 * stylesheet and building the widget tree.
 *
 * @date 03/09/2026
 * @author Coela
 */

#pragma once

#include "kml_types.hpp"
#include "kml_parser.hpp"
#include "kss_parser.hpp"
#include "kml_builder.hpp"
#include <string>

namespace koilo {
namespace ui {
namespace markup {

class KMLLoader {
public:
    explicit KMLLoader(UIContext& ctx, Theme& theme);

    /// Load and build a layout from a KML string.
    /// Optionally applies a KSS stylesheet string.
    /// Returns the root widget index, or -1 on failure.
    int LoadFromString(const std::string& kml,
                       const std::string& kss = "");

    /// Load and build a layout from file paths.
    /// The stylesheet path may be empty (no styles applied).
    int LoadFromFile(const std::string& kmlPath,
                     const std::string& kssPath = "");

    /// Load only a stylesheet from string for later use.
    bool LoadStylesheet(const std::string& kss);

    /// Get the last loaded stylesheet (for reuse across multiple layouts).
    const KSSStylesheet& Stylesheet() const { return stylesheet_; }

    /// Parse errors from the most recent operation.
    const std::vector<ParseError>& Errors() const { return errors_; }

    /// Number of widgets created by the last Build operation.
    int WidgetsCreated() const { return widgetsCreated_; }

private:
    UIContext& ctx_;
    Theme& theme_;
    KSSStylesheet stylesheet_;
    std::vector<ParseError> errors_;
    int widgetsCreated_ = 0;

    static bool ReadFile(const std::string& path, std::string& out);
};

} // namespace markup
} // namespace ui
} // namespace koilo
