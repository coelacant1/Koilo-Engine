// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file kss_parser.cpp
 * @brief CSS-like stylesheet parser implementation.
 * @date 03/08/2026
 * @author Coela Can't
 */

#include "kss_parser.hpp"
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cstring>

namespace koilo {
namespace ui {
namespace markup {

// ============================================================================
// Character-level
// ============================================================================

// Return current character without advancing
char KSSParser::Peek() const {
    return AtEnd() ? '\0' : *pos_;
}

// Consume and return current character, tracking line/column
char KSSParser::Advance() {
    if (AtEnd()) return '\0';
    char c = *pos_++;
    if (c == '\n') { ++line_; col_ = 1; }
    else { ++col_; }
    return c;
}

// Check if scanner has reached end of input
bool KSSParser::AtEnd() const {
    return pos_ >= end_;
}

// Skip over whitespace characters
void KSSParser::SkipWhitespace() {
    while (!AtEnd() && std::isspace(static_cast<unsigned char>(Peek()))) {
        Advance();
    }
}

// Skip a // line comment until newline
void KSSParser::SkipLineComment() {
    while (!AtEnd() && Peek() != '\n') Advance();
}

// Skip a /* ... */ block comment
void KSSParser::SkipBlockComment() {
    while (!AtEnd()) {
        if (Peek() == '*') {
            Advance();
            if (!AtEnd() && Peek() == '/') { Advance(); return; }
        } else {
            Advance();
        }
    }
    Error("Unterminated block comment");
}

// Skip whitespace, line comments, and block comments
void KSSParser::SkipWhitespaceAndComments() {
    while (!AtEnd()) {
        SkipWhitespace();
        if (AtEnd()) return;
        if (Peek() == '/' && (end_ - pos_) >= 2) {
            if (pos_[1] == '/') { Advance(); Advance(); SkipLineComment(); continue; }
            if (pos_[1] == '*') { Advance(); Advance(); SkipBlockComment(); continue; }
        }
        break;
    }
}

// Record a parse error at current position
void KSSParser::Error(const std::string& msg) {
    errors_.push_back({line_, col_, msg});
}

// ============================================================================
// Identifier parsing
// ============================================================================

// Parse a CSS identifier (supports -- prefix for custom properties)
std::string KSSParser::ParseIdent() {
    std::string id;
    // Allow -- prefix for CSS custom properties
    if (!AtEnd() && Peek() == '-') {
        id += Advance();
        if (!AtEnd() && Peek() == '-') {
            id += Advance();
        }
    }
    while (!AtEnd()) {
        char c = Peek();
        if (std::isalnum(static_cast<unsigned char>(c)) || c == '-' || c == '_') {
            id += static_cast<char>(std::tolower(static_cast<unsigned char>(Advance())));
        } else {
            break;
        }
    }
    return id;
}

// Read a property value until ';' or '}' (does not consume the delimiter)
std::string KSSParser::ParseValue() {
    std::string val;
    int parenDepth = 0;
    while (!AtEnd()) {
        char c = Peek();
        if (c == '(') { parenDepth++; val += Advance(); continue; }
        if (c == ')') { parenDepth--; val += Advance(); continue; }
        if (parenDepth == 0 && (c == ';' || c == '}')) break;
        val += Advance();
    }
    // Trim trailing whitespace
    while (!val.empty() && std::isspace(static_cast<unsigned char>(val.back())))
        val.pop_back();
    // Trim leading whitespace
    size_t start = val.find_first_not_of(" \t\r\n");
    return (start == std::string::npos) ? "" : val.substr(start);
}

// ============================================================================
// Selector parsing
// ============================================================================

// Parse a simple selector: element.class#id:pseudo[attr]
bool KSSParser::ParseSimpleSelector(KSSSimpleSelector& simple) {
    // A simple selector: element.class#id:pseudo (all parts optional, at least one required)
    bool hasAny = false;

    // Element name (or *)
    if (Peek() == '*') {
        simple.element = "*";
        Advance();
        hasAny = true;
    } else if (std::isalpha(static_cast<unsigned char>(Peek())) || Peek() == '_') {
        simple.element = ParseIdent();
        hasAny = true;
    }

    // .class (may chain: .a.b.c)
    while (!AtEnd() && Peek() == '.') {
        Advance();
        simple.classNames.push_back(ParseIdent());
        hasAny = true;
    }

    // #id
    if (!AtEnd() && Peek() == '#') {
        Advance();
        simple.id = ParseIdent();
        hasAny = true;
    }

    // :pseudo-class (may include functional pseudo-classes like :not(), :nth-child())
    while (!AtEnd() && Peek() == ':') {
        Advance();
        std::string pseudo = ParseIdent();
        if (pseudo == "not") {
            // :not(selector)
            if (!AtEnd() && Peek() == '(') {
                Advance(); // '('
                std::string notContent;
                int depth = 1;
                while (!AtEnd() && depth > 0) {
                    if (Peek() == '(') depth++;
                    if (Peek() == ')') { depth--; if (depth == 0) break; }
                    notContent += Advance();
                }
                if (!AtEnd()) Advance(); // ')'
                simple.negation = notContent;
            }
        } else if (pseudo == "nth-child") {
            // :nth-child(n) - store as pseudo class with argument
            if (!AtEnd() && Peek() == '(') {
                Advance();
                std::string arg;
                while (!AtEnd() && Peek() != ')') arg += Advance();
                if (!AtEnd()) Advance(); // ')'
                simple.pseudoClass = "nth-child(" + arg + ")";
            }
        } else {
            simple.pseudoClass = pseudo;
        }
        hasAny = true;
    }

    // [attr], [attr=val], [attr^=val], [attr$=val], [attr*=val]
    while (!AtEnd() && Peek() == '[') {
        Advance(); // '['
        SkipWhitespace();
        std::string attrName;
        while (!AtEnd() && Peek() != ']' && Peek() != '=' && Peek() != '^' &&
               Peek() != '$' && Peek() != '*' && Peek() != '~') {
            attrName += Advance();
        }
        // Trim
        while (!attrName.empty() && attrName.back() == ' ') attrName.pop_back();

        AttributeSelector as;
        as.name = attrName;
        as.mode = AttrMatchMode::Exists;

        if (!AtEnd() && Peek() != ']') {
            if (Peek() == '^') { as.mode = AttrMatchMode::Prefix; Advance(); }
            else if (Peek() == '$') { as.mode = AttrMatchMode::Suffix; Advance(); }
            else if (Peek() == '*') { as.mode = AttrMatchMode::Contains; Advance(); }
            else { as.mode = AttrMatchMode::Equals; }

            if (!AtEnd() && Peek() == '=') {
                Advance(); // '='
                SkipWhitespace();
                // Parse value (may be quoted)
                std::string val;
                if (!AtEnd() && (Peek() == '"' || Peek() == '\'')) {
                    char quote = Advance();
                    while (!AtEnd() && Peek() != quote) val += Advance();
                    if (!AtEnd()) Advance(); // closing quote
                } else {
                    while (!AtEnd() && Peek() != ']') val += Advance();
                    while (!val.empty() && val.back() == ' ') val.pop_back();
                }
                as.value = val;
            }
        }
        if (!AtEnd() && Peek() == ']') Advance();
        simple.attrSelectors.push_back(as);
        hasAny = true;
    }

    return hasAny;
}

// Parse a compound selector with combinators (space, >, +, ~)
bool KSSParser::ParseSelector(KSSSelector& selector) {
    // A selector is one or more simple selectors separated by whitespace
    // (descendant), > (child), + (adjacent sibling), or ~ (general sibling).
    // Stored in ancestor order: [0]=outermost, reversed later.
    std::vector<KSSSimpleSelector> parts;

    while (true) {
        KSSSimpleSelector simple;
        if (!ParseSimpleSelector(simple)) break;

        // Check for combinator
        bool hadSpace = false;
        while (!AtEnd() && Peek() == ' ') { Advance(); hadSpace = true; }

        CombinatorType comb = CombinatorType::Descendant;
        char c = Peek();
        if (c == '>') {
            comb = CombinatorType::Child;
            Advance();
            while (!AtEnd() && Peek() == ' ') Advance();
        } else if (c == '+') {
            comb = CombinatorType::AdjacentSibling;
            Advance();
            while (!AtEnd() && Peek() == ' ') Advance();
        } else if (c == '~') {
            comb = CombinatorType::GeneralSibling;
            Advance();
            while (!AtEnd() && Peek() == ' ') Advance();
        } else if (!hadSpace) {
            parts.push_back(std::move(simple));
            break;
        }

        simple.combinator = comb;
        parts.push_back(std::move(simple));

        // Peek: if next char starts a selector, continue; otherwise stop
        c = Peek();
        if (c == '{' || c == ',' || c == '\0' || c == ')') break;
    }

    if (parts.empty()) return false;

    // Reverse so [0] = target element (innermost)
    std::reverse(parts.begin(), parts.end());
    selector.parts = std::move(parts);
    return true;
}

// Parse comma-separated list of selectors
bool KSSParser::ParseSelectorList(std::vector<KSSSelector>& selectors) {
    while (true) {
        SkipWhitespaceAndComments();
        KSSSelector sel;
        if (!ParseSelector(sel)) {
            Error("Expected selector");
            return false;
        }
        selectors.push_back(std::move(sel));

        SkipWhitespaceAndComments();
        if (Peek() == ',') {
            Advance();
            continue;
        }
        break;
    }
    return !selectors.empty();
}

// ============================================================================
// Declaration parsing
// ============================================================================

// Parse a single property: value declaration
bool KSSParser::ParseDeclaration(KSSDeclaration& decl) {
    SkipWhitespaceAndComments();
    decl.property = ParseIdent();
    if (decl.property.empty()) return false;

    SkipWhitespaceAndComments();
    if (Peek() != ':') {
        Error("Expected ':' after property '" + decl.property + "'");
        return false;
    }
    Advance(); // ':'
    SkipWhitespace();

    decl.value = ParseValue();
    if (decl.value.empty()) {
        Error("Empty value for property '" + decl.property + "'");
        return false;
    }

    // Consume optional ';'
    if (!AtEnd() && Peek() == ';') Advance();

    return true;
}

// Parse a { property: value; ... } declaration block
bool KSSParser::ParseDeclarationBlock(std::vector<KSSDeclaration>& decls) {
    SkipWhitespaceAndComments();
    if (Peek() != '{') {
        Error("Expected '{' to open declaration block");
        return false;
    }
    Advance(); // '{'

    while (!AtEnd()) {
        SkipWhitespaceAndComments();
        if (Peek() == '}') {
            Advance();
            return true;
        }

        KSSDeclaration decl;
        if (!ParseDeclaration(decl)) {
            // Skip to next ';' or '}' on error
            while (!AtEnd() && Peek() != ';' && Peek() != '}') Advance();
            if (!AtEnd() && Peek() == ';') Advance();
            continue;
        }
        decls.push_back(std::move(decl));
    }

    Error("Unterminated declaration block (missing '}')");
    return false;
}

// ============================================================================
// Rule parsing
// ============================================================================

// Parse a complete rule: selector list + declaration block
bool KSSParser::ParseRule(KSSRule& rule) {
    SkipWhitespaceAndComments();
    if (AtEnd()) return false;

    if (!ParseSelectorList(rule.selectors)) return false;
    if (!ParseDeclarationBlock(rule.declarations)) return false;

    return true;
}

// ============================================================================
// Public API
// ============================================================================

// Parse a complete KSS stylesheet with rules, @media, @keyframes, and variables
bool KSSParser::Parse(const std::string& source) {
    stylesheet_.rules.clear();
    stylesheet_.variables.clear();
    errors_.clear();
    src_ = source.c_str();
    end_ = src_ + source.size();
    pos_ = src_;
    line_ = 1;
    col_ = 1;

    while (true) {
        SkipWhitespaceAndComments();
        if (AtEnd()) break;

        // Handle @-rules
        if (Peek() == '@') {
            Advance(); // '@'
            std::string keyword = ParseIdent();

            if (keyword == "media") {
                // @media (condition) { rules... }
                SkipWhitespace();
                // Parse media condition
                std::string condition;
                while (!AtEnd() && Peek() != '{') condition += Advance();
                if (!AtEnd()) Advance(); // '{'

                // Store condition for builder to evaluate
                MediaQuery mq;
                // Trim condition
                while (!condition.empty() && condition.back() == ' ') condition.pop_back();
                while (!condition.empty() && condition[0] == ' ') condition.erase(0, 1);
                // Parse simple conditions: (max-width: 768px), (min-width: 1024px)
                if (condition.find("max-width") != std::string::npos) {
                    mq.type = MediaQueryType::MaxWidth;
                    size_t colon = condition.find(':');
                    if (colon != std::string::npos) {
                        mq.value = std::strtof(condition.c_str() + colon + 1, nullptr);
                    }
                } else if (condition.find("min-width") != std::string::npos) {
                    mq.type = MediaQueryType::MinWidth;
                    size_t colon = condition.find(':');
                    if (colon != std::string::npos) {
                        mq.value = std::strtof(condition.c_str() + colon + 1, nullptr);
                    }
                }

                // Parse rules inside @media block
                while (true) {
                    SkipWhitespaceAndComments();
                    if (AtEnd() || Peek() == '}') break;
                    KSSRule rule;
                    if (ParseRule(rule)) {
                        rule.mediaQuery = mq;
                        stylesheet_.rules.push_back(std::move(rule));
                    } else {
                        while (!AtEnd() && Peek() != '}') Advance();
                        if (!AtEnd()) Advance();
                    }
                }
                if (!AtEnd()) Advance(); // closing '}'
                continue;
            } else if (keyword == "keyframes") {
                // @keyframes name { ... }
                SkipWhitespace();
                std::string name = ParseIdent();
                SkipWhitespace();
                if (!AtEnd() && Peek() == '{') {
                    Advance();
                    KSSKeyframes kf;
                    kf.name = name;
                    while (true) {
                        SkipWhitespaceAndComments();
                        if (AtEnd() || Peek() == '}') break;
                        // Parse percentage or "from"/"to"
                        std::string pctStr;
                        while (!AtEnd() && Peek() != '{' && Peek() != '}')
                            pctStr += Advance();
                        // Trim
                        while (!pctStr.empty() && pctStr.back() == ' ') pctStr.pop_back();
                        while (!pctStr.empty() && pctStr[0] == ' ') pctStr.erase(0, 1);
                        float pct = 0.0f;
                        if (pctStr == "from") pct = 0.0f;
                        else if (pctStr == "to") pct = 100.0f;
                        else pct = std::strtof(pctStr.c_str(), nullptr);

                        KSSKeyframe frame;
                        frame.percent = pct;
                        if (!AtEnd() && Peek() == '{') {
                            ParseDeclarationBlock(frame.declarations);
                        }
                        kf.frames.push_back(std::move(frame));
                    }
                    if (!AtEnd()) Advance(); // '}'
                    stylesheet_.keyframes.push_back(std::move(kf));
                }
                continue;
            }
            // Unknown @-rule - skip to next block
            while (!AtEnd() && Peek() != '{' && Peek() != ';') Advance();
            if (!AtEnd() && Peek() == '{') {
                Advance();
                int depth = 1;
                while (!AtEnd() && depth > 0) {
                    if (Peek() == '{') depth++;
                    if (Peek() == '}') depth--;
                    Advance();
                }
            } else if (!AtEnd()) Advance();
            continue;
        }

        KSSRule rule;
        if (ParseRule(rule)) {
            // Extract CSS variables from :root rules
            bool isRoot = false;
            for (const auto& sel : rule.selectors) {
                if (!sel.parts.empty() && sel.parts[0].pseudoClass == "root") {
                    isRoot = true;
                    break;
                }
            }
            if (isRoot) {
                for (const auto& decl : rule.declarations) {
                    if (decl.property.size() > 2 && decl.property[0] == '-' && decl.property[1] == '-') {
                        stylesheet_.variables.push_back(decl);
                    }
                }
            }
            stylesheet_.rules.push_back(std::move(rule));
        } else {
            // Skip to next rule on error
            while (!AtEnd() && Peek() != '}') Advance();
            if (!AtEnd()) Advance();
        }
    }

    return errors_.empty();
}

} // namespace markup
} // namespace ui
} // namespace koilo
