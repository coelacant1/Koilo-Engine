// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file kss_parser.hpp
 * @brief CSS-like stylesheet parser for KSS files.
 *
 * Parses selectors (element, .class, #id, :pseudo, descendant) and
 * declaration blocks into KSSRule structures.
 *
 * @date 03/08/2026
 * @author Coela Can't
 */

#pragma once

#include "kml_types.hpp"

namespace koilo {
namespace ui {
namespace markup {

/** @class KSSParser @brief CSS-like stylesheet parser for KSS files. */
class KSSParser {
public:
    /** @brief Parse a stylesheet string into rules. */
    bool Parse(const std::string& source);

    /** @brief The parsed stylesheet. */
    const KSSStylesheet& Stylesheet() const { return stylesheet_; }

    /** @brief Parse errors. */
    const std::vector<ParseError>& Errors() const { return errors_; }

private:
    const char* src_ = nullptr; ///< start of source buffer
    const char* end_ = nullptr; ///< end of source buffer
    const char* pos_ = nullptr; ///< current scan position
    int line_ = 1;              ///< current line number
    int col_  = 1;              ///< current column number

    KSSStylesheet stylesheet_;       ///< parsed stylesheet output
    std::vector<ParseError> errors_;  ///< accumulated parse errors

    char Peek() const;
    char Advance();
    bool AtEnd() const;
    void SkipWhitespace();
    void SkipLineComment();
    void SkipBlockComment();
    void SkipWhitespaceAndComments();

    bool ParseRule(KSSRule& rule);
    bool ParseSelectorList(std::vector<KSSSelector>& selectors);
    bool ParseSelector(KSSSelector& selector);
    bool ParseSimpleSelector(KSSSimpleSelector& simple);
    bool ParseDeclarationBlock(std::vector<KSSDeclaration>& decls);
    bool ParseDeclaration(KSSDeclaration& decl);
    std::string ParseIdent();
    std::string ParseValue();

    void Error(const std::string& msg);
};

} // namespace markup
} // namespace ui
} // namespace koilo
