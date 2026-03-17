// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file kml_types.hpp
 * @brief Core data structures for KML (markup) and KSS (stylesheets).
 *
 * Defines the intermediate representation produced by the KML and KSS
 * parsers, consumed by the builder to emit UIContext API calls.
 *
 * @date 03/08/2026
 * @author Coela Can't
 */

#pragma once

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include "../../../registry/reflect_macros.hpp"

namespace koilo {
namespace ui {
namespace markup {

// ============================================================================
// Color parsing
// ============================================================================

/** @struct Color @brief RGBA color value. */
struct Color {
    uint8_t r = 0, g = 0, b = 0, a = 255; ///< color components

    KL_BEGIN_FIELDS(Color)
        KL_FIELD(Color, r, "R", 0, 255)
    KL_END_FIELDS

    KL_BEGIN_METHODS(Color)
        /* No reflected methods. */
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(Color)
        /* No reflected ctors. */
    KL_END_DESCRIBE(Color)

};

/// Parse "#RGB", "#RRGGBB", "#RRGGBBAA", "rgb(r,g,b)", "rgba(r,g,b,a)".
/// Returns false on parse failure.
bool ParseColor(const std::string& str, Color& out);

// ============================================================================
// CSS value types
// ============================================================================

enum class SizeUnit : uint8_t {
    Px,           ///< 100px  (or bare number)
    Percent,      ///< 50%
    Fill,         ///< fill   -> SizeMode::FillRemaining
    FitContent,   ///< fit-content -> SizeMode::FitContent
    Auto,         ///< auto   -> unset / default
    Em,           ///< 1.5em  (relative to element font size)
    Rem,          ///< 1.5rem (relative to root font size)
    Vw,           ///< 10vw   (1% of viewport width)
    Vh            ///< 10vh   (1% of viewport height)
};

/** @struct SizeValue @brief A numeric value with a CSS unit. */
struct SizeValue {
    float number = 0.0f;          ///< numeric magnitude
    SizeUnit unit = SizeUnit::Auto; ///< CSS unit type

    KL_BEGIN_FIELDS(SizeValue)
        KL_FIELD(SizeValue, number, "Number", __FLT_MIN__, __FLT_MAX__)
    KL_END_FIELDS

    KL_BEGIN_METHODS(SizeValue)
        /* No reflected methods. */
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(SizeValue)
        /* No reflected ctors. */
    KL_END_DESCRIBE(SizeValue)

};

/// Parse "100px", "50%", "fill", "fit-content", "auto", or bare number.
bool ParseSizeValue(const std::string& str, SizeValue& out);

/// Parse edge shorthand: "10px", "10px 20px", "10px 20px 30px 40px".
/// Returns {top, right, bottom, left}.
/** @struct EdgesValue @brief Four-edge shorthand (top, right, bottom, left). */

struct EdgesValue { float top=0, right=0, bottom=0, left=0;

    KL_BEGIN_FIELDS(EdgesValue)
        KL_FIELD(EdgesValue, top, "Top", __FLT_MIN__, __FLT_MAX__)
    KL_END_FIELDS

    KL_BEGIN_METHODS(EdgesValue)
        /* No reflected methods. */
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(EdgesValue)
        /* No reflected ctors. */
    KL_END_DESCRIBE(EdgesValue)

};
bool ParseEdges(const std::string& str, EdgesValue& out);

// ============================================================================
// KML - parsed markup elements
// ============================================================================

/** @struct KMLAttribute @brief A single name=value attribute on a KML element. */
struct KMLAttribute {
    std::string name;  ///< attribute name
    std::string value; ///< attribute value

    KL_BEGIN_FIELDS(KMLAttribute)
        KL_FIELD(KMLAttribute, name, "Name", 0, 0)
    KL_END_FIELDS

    KL_BEGIN_METHODS(KMLAttribute)
        /* No reflected methods. */
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(KMLAttribute)
        /* No reflected ctors. */
    KL_END_DESCRIBE(KMLAttribute)

};

/** @struct KMLElement @brief Parsed markup element with tag, attributes, and children. */
struct KMLElement {
    std::string tag;                          ///< "panel", "button", etc.
    std::string id;                           ///< from id="..."
    std::vector<std::string> classes;         ///< from class="a b c"
    std::vector<KMLAttribute> attributes;    ///< all other attributes
    std::string textContent;                  ///< text between open/close tags
    std::vector<KMLElement> children;         ///< child elements
    int line = 0;                             ///< source line for error messages

    KL_BEGIN_FIELDS(KMLElement)
        KL_FIELD(KMLElement, tag, "Tag", 0, 0)
    KL_END_FIELDS

    KL_BEGIN_METHODS(KMLElement)
        /* No reflected methods. */
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(KMLElement)
        /* No reflected ctors. */
    KL_END_DESCRIBE(KMLElement)

};

// ============================================================================
// KSS - parsed stylesheet rules
// ============================================================================

/// Combinator type between selector parts
enum class CombinatorType : uint8_t {
    Descendant = 0,   ///< space (any ancestor)
    Child,            ///< > (direct parent only)
    AdjacentSibling,  ///< + (immediately preceding sibling)
    GeneralSibling    ///< ~ (any preceding sibling)
};

/// Attribute selector match mode
enum class AttrMatchMode : uint8_t {
    Exists = 0,  ///< [attr]
    Equals,      ///< [attr=val]
    Prefix,      ///< [attr^=val]
    Suffix,      ///< [attr$=val]
    Contains     ///< [attr*=val]
};

/** @struct AttributeSelector @brief Attribute selector with name, value, and match mode. */
struct AttributeSelector {
    std::string name;                           ///< attribute name
    std::string value;                          ///< value to match against
    AttrMatchMode mode = AttrMatchMode::Exists; ///< matching strategy

    KL_BEGIN_FIELDS(AttributeSelector)
        KL_FIELD(AttributeSelector, name, "Name", 0, 0)
    KL_END_FIELDS

    KL_BEGIN_METHODS(AttributeSelector)
        /* No reflected methods. */
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(AttributeSelector)
        /* No reflected ctors. */
    KL_END_DESCRIBE(AttributeSelector)

};

/** @struct KSSSimpleSelector @brief A single selector component: element.class#id:pseudo. */
struct KSSSimpleSelector {
    std::string element;                    ///< "button", "*", "" (empty = universal)
    std::string id;                         ///< from #id
    std::vector<std::string> classNames;    ///< from .class (supports .a.b compound)
    std::string pseudoClass;                ///< "hover", "active", "focus", "disabled"
    CombinatorType combinator = CombinatorType::Descendant; ///< how this part relates to next
    std::string negation;                   ///< content of :not(...) - simple selector string
    std::vector<AttributeSelector> attrSelectors; ///< [attr], [attr=val], etc.

    KL_BEGIN_FIELDS(KSSSimpleSelector)
        KL_FIELD(KSSSimpleSelector, element, "Element", 0, 0)
    KL_END_FIELDS

    KL_BEGIN_METHODS(KSSSimpleSelector)
        /* No reflected methods. */
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(KSSSimpleSelector)
        /* No reflected ctors. */
    KL_END_DESCRIBE(KSSSimpleSelector)

};

/** @struct KSSSelector @brief Compound selector stored right-to-left: [0] is the target element. */
struct KSSSelector {
    std::vector<KSSSimpleSelector> parts;  ///< [0]=target, [1]=parent, etc.

    /// CSS specificity as a single comparable integer.
    /// Format: (id_count * 100) + (class_count * 10) + element_count
    int Specificity() const;

    KL_BEGIN_FIELDS(KSSSelector)
        KL_FIELD(KSSSelector, parts, "Parts", 0, 0)
    KL_END_FIELDS

    KL_BEGIN_METHODS(KSSSelector)
        /* No reflected methods. */
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(KSSSelector)
        /* No reflected ctors. */
    KL_END_DESCRIBE(KSSSelector)

};

/** @struct KSSDeclaration @brief A single CSS property-value declaration. */
struct KSSDeclaration {
    std::string property; ///< CSS property name
    std::string value;    ///< CSS property value

    KL_BEGIN_FIELDS(KSSDeclaration)
        KL_FIELD(KSSDeclaration, property, "Property", 0, 0)
    KL_END_FIELDS

    KL_BEGIN_METHODS(KSSDeclaration)
        /* No reflected methods. */
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(KSSDeclaration)
        /* No reflected ctors. */
    KL_END_DESCRIBE(KSSDeclaration)

};

enum class MediaQueryType : uint8_t {
    None = 0,
    MinWidth,
    MaxWidth,
    MinHeight,
    MaxHeight
};

/** @struct MediaQuery @brief Parsed @media condition. */
struct MediaQuery {
    MediaQueryType type = MediaQueryType::None; ///< condition type
    float value = 0.0f;                        ///< threshold in pixels

    KL_BEGIN_FIELDS(MediaQuery)
        KL_FIELD(MediaQuery, type, "Type", 0, 0)
    KL_END_FIELDS

    KL_BEGIN_METHODS(MediaQuery)
        /* No reflected methods. */
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(MediaQuery)
        /* No reflected ctors. */
    KL_END_DESCRIBE(MediaQuery)

};

/** @struct KSSRule @brief A stylesheet rule: selectors + declaration block. */
struct KSSRule {
    std::vector<KSSSelector> selectors;      ///< comma-separated selectors
    std::vector<KSSDeclaration> declarations; ///< property declarations
    MediaQuery mediaQuery;                    ///< optional @media condition

    KL_BEGIN_FIELDS(KSSRule)
        KL_FIELD(KSSRule, selectors, "Selectors", 0, 0)
    KL_END_FIELDS

    KL_BEGIN_METHODS(KSSRule)
        /* No reflected methods. */
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(KSSRule)
        /* No reflected ctors. */
    KL_END_DESCRIBE(KSSRule)

};

/** @struct KSSKeyframe @brief A single keyframe stop in an animation. */
struct KSSKeyframe {
    float percent = 0.0f;                     ///< progress percentage (0-100)
    std::vector<KSSDeclaration> declarations; ///< style declarations at this stop

    KL_BEGIN_FIELDS(KSSKeyframe)
        KL_FIELD(KSSKeyframe, percent, "Percent", __FLT_MIN__, __FLT_MAX__)
    KL_END_FIELDS

    KL_BEGIN_METHODS(KSSKeyframe)
        /* No reflected methods. */
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(KSSKeyframe)
        /* No reflected ctors. */
    KL_END_DESCRIBE(KSSKeyframe)

};

/** @struct KSSKeyframes @brief Named @keyframes animation definition. */
struct KSSKeyframes {
    std::string name;                ///< animation name
    std::vector<KSSKeyframe> frames; ///< ordered keyframe stops

    KL_BEGIN_FIELDS(KSSKeyframes)
        KL_FIELD(KSSKeyframes, name, "Name", 0, 0)
    KL_END_FIELDS

    KL_BEGIN_METHODS(KSSKeyframes)
        /* No reflected methods. */
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(KSSKeyframes)
        /* No reflected ctors. */
    KL_END_DESCRIBE(KSSKeyframes)

};

/** @struct KSSStylesheet @brief A parsed stylesheet with rules, variables, and keyframes. */
struct KSSStylesheet {
    std::vector<KSSRule> rules;             ///< stylesheet rules
    std::vector<KSSDeclaration> variables;  ///< --var-name: value declarations
    std::vector<KSSKeyframes> keyframes;   ///< @keyframes definitions

    KL_BEGIN_FIELDS(KSSStylesheet)
        KL_FIELD(KSSStylesheet, rules, "Rules", 0, 0)
    KL_END_FIELDS

    KL_BEGIN_METHODS(KSSStylesheet)
        /* No reflected methods. */
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(KSSStylesheet)
        /* No reflected ctors. */
    KL_END_DESCRIBE(KSSStylesheet)

};

// ============================================================================
// Parse errors
// ============================================================================

/** @struct ParseError @brief Diagnostic error with source location. */
struct ParseError {
    int line = 0;        ///< source line number
    int column = 0;      ///< source column number
    std::string message; ///< human-readable error description

    KL_BEGIN_FIELDS(ParseError)
        KL_FIELD(ParseError, line, "Line", -2147483648, 2147483647)
    KL_END_FIELDS

    KL_BEGIN_METHODS(ParseError)
        /* No reflected methods. */
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(ParseError)
        /* No reflected ctors. */
    KL_END_DESCRIBE(ParseError)

};

} // namespace markup
} // namespace ui
} // namespace koilo
