// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file kml_types.hpp
 * @brief Core data structures for KML (markup) and KSS (stylesheets).
 *
 * Defines the intermediate representation produced by the KML and KSS
 * parsers, consumed by the builder to emit UIContext API calls.
 *
 * @date 03/09/2026
 * @author Coela
 */

#pragma once

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace koilo {
namespace ui {
namespace markup {

// ---------------------------------------------------------------------------
// Color parsing
// ---------------------------------------------------------------------------

struct Color {
    uint8_t r = 0, g = 0, b = 0, a = 255;
};

/// Parse "#RGB", "#RRGGBB", "#RRGGBBAA", "rgb(r,g,b)", "rgba(r,g,b,a)".
/// Returns false on parse failure.
bool ParseColor(const std::string& str, Color& out);

// ---------------------------------------------------------------------------
// CSS value types
// ---------------------------------------------------------------------------

enum class SizeUnit : uint8_t {
    Px,           // 100px  (or bare number)
    Percent,      // 50%
    Fill,         // fill   -> SizeMode::FillRemaining
    FitContent,   // fit-content -> SizeMode::FitContent
    Auto,         // auto   -> unset / default
    Em,           // 1.5em  (relative to element font size)
    Rem,          // 1.5rem (relative to root font size)
    Vw,           // 10vw   (1% of viewport width)
    Vh            // 10vh   (1% of viewport height)
};

struct SizeValue {
    float number = 0.0f;
    SizeUnit unit = SizeUnit::Auto;
};

/// Parse "100px", "50%", "fill", "fit-content", "auto", or bare number.
bool ParseSizeValue(const std::string& str, SizeValue& out);

/// Parse edge shorthand: "10px", "10px 20px", "10px 20px 30px 40px".
/// Returns {top, right, bottom, left}.
struct EdgesValue { float top=0, right=0, bottom=0, left=0; };
bool ParseEdges(const std::string& str, EdgesValue& out);

// ---------------------------------------------------------------------------
// KML - parsed markup elements
// ---------------------------------------------------------------------------

struct KMLAttribute {
    std::string name;
    std::string value;
};

struct KMLElement {
    std::string tag;                          // "panel", "button", etc.
    std::string id;                           // from id="..."
    std::vector<std::string> classes;         // from class="a b c"
    std::vector<KMLAttribute> attributes;    // all other attributes
    std::string textContent;                  // text between open/close tags
    std::vector<KMLElement> children;
    int line = 0;                             // source line for error messages
};

// ---------------------------------------------------------------------------
// KSS - parsed stylesheet rules
// ---------------------------------------------------------------------------

/// Combinator type between selector parts
enum class CombinatorType : uint8_t {
    Descendant = 0,   // space (any ancestor)
    Child,            // > (direct parent only)
    AdjacentSibling,  // + (immediately preceding sibling)
    GeneralSibling    // ~ (any preceding sibling)
};

/// Attribute selector match mode
enum class AttrMatchMode : uint8_t {
    Exists = 0,  // [attr]
    Equals,      // [attr=val]
    Prefix,      // [attr^=val]
    Suffix,      // [attr$=val]
    Contains     // [attr*=val]
};

struct AttributeSelector {
    std::string name;
    std::string value;
    AttrMatchMode mode = AttrMatchMode::Exists;
};

/// A single selector component: element.class#id:pseudo
struct KSSSimpleSelector {
    std::string element;                    // "button", "*", "" (empty = universal)
    std::string id;                         // from #id
    std::vector<std::string> classNames;    // from .class (supports .a.b compound)
    std::string pseudoClass;                // "hover", "active", "focus", "disabled"
    CombinatorType combinator = CombinatorType::Descendant; // how this part relates to next
    std::string negation;                   // content of :not(...) - simple selector string
    std::vector<AttributeSelector> attrSelectors; // [attr], [attr=val], etc.
};

/// A compound selector is one or more simple selectors separated by spaces
/// (descendant combinator). Stored right-to-left: [0] is the target element.
struct KSSSelector {
    std::vector<KSSSimpleSelector> parts;  // [0]=target, [1]=parent, etc.

    /// CSS specificity as a single comparable integer.
    /// Format: (id_count * 100) + (class_count * 10) + element_count
    int Specificity() const;
};

struct KSSDeclaration {
    std::string property;
    std::string value;
};

enum class MediaQueryType : uint8_t {
    None = 0,
    MinWidth,
    MaxWidth,
    MinHeight,
    MaxHeight
};

struct MediaQuery {
    MediaQueryType type = MediaQueryType::None;
    float value = 0.0f;
};

struct KSSRule {
    std::vector<KSSSelector> selectors;  // comma-separated selectors
    std::vector<KSSDeclaration> declarations;
    MediaQuery mediaQuery;                // optional @media condition
};

struct KSSKeyframe {
    float percent = 0.0f;
    std::vector<KSSDeclaration> declarations;
};

struct KSSKeyframes {
    std::string name;
    std::vector<KSSKeyframe> frames;
};

/// A parsed stylesheet is a list of rules plus CSS variables and keyframes.
struct KSSStylesheet {
    std::vector<KSSRule> rules;
    std::vector<KSSDeclaration> variables; // --var-name: value declarations
    std::vector<KSSKeyframes> keyframes;   // @keyframes definitions
};

// ---------------------------------------------------------------------------
// Parse errors
// ---------------------------------------------------------------------------

struct ParseError {
    int line = 0;
    int column = 0;
    std::string message;
};

} // namespace markup
} // namespace ui
} // namespace koilo
