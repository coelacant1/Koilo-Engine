// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file kml_builder.hpp
 * @brief Converts a parsed KML element tree + KSS stylesheet into
 *        UIContext widget tree via API calls.
 *
 * Handles tag-name-to-Create* dispatch, CSS property-to-Set* mapping,
 * selector matching with specificity-based cascade, and pseudo-state
 * style injection into the Theme.
 *
 * @date 03/09/2026
 * @author Coela
 */

#pragma once

#include "kml_types.hpp"
#include "../ui_context.hpp"
#include "../style.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>

namespace koilo {
namespace ui {
namespace markup {

/// Build result returned from KMLBuilder::Build().
struct BuildResult {
    int rootWidgetIdx = -1;
    std::vector<ParseError> errors;
    int widgetsCreated = 0;
};

class KMLBuilder {
public:
    explicit KMLBuilder(UIContext& ctx, Theme& theme);

    /// Build widget tree from parsed elements, applying stylesheet rules.
    BuildResult Build(const std::vector<KMLElement>& elements,
                      const KSSStylesheet& stylesheet);

    /// Build widget tree from parsed elements without a stylesheet.
    BuildResult Build(const std::vector<KMLElement>& elements);

private:
    UIContext& ctx_;
    Theme& theme_;

    // Selector matching context: stores class list and ancestry per widget
    struct WidgetMeta {
        int widgetIdx;
        std::string tag;
        std::string id;
        std::vector<std::string> classes;
        std::vector<std::pair<std::string, std::string>> attributes; // raw attributes
        int parentMetaIdx; // -1 for root
        int childIndex = 0;     // index among parent's children
        int siblingCount = 0;   // total siblings (including self)
    };
    std::vector<WidgetMeta> metas_;

    // CSS variables resolved from :root
    std::unordered_map<std::string, std::string> cssVariables_;

    int widgetsCreated_ = 0;
    std::vector<ParseError> errors_;

    // Core build
    int BuildElement(const KMLElement& elem, int parentIdx, int parentMetaIdx);
    int CreateWidget(const std::string& tag, const std::string& id);

    // Attribute application
    void ApplyAttributes(int widgetIdx, const KMLElement& elem);
    void ApplyAttribute(int widgetIdx, const std::string& tag,
                        const std::string& name, const std::string& value);

    // Stylesheet application
    void ApplyStylesheet(const KSSStylesheet& stylesheet);
    bool SelectorMatches(const KSSSelector& selector, int metaIdx) const;
    bool SimpleSelectorMatches(const KSSSimpleSelector& simple,
                               const WidgetMeta& meta) const;
    bool SimpleSelectorMatchesBasic(const KSSSimpleSelector& simple,
                                    const WidgetMeta& meta) const;
    void ApplyDeclarations(int widgetIdx, const std::string& tag,
                           const std::string& pseudoClass,
                           const std::vector<KSSDeclaration>& decls);

    // CSS property application
    void ApplyLayoutProperty(int widgetIdx, const std::string& prop,
                             const std::string& value);
    void ApplyVisualProperty(int widgetIdx, const std::string& tag,
                             const std::string& pseudoClass,
                             const std::string& prop, const std::string& value);

    // CSS property inheritance (top-down tree walk)
    void InheritProperties(int widgetIdx);

    // Helpers
    PseudoState PseudoStateFromString(const std::string& s) const;
    WidgetTag TagFromString(const std::string& s) const;
    float ResolveSizeUnit(const SizeValue& sv, float elementFontSize) const;

    void Error(int line, const std::string& msg);
    std::string ResolveVariable(const std::string& value) const;

    // Viewport dimensions for vw/vh resolution
    float viewportW_ = 1280.0f;
    float viewportH_ = 720.0f;
    float rootFontSize_ = 14.0f;
};

} // namespace markup
} // namespace ui
} // namespace koilo
