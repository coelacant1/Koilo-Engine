// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file kml_parser.hpp
 * @brief HTML-like markup parser for KML layout files.
 *
 * Parses a restricted HTML subset into a KMLElement tree.
 * Supports: elements, attributes, text content, self-closing tags,
 * nesting, and <!-- comments -->.
 *
 * @date 03/09/2026
 * @author Coela
 */

#pragma once

#include "kml_types.hpp"

namespace koilo {
namespace ui {
namespace markup {

class KMLParser {
public:
    /// Parse markup string into an element tree.
    /// Returns true on success. On failure, errors() has diagnostics.
    bool Parse(const std::string& source);

    /// The parsed root elements (typically one root element).
    const std::vector<KMLElement>& Elements() const { return roots_; }

    /// Parse errors (empty on success).
    const std::vector<ParseError>& Errors() const { return errors_; }

private:
    // Scanner state
    const char* src_ = nullptr;
    const char* end_ = nullptr;
    const char* pos_ = nullptr;
    int line_ = 1;
    int col_ = 1;

    std::vector<KMLElement> roots_;
    std::vector<ParseError> errors_;

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
