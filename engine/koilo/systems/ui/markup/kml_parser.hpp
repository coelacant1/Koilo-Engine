// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file kml_parser.hpp
 * @brief HTML-like markup parser for KML layout files.
 *
 * Parses a restricted HTML subset into a KMLElement tree.
 * Supports: elements, attributes, text content, self-closing tags,
 * nesting, and <!-- comments -->.
 *
 * @date 03/08/2026
 * @author Coela Can't
 */

#pragma once

#include "kml_types.hpp"

namespace koilo {
namespace ui {
namespace markup {

/** @class KMLParser @brief HTML-like markup parser for KML layout files. */
class KMLParser {
public:
    /**
     * @brief Parse markup string into an element tree.
     * @return true on success; on failure, Errors() has diagnostics.
     */
    bool Parse(const std::string& source);

    /** @brief The parsed root elements (typically one root element). */
    const std::vector<KMLElement>& Elements() const { return roots_; }

    /** @brief Parse errors (empty on success). */
    const std::vector<ParseError>& Errors() const { return errors_; }

private:
    const char* src_ = nullptr; ///< start of source buffer
    const char* end_ = nullptr; ///< end of source buffer
    const char* pos_ = nullptr; ///< current scan position
    int line_ = 1;              ///< current line number
    int col_ = 1;               ///< current column number

    std::vector<KMLElement> roots_;  ///< parsed root elements
    std::vector<ParseError> errors_; ///< accumulated parse errors

    // Character-level
    char Peek() const;
    char Advance();
    bool AtEnd() const;
    void SkipWhitespace();
    bool Match(const char* str);

    // Parsing
    void ParseContent(std::vector<KMLElement>& siblings);
    bool ParseElement(KMLElement& elem);
    bool ParseAttribute(KMLAttribute& attr);
    std::string ParseQuotedString();
    std::string ParseTagName();
    std::string ParseText();
    void SkipComment();

    void Error(const std::string& msg);
};

} // namespace markup
} // namespace ui
} // namespace koilo
