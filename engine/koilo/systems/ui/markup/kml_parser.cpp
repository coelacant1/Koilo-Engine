// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file kml_parser.cpp
 * @brief HTML-like markup parser implementation.
 * @date 03/08/2026
 * @author Coela Can't
 */

#include "kml_parser.hpp"
#include <algorithm>
#include <cctype>

namespace koilo {
namespace ui {
namespace markup {

// ============================================================================
// HTML entity decoding
// ============================================================================

// Decode HTML entities (&amp;, &#60;, etc.) in text content
static std::string DecodeEntities(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    size_t i = 0;
    while (i < s.size()) {
        if (s[i] == '&') {
            size_t semi = s.find(';', i + 1);
            if (semi != std::string::npos && semi - i <= 10) {
                std::string entity = s.substr(i + 1, semi - i - 1);
                if      (entity == "amp")  { out += '&';  i = semi + 1; continue; }
                else if (entity == "lt")   { out += '<';  i = semi + 1; continue; }
                else if (entity == "gt")   { out += '>';  i = semi + 1; continue; }
                else if (entity == "quot") { out += '"';  i = semi + 1; continue; }
                else if (entity == "apos") { out += '\''; i = semi + 1; continue; }
                else if (entity.size() > 1 && entity[0] == '#') {
                    // Numeric entity (&#60; or &#x3C;)
                    uint32_t cp = 0;
                    if (entity[1] == 'x' || entity[1] == 'X')
                        cp = static_cast<uint32_t>(std::strtoul(entity.c_str() + 2, nullptr, 16));
                    else
                        cp = static_cast<uint32_t>(std::strtoul(entity.c_str() + 1, nullptr, 10));
                    if (cp > 0 && cp < 0x80) {
                        out += static_cast<char>(cp);
                    } else if (cp < 0x800) {
                        out += static_cast<char>(0xC0 | (cp >> 6));
                        out += static_cast<char>(0x80 | (cp & 0x3F));
                    } else if (cp < 0x10000) {
                        out += static_cast<char>(0xE0 | (cp >> 12));
                        out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
                        out += static_cast<char>(0x80 | (cp & 0x3F));
                    }
                    i = semi + 1;
                    continue;
                }
            }
        }
        out += s[i++];
    }
    return out;
}

// ============================================================================
// Character-level helpers
// ============================================================================

// Return current character without advancing
char KMLParser::Peek() const {
    return AtEnd() ? '\0' : *pos_;
}

// Consume and return current character, tracking line/column
char KMLParser::Advance() {
    if (AtEnd()) return '\0';
    char c = *pos_++;
    if (c == '\n') { ++line_; col_ = 1; }
    else { ++col_; }
    return c;
}

// Check if scanner has reached end of input
bool KMLParser::AtEnd() const {
    return pos_ >= end_;
}

// Skip over whitespace characters
void KMLParser::SkipWhitespace() {
    while (!AtEnd() && std::isspace(static_cast<unsigned char>(Peek()))) {
        Advance();
    }
}

// Match and consume a literal string
bool KMLParser::Match(const char* str) {
    size_t len = std::strlen(str);
    if (static_cast<size_t>(end_ - pos_) < len) return false;
    if (std::strncmp(pos_, str, len) != 0) return false;
    for (size_t i = 0; i < len; ++i) Advance();
    return true;
}

// Record a parse error at current position
void KMLParser::Error(const std::string& msg) {
    errors_.push_back({line_, col_, msg});
}

// ============================================================================
// Parsing primitives
// ============================================================================

// Parse an element tag name (alphanumeric, dash, underscore)
std::string KMLParser::ParseTagName() {
    std::string name;
    while (!AtEnd()) {
        char c = Peek();
        if (std::isalnum(static_cast<unsigned char>(c)) || c == '-' || c == '_') {
            name += Advance();
        } else {
            break;
        }
    }
    // Normalize to lowercase
    for (auto& ch : name)
        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    return name;
}

// Parse a single- or double-quoted string with escape handling
std::string KMLParser::ParseQuotedString() {
    char quote = Peek();
    if (quote != '"' && quote != '\'') {
        Error("Expected quoted string");
        return "";
    }
    Advance(); // consume opening quote
    std::string result;
    while (!AtEnd() && Peek() != quote) {
        if (Peek() == '\\') {
            Advance();
            if (!AtEnd()) result += Advance();
        } else {
            result += Advance();
        }
    }
    if (!AtEnd()) Advance(); // consume closing quote
    return DecodeEntities(result);
}

// Parse raw text content until next '<', collapsing whitespace
std::string KMLParser::ParseText() {
    std::string text;
    while (!AtEnd() && Peek() != '<') {
        text += Advance();
    }
    // Trim and collapse whitespace
    std::string result;
    bool lastWasSpace = true;
    for (char c : text) {
        if (std::isspace(static_cast<unsigned char>(c))) {
            if (!lastWasSpace) result += ' ';
            lastWasSpace = true;
        } else {
            result += c;
            lastWasSpace = false;
        }
    }
    // Trim trailing space
    if (!result.empty() && result.back() == ' ') result.pop_back();
    // Trim leading space
    if (!result.empty() && result.front() == ' ') result = result.substr(1);
    return DecodeEntities(result);
}

// Parse a single element attribute (name=value or boolean)
bool KMLParser::ParseAttribute(KMLAttribute& attr) {
    SkipWhitespace();
    attr.name = ParseTagName();
    if (attr.name.empty()) return false;

    SkipWhitespace();
    if (Peek() == '=') {
        Advance(); // consume '='
        SkipWhitespace();
        if (Peek() == '"' || Peek() == '\'') {
            attr.value = ParseQuotedString();
        } else {
            // Unquoted value (read until whitespace or >)
            while (!AtEnd() && Peek() != ' ' && Peek() != '>' && Peek() != '/') {
                attr.value += Advance();
            }
        }
    } else {
        // Boolean attribute (e.g., "expanded", "checked")
        attr.value = "true";
    }
    return true;
}

// ============================================================================
// Comment handling
// ============================================================================

// Skip over an HTML comment (<!-- ... -->)
void KMLParser::SkipComment() {
    // pos_ is after "<!--"
    while (!AtEnd()) {
        if (Peek() == '-') {
            const char* saved = pos_;
            int savedLine = line_, savedCol = col_;
            if (Match("-->")) return;
            // Restore if partial match failed
            pos_ = saved; line_ = savedLine; col_ = savedCol;
            Advance();
        } else {
            Advance();
        }
    }
    Error("Unterminated comment");
}

// ============================================================================
// Element parsing
// ============================================================================

// Parse a single element with attributes, children, and closing tag
bool KMLParser::ParseElement(KMLElement& elem) {
    // Assume '<' already consumed
    elem.line = line_;
    elem.tag = ParseTagName();
    if (elem.tag.empty()) {
        Error("Expected tag name after '<'");
        return false;
    }

    // Parse attributes
    while (!AtEnd()) {
        SkipWhitespace();
        char c = Peek();
        if (c == '>') {
            Advance();
            break;
        }
        if (c == '/') {
            Advance();
            if (Peek() == '>') {
                Advance();
                return true; // Self-closing: <tag ... />
            }
            Error("Expected '>' after '/'");
            return false;
        }

        KMLAttribute attr;
        if (!ParseAttribute(attr)) {
            Error("Invalid attribute in <" + elem.tag + ">");
            return false;
        }

        // Extract special attributes
        if (attr.name == "id") {
            elem.id = attr.value;
        } else if (attr.name == "class") {
            // Split by spaces
            std::string cls;
            for (size_t i = 0; i <= attr.value.size(); ++i) {
                if (i == attr.value.size() || attr.value[i] == ' ') {
                    if (!cls.empty()) {
                        elem.classes.push_back(cls);
                        cls.clear();
                    }
                } else {
                    cls += attr.value[i];
                }
            }
        } else {
            elem.attributes.push_back(std::move(attr));
        }
    }

    // Parse children and text until closing tag
    ParseContent(elem.children);

    // Promote _text pseudo-elements to parent's textContent
    for (auto it = elem.children.begin(); it != elem.children.end(); ) {
        if (it->tag == "_text") {
            if (elem.textContent.empty())
                elem.textContent = it->textContent;
            else
                elem.textContent += it->textContent;
            it = elem.children.erase(it);
        } else {
            ++it;
        }
    }

    // Check for closing tag
    if (!AtEnd() && Peek() == '<') {
        const char* saved = pos_;
        int savedLine = line_, savedCol = col_;
        Advance(); // '<'
        if (Peek() == '/') {
            Advance(); // '/'
            std::string closeTag = ParseTagName();
            SkipWhitespace();
            if (Peek() == '>') {
                Advance();
                if (closeTag != elem.tag) {
                    Error("Mismatched closing tag: expected </" + elem.tag +
                          "> but found </" + closeTag + ">");
                }
                return true;
            }
        }
        // Not a valid closing tag - restore
        pos_ = saved; line_ = savedLine; col_ = savedCol;
    }

    return true;
}

// ============================================================================
// Content parsing (children + text)
// ============================================================================

// Parse child elements and text content within a parent
void KMLParser::ParseContent(std::vector<KMLElement>& siblings) {
    while (!AtEnd()) {
        SkipWhitespace();
        if (AtEnd()) break;

        if (Peek() == '<') {
            // Check what follows '<'
            const char* saved = pos_;
            int savedLine = line_, savedCol = col_;
            Advance(); // consume '<'

            // Comment: <!-- ... -->
            if (static_cast<size_t>(end_ - pos_) >= 3 &&
                pos_[0] == '!' && pos_[1] == '-' && pos_[2] == '-') {
                pos_ += 3; col_ += 3;
                SkipComment();
                continue;
            }

            // Closing tag: </tag> - return to parent
            if (Peek() == '/') {
                pos_ = saved; line_ = savedLine; col_ = savedCol;
                return;
            }

            // Child element
            KMLElement child;
            if (ParseElement(child)) {
                siblings.push_back(std::move(child));
            } else {
                break; // Error already recorded
            }
        } else {
            // Text content - attach to parent (caller handles placement)
            std::string text = ParseText();
            if (!text.empty() && !siblings.empty()) {
                siblings.back().textContent = text;
            } else if (!text.empty()) {
                // Text before any element - create a text pseudo-element
                KMLElement textElem;
                textElem.tag = "_text";
                textElem.textContent = text;
                textElem.line = line_;
                siblings.push_back(std::move(textElem));
            }
        }
    }
}

// ============================================================================
// Public API
// ============================================================================

// Parse a complete KML markup string into an element tree
bool KMLParser::Parse(const std::string& source) {
    roots_.clear();
    errors_.clear();
    src_ = source.c_str();
    end_ = src_ + source.size();
    pos_ = src_;
    line_ = 1;
    col_ = 1;

    ParseContent(roots_);

    return errors_.empty();
}

} // namespace markup
} // namespace ui
} // namespace koilo
