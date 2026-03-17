// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file kml_loader.hpp
 * @brief Top-level API for loading KML layouts and KSS stylesheets.
 *
 * Provides file I/O and a single-call interface for parsing markup +
 * stylesheet and building the widget tree.
 *
 * @date 03/08/2026
 * @author Coela Can't
 */

#pragma once

#include "kml_types.hpp"
#include "kml_parser.hpp"
#include "kss_parser.hpp"
#include "kml_builder.hpp"
#include <string>
#include "../../../registry/reflect_macros.hpp"

namespace koilo {
namespace ui {
namespace markup {

/** @class KMLLoader @brief Top-level API for loading KML layouts and KSS stylesheets. */
class KMLLoader {
public:
    explicit KMLLoader(UIContext& ctx, Theme& theme);

    /**
     * @brief Load and build a layout from a KML string.
     * @param kml  KML markup string.
     * @param kss  Optional KSS stylesheet string.
     * @return Root widget index, or -1 on failure.
     */
    int LoadFromString(const std::string& kml,
                       const std::string& kss = "");

    /**
     * @brief Load and build a layout from file paths.
     * @param kmlPath  Path to the KML file.
     * @param kssPath  Optional path to a KSS stylesheet.
     * @return Root widget index, or -1 on failure.
     */
    int LoadFromFile(const std::string& kmlPath,
                     const std::string& kssPath = "");

    /** @brief Load only a stylesheet from string for later use. */
    bool LoadStylesheet(const std::string& kss);

    /** @brief Get the last loaded stylesheet (for reuse across layouts). */
    const KSSStylesheet& Stylesheet() const { return stylesheet_; }

    /** @brief Parse errors from the most recent operation. */
    const std::vector<ParseError>& Errors() const { return errors_; }

    /** @brief Number of widgets created by the last Build operation. */
    int WidgetsCreated() const { return widgetsCreated_; }

private:
    UIContext& ctx_;                  ///< UI context for widget creation
    Theme& theme_;                    ///< theme for style resolution
    KSSStylesheet stylesheet_;        ///< last loaded stylesheet
    std::vector<ParseError> errors_;  ///< errors from last operation
    int widgetsCreated_ = 0;          ///< widgets created in last build

    static bool ReadFile(const std::string& path, std::string& out);

    KL_BEGIN_FIELDS(KMLLoader)
        /* No reflected fields. */
    KL_END_FIELDS

    KL_BEGIN_METHODS(KMLLoader)
        KL_METHOD_AUTO(KMLLoader, Errors, "Errors"),
        KL_METHOD_AUTO(KMLLoader, WidgetsCreated, "Widgets created")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(KMLLoader)
        /* Requires references - no reflected ctors. */
    KL_END_DESCRIBE(KMLLoader)

};

} // namespace markup
} // namespace ui
} // namespace koilo
