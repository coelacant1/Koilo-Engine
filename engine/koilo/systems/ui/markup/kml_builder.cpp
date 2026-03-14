// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file kml_builder.cpp
 * @brief Converts parsed KML + KSS into UIContext widget trees.
 * @date 03/08/2026
 * @author Coela Can't
 */

#include "kml_builder.hpp"
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <sstream>
#include <vector>

namespace koilo {
namespace ui {
namespace markup {

// ============================================================================
// Tag / pseudo-state lookup tables
// ============================================================================

static const struct { const char* name; WidgetTag tag; } kTagMap[] = {
    {"panel",          WidgetTag::Panel},
    {"label",          WidgetTag::Label},
    {"button",         WidgetTag::Button},
    {"textfield",      WidgetTag::TextField},
    {"input",          WidgetTag::TextField},   // HTML-familiar alias
    {"slider",         WidgetTag::Slider},
    {"checkbox",       WidgetTag::Checkbox},
    {"dropdown",       WidgetTag::Dropdown},
    {"select",         WidgetTag::Dropdown},    // HTML-familiar alias
    {"scrollview",     WidgetTag::ScrollView},
    {"separator",      WidgetTag::Separator},
    {"hr",             WidgetTag::Separator},   // HTML-familiar alias
    {"image",          WidgetTag::Image},
    {"img",            WidgetTag::Image},       // HTML-familiar alias
    {"treenode",       WidgetTag::TreeNode},
    {"colorfield",     WidgetTag::ColorField},
    {"viewport",       WidgetTag::Viewport},
    {"dock",           WidgetTag::DockContainer},
    {"dockcontainer",  WidgetTag::DockContainer},
    {"splitpane",      WidgetTag::SplitPane},
    {"split",          WidgetTag::SplitPane},   // shorthand
    {"tabbar",         WidgetTag::TabBar},
    {"tabs",           WidgetTag::TabBar},      // shorthand
    {"popupmenu",      WidgetTag::PopupMenu},
    {"popup",          WidgetTag::PopupMenu},   // shorthand
    {"menuitem",       WidgetTag::MenuItem},
    {"floatingpanel",  WidgetTag::FloatingPanel},
    {"float",          WidgetTag::FloatingPanel},  // shorthand
    {"virtuallist",    WidgetTag::VirtualList},
    {"vlist",          WidgetTag::VirtualList},     // shorthand
    {"canvas",         WidgetTag::Canvas2D},
    {"canvas2d",       WidgetTag::Canvas2D},        // alias
};

static const struct { const char* name; PseudoState state; } kPseudoMap[] = {
    {"hover",         PseudoState::Hovered},
    {"hovered",       PseudoState::Hovered},
    {"active",        PseudoState::Pressed},
    {"pressed",       PseudoState::Pressed},
    {"focus",         PseudoState::Focused},
    {"focused",       PseudoState::Focused},
    {"focus-visible", PseudoState::FocusVisible},
    {"selected",      PseudoState::Selected},
    {"disabled",      PseudoState::Disabled},
};

// Look up WidgetTag enum value from a tag name string
WidgetTag KMLBuilder::TagFromString(const std::string& s) const {
    for (const auto& m : kTagMap) {
        if (s == m.name) return m.tag;
    }
    return WidgetTag::Panel; // default fallback
}

// Look up PseudoState enum value from a pseudo-class string
PseudoState KMLBuilder::PseudoStateFromString(const std::string& s) const {
    for (const auto& m : kPseudoMap) {
        if (s == m.name) return m.state;
    }
    return PseudoState::Normal;
}

// ============================================================================
// Constructor
// ============================================================================

// Construct builder with UI context and theme references
KMLBuilder::KMLBuilder(UIContext& ctx, Theme& theme)
    : ctx_(ctx), theme_(theme) {}

// ============================================================================
// Widget creation from tag name
// ============================================================================

// Create a widget from a tag name, dispatching to the correct UIContext factory
int KMLBuilder::CreateWidget(const std::string& tag, const std::string& id) {
    const char* cid = id.empty() ? tag.c_str() : id.c_str();

    // Tag-specific creation
    if (tag == "label")         return ctx_.CreateLabel(cid, "");
    if (tag == "button")        return ctx_.CreateButton(cid, "");
    if (tag == "textfield" || tag == "input")
                                return ctx_.CreateTextField(cid, "");
    if (tag == "slider")        return ctx_.CreateSlider(cid, 0.0f, 100.0f, 0.0f);
    if (tag == "checkbox")      return ctx_.CreateCheckbox(cid, false);
    if (tag == "separator" || tag == "hr")
                                return ctx_.CreateSeparator(cid);
    if (tag == "dock" || tag == "dockcontainer")
                                return ctx_.CreateDockContainer(cid);
    if (tag == "splitpane" || tag == "split")
                                return ctx_.CreateSplitPane(cid);
    if (tag == "tabbar" || tag == "tabs")
                                return ctx_.CreateTabBar(cid);
    if (tag == "popupmenu" || tag == "popup")
                                return ctx_.CreatePopupMenu(cid);
    if (tag == "menuitem")      return ctx_.CreateMenuItem(cid, "");
    if (tag == "scrollview") {
        int idx = ctx_.CreateWidget(WidgetTag::ScrollView, cid);
        if (idx >= 0) {
            Widget* w = ctx_.GetWidget(idx);
            if (w) {
                w->flags.clipChildren = 1;
                w->layout.direction = LayoutDirection::Column;
            }
        }
        return idx;
    }
    if (tag == "treenode") {
        int idx = ctx_.CreateWidget(WidgetTag::TreeNode, cid);
        if (idx >= 0) {
            Widget* w = ctx_.GetWidget(idx);
            if (w) {
                w->widthMode = SizeMode::FillRemaining;
                w->localH = 22.0f;
                w->layout.direction = LayoutDirection::Column;
            }
        }
        return idx;
    }
    if (tag == "image" || tag == "img")
                                return ctx_.CreateWidget(WidgetTag::Image, cid);
    if (tag == "colorfield")    return ctx_.CreateWidget(WidgetTag::ColorField, cid);
    if (tag == "viewport")      return ctx_.CreateWidget(WidgetTag::Viewport, cid);
    if (tag == "dropdown" || tag == "select")
                                return ctx_.CreateWidget(WidgetTag::Dropdown, cid);
    if (tag == "progressbar" || tag == "progress")
                                return ctx_.CreateProgressBar(cid, 0.0f);
    if (tag == "toggleswitch" || tag == "toggle")
                                return ctx_.CreateToggleSwitch(cid, false);
    if (tag == "radiobutton" || tag == "radio")
                                return ctx_.CreateRadioButton(cid, "", false);
    if (tag == "numberspinner" || tag == "spinner")
                                return ctx_.CreateNumberSpinner(cid, 0.0f, 0.0f, 100.0f, 1.0f);

    if (tag == "floatingpanel" || tag == "float") {
        float offX = 100.0f + floatingPanelCount_ * 40.0f;
        float offY = 100.0f + floatingPanelCount_ * 30.0f;
        int idx = ctx_.CreateFloatingPanel(cid, cid, offX, offY, 300.0f, 200.0f);
        if (idx >= 0) {
            Widget* w = ctx_.Pool().Get(idx);
            if (w) w->zOrder = static_cast<int16_t>(500 + floatingPanelCount_);
        }
        ++floatingPanelCount_;
        return idx;
    }

    // Default: panel
    return ctx_.CreatePanel(cid);
}

// ============================================================================
// Attribute application (inline HTML attributes -> API calls)
// ============================================================================

void KMLBuilder::ApplyAttribute(int widgetIdx, const std::string& tag,
                                  const std::string& name, const std::string& value) {
    Widget* w = ctx_.GetWidget(widgetIdx);
    if (!w) return;

    // Text content
    if (name == "text") {
        ctx_.SetText(widgetIdx, value.c_str());
        return;
    }

    // Visibility / state
    if (name == "visible") {
        ctx_.SetVisible(widgetIdx, value != "false" && value != "0");
        return;
    }
    if (name == "enabled") {
        ctx_.SetEnabled(widgetIdx, value != "false" && value != "0");
        return;
    }

    // Slider attributes
    if (name == "min") { w->sliderMin = std::strtof(value.c_str(), nullptr); return; }
    if (name == "max") { w->sliderMax = std::strtof(value.c_str(), nullptr); return; }
    if (name == "value") {
        // ColorField: parse as color hex
        if (w->tag == WidgetTag::ColorField) {
            Color c;
            if (ParseColor(value, c)) {
                w->colorValue = {c.r, c.g, c.b, c.a};
            }
            return;
        }
        w->sliderValue = std::strtof(value.c_str(), nullptr);
        if (w->tag == WidgetTag::ProgressBar)
            w->progressValue = w->sliderValue;
        return;
    }

    // ProgressBar progress attribute
    if (name == "progress") {
        w->progressValue = std::strtof(value.c_str(), nullptr);
        return;
    }

    // NumberSpinner step
    if (name == "step") {
        w->spinnerStep = std::strtof(value.c_str(), nullptr);
        return;
    }

    // RadioButton group
    if (name == "group") {
        w->radioGroup = ctx_.Strings().Intern(value.c_str());
        return;
    }

    // Checkbox / ToggleSwitch / RadioButton
    if (name == "checked") {
        w->checked = (value != "false" && value != "0");
        return;
    }

    // TreeNode
    if (name == "expanded") {
        w->expanded = (value != "false" && value != "0");
        return;
    }
    if (name == "depth" || name == "tree-depth") {
        w->treeDepth = std::atoi(value.c_str());
        return;
    }
    if (name == "has-children") {
        w->treeHasChildren = (value != "false" && value != "0");
        return;
    }

    // FloatingPanel title
    if (name == "title") {
        w->panelTitleId = ctx_.Strings().Intern(value.c_str());
        return;
    }

    // MenuItem shortcut display text
    if (name == "shortcut") {
        w->shortcutTextId = ctx_.Strings().Intern(value.c_str());
        return;
    }

    // MenuItem submenu reference (resolved by id during tree build)
    if (name == "submenu") {
        // Store the target popup id for deferred resolution
        w->submenuIdx = -2; // sentinel: needs resolution
        w->panelTitleId = ctx_.Strings().Intern(value.c_str()); // reuse field temporarily
        return;
    }

    // VirtualList configuration
    if (name == "item-count") {
        w->virtualItemCount = std::atoi(value.c_str());
        w->contentHeight = w->virtualItemCount * w->virtualItemHeight;
        return;
    }
    if (name == "item-height") {
        w->virtualItemHeight = std::strtof(value.c_str(), nullptr);
        w->contentHeight = w->virtualItemCount * w->virtualItemHeight;
        return;
    }

    // Placeholder
    if (name == "placeholder") {
        w->placeholderId = ctx_.Strings().Intern(value.c_str());
        return;
    }

    // Icon
    if (name == "icon") {
        w->iconId = IconFromName(value.c_str());
        return;
    }

    // Event handlers
    if (name == "onclick") {
        ctx_.SetOnClickScript(widgetIdx, value.c_str());
        return;
    }
    if (name == "onchange") {
        ctx_.SetOnChangeScript(widgetIdx, value.c_str());
        return;
    }

    // Accessibility
    if (name == "aria-label") {
        w->ariaLabel = ctx_.Strings().Intern(value.c_str());
        return;
    }

    // Tooltip
    if (name == "title") {
        w->tooltipId = ctx_.Strings().Intern(value.c_str());
        return;
    }

    // SplitPane orientation
    if (name == "vertical") {
        bool vert = (value != "false" && value != "0");
        w->layout.direction = vert ? LayoutDirection::Column : LayoutDirection::Row;
        return;
    }
    if (name == "horizontal") {
        bool horiz = (value != "false" && value != "0");
        w->layout.direction = horiz ? LayoutDirection::Row : LayoutDirection::Column;
        return;
    }

    // z-index
    if (name == "z-index" || name == "zindex") {
        w->zOrder = static_cast<int16_t>(std::strtol(value.c_str(), nullptr, 10));
        return;
    }

    // Inline style - delegate to CSS property handler
    if (name == "style") {
        // Mini inline style parser: "property: value; property: value"
        size_t pos = 0;
        while (pos < value.size()) {
            size_t colon = value.find(':', pos);
            if (colon == std::string::npos) break;
            size_t semi = value.find(';', colon);
            if (semi == std::string::npos) semi = value.size();

            std::string prop = value.substr(pos, colon - pos);
            std::string val = value.substr(colon + 1, semi - colon - 1);
            // Trim
            auto trim = [](std::string s) {
                size_t a = s.find_first_not_of(" \t");
                size_t b = s.find_last_not_of(" \t");
                return (a == std::string::npos) ? "" : s.substr(a, b - a + 1);
            };
            prop = trim(prop);
            val = trim(val);
            for (auto& c : prop) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

            ApplyLayoutProperty(widgetIdx, prop, val);
            ApplyVisualProperty(widgetIdx, tag, "", prop, val);

            pos = semi + 1;
        }
        return;
    }
}

// Apply all attributes and text content from an element to a widget
void KMLBuilder::ApplyAttributes(int widgetIdx, const KMLElement& elem) {
    for (const auto& attr : elem.attributes) {
        ApplyAttribute(widgetIdx, elem.tag, attr.name, attr.value);
    }

    // Text content from element body
    if (!elem.textContent.empty()) {
        ctx_.SetText(widgetIdx, elem.textContent.c_str());
    }
}

// ============================================================================
// CSS property application - layout
// ============================================================================

void KMLBuilder::ApplyLayoutProperty(int widgetIdx, const std::string& prop,
                                       const std::string& value) {
    Widget* w = ctx_.GetWidget(widgetIdx);
    if (!w) return;

    if (prop == "display") {
        if (value == "row")    w->layout.direction = LayoutDirection::Row;
        if (value == "column") w->layout.direction = LayoutDirection::Column;
        if (value == "none") {
            ctx_.SetVisible(widgetIdx, false);
        } else if (value == "flex") {
            // flex is the default - direction already set or defaults to column
        } else if (value == "grid") {
            w->layout.isGrid = true;
        }
        ctx_.SetSizeMode(widgetIdx, w->widthMode, w->heightMode); // trigger dirty
        return;
    }

    if (prop == "width") {
        if (value == "min-content") { w->widthMode = SizeMode::MinContent; ctx_.SetSizeMode(widgetIdx, w->widthMode, w->heightMode); return; }
        if (value == "max-content") { w->widthMode = SizeMode::MaxContent; ctx_.SetSizeMode(widgetIdx, w->widthMode, w->heightMode); return; }
        SizeValue sv;
        if (ParseSizeValue(value, sv)) {
            float resolved = ResolveSizeUnit(sv, w->fontSize);
            switch (sv.unit) {
                case SizeUnit::Px:
                case SizeUnit::Em:
                case SizeUnit::Rem:
                case SizeUnit::Vw:
                case SizeUnit::Vh:
                    w->localW = resolved; w->widthMode = SizeMode::Fixed; break;
                case SizeUnit::Percent:     w->localW = sv.number; w->widthMode = SizeMode::Percent; break;
                case SizeUnit::Fill:        w->widthMode = SizeMode::FillRemaining; break;
                case SizeUnit::FitContent:  w->widthMode = SizeMode::FitContent; break;
                default: break;
            }
            ctx_.SetSizeMode(widgetIdx, w->widthMode, w->heightMode);
        }
        return;
    }

    if (prop == "height") {
        if (value == "min-content") { w->heightMode = SizeMode::MinContent; ctx_.SetSizeMode(widgetIdx, w->widthMode, w->heightMode); return; }
        if (value == "max-content") { w->heightMode = SizeMode::MaxContent; ctx_.SetSizeMode(widgetIdx, w->widthMode, w->heightMode); return; }
        SizeValue sv;
        if (ParseSizeValue(value, sv)) {
            float resolved = ResolveSizeUnit(sv, w->fontSize);
            switch (sv.unit) {
                case SizeUnit::Px:
                case SizeUnit::Em:
                case SizeUnit::Rem:
                case SizeUnit::Vw:
                case SizeUnit::Vh:
                    w->localH = resolved; w->heightMode = SizeMode::Fixed; break;
                case SizeUnit::Percent:     w->localH = sv.number; w->heightMode = SizeMode::Percent; break;
                case SizeUnit::Fill:        w->heightMode = SizeMode::FillRemaining; break;
                case SizeUnit::FitContent:  w->heightMode = SizeMode::FitContent; break;
                default: break;
            }
            ctx_.SetSizeMode(widgetIdx, w->widthMode, w->heightMode);
        }
        return;
    }

    if (prop == "padding") {
        EdgesValue ev;
        if (ParseEdges(value, ev))
            ctx_.SetPadding(widgetIdx, ev.top, ev.right, ev.bottom, ev.left);
        return;
    }
    if (prop == "padding-top")    { w->padding.top    = std::strtof(value.c_str(), nullptr); return; }
    if (prop == "padding-right")  { w->padding.right  = std::strtof(value.c_str(), nullptr); return; }
    if (prop == "padding-bottom") { w->padding.bottom = std::strtof(value.c_str(), nullptr); return; }
    if (prop == "padding-left")   { w->padding.left   = std::strtof(value.c_str(), nullptr); return; }

    if (prop == "margin") {
        if (value == "auto") {
            w->marginAuto = {1, 1, 1, 1};
        } else if (value.find("auto") != std::string::npos) {
            // Handle "0 auto" or "0px auto" patterns (horizontal centering)
            // Parse carefully: split by spaces
            std::istringstream iss(value);
            std::string tok;
            std::vector<std::string> tokens;
            while (iss >> tok) tokens.push_back(tok);
            if (tokens.size() == 2) {
                // vertical horizontal
                w->marginAuto.top = (tokens[0] == "auto") ? 1 : 0;
                w->marginAuto.bottom = w->marginAuto.top;
                w->marginAuto.left = (tokens[1] == "auto") ? 1 : 0;
                w->marginAuto.right = w->marginAuto.left;
                if (!w->marginAuto.top)    w->margin.top = w->margin.bottom = std::strtof(tokens[0].c_str(), nullptr);
                if (!w->marginAuto.left)   w->margin.left = w->margin.right = std::strtof(tokens[1].c_str(), nullptr);
            }
        } else {
            EdgesValue ev;
            if (ParseEdges(value, ev))
                ctx_.SetMargin(widgetIdx, ev.top, ev.right, ev.bottom, ev.left);
        }
        return;
    }
    if (prop == "margin-top")    { w->margin.top    = std::strtof(value.c_str(), nullptr); return; }
    if (prop == "margin-right")  { w->margin.right  = std::strtof(value.c_str(), nullptr); return; }
    if (prop == "margin-bottom") { w->margin.bottom = std::strtof(value.c_str(), nullptr); return; }
    if (prop == "margin-left")   { w->margin.left   = std::strtof(value.c_str(), nullptr); return; }

    if (prop == "gap") {
        w->layout.spacing = std::strtof(value.c_str(), nullptr);
        return;
    }

    if (prop == "align-items") {
        if (value == "start")        w->layout.crossAlign = Alignment::Start;
        else if (value == "center")  w->layout.crossAlign = Alignment::Center;
        else if (value == "end")     w->layout.crossAlign = Alignment::End;
        else if (value == "stretch") w->layout.crossAlign = Alignment::Stretch;
        return;
    }

    if (prop == "justify-content") {
        if (value == "start")              w->layout.mainAlign = Alignment::Start;
        else if (value == "center")        w->layout.mainAlign = Alignment::Center;
        else if (value == "end")           w->layout.mainAlign = Alignment::End;
        else if (value == "stretch")       w->layout.mainAlign = Alignment::Stretch;
        else if (value == "space-between") w->layout.mainAlign = Alignment::SpaceBetween;
        else if (value == "space-around")  w->layout.mainAlign = Alignment::SpaceAround;
        else if (value == "space-evenly")  w->layout.mainAlign = Alignment::SpaceEvenly;
        return;
    }

    if (prop == "flex-wrap") {
        w->layout.wrap = (value == "wrap");
        return;
    }

    if (prop == "flex-grow") {
        w->flexGrow = std::strtof(value.c_str(), nullptr);
        return;
    }

    if (prop == "flex-shrink") {
        w->flexShrink = std::strtof(value.c_str(), nullptr);
        return;
    }

    if (prop == "flex-basis") {
        SizeValue sv;
        if (ParseSizeValue(value, sv)) w->flexBasis = sv.number;
        return;
    }

    if (prop == "flex") {
        // Shorthand: flex: <grow> [<shrink> [<basis>]]
        float grow = 0.0f, shrink = 1.0f, basis = 0.0f;
        char* end = nullptr;
        grow = std::strtof(value.c_str(), &end);
        if (end && *end) {
            shrink = std::strtof(end, &end);
            if (end && *end) {
                basis = std::strtof(end, nullptr);
            }
        }
        w->flexGrow = grow;
        w->flexShrink = shrink;
        w->flexBasis = basis;
        if (grow > 0.0f && w->widthMode != SizeMode::Percent && w->heightMode != SizeMode::Percent) {
            bool isRow = (w->layout.direction == LayoutDirection::Row);
            // For flex items, auto-set to FillRemaining so they participate in flex distribution
            // (This is set on the parent's children, so check is deferred to layout)
        }
        return;
    }

    if (prop == "overflow") {
        if (value == "scroll" || value == "auto") {
            w->flags.clipChildren = 1;
            w->flags.scrollable = 1;
        } else if (value == "hidden") {
            w->flags.clipChildren = 1;
            w->flags.scrollable = 0;
        } else if (value == "visible") {
            w->flags.clipChildren = 0;
            w->flags.scrollable = 0;
        }
        return;
    }

    if (prop == "visibility") {
        if (value == "hidden")        w->visibility = Visibility::Hidden;
        else if (value == "collapse") w->visibility = Visibility::Collapse;
        else                          w->visibility = Visibility::Visible;
        return;
    }

    if (prop == "z-index") {
        w->zOrder = static_cast<int16_t>(std::strtol(value.c_str(), nullptr, 10));
        return;
    }

    if (prop == "pointer-events") {
        w->flags.pointerEvents = (value != "none") ? 1 : 0;
        return;
    }

    if (prop == "cursor") {
        if (value == "pointer" || value == "hand")  w->cursor = CursorType::Pointer;
        else if (value == "text")                   w->cursor = CursorType::Text;
        else if (value == "crosshair")              w->cursor = CursorType::Crosshair;
        else if (value == "move")                   w->cursor = CursorType::Move;
        else if (value == "grab")                   w->cursor = CursorType::Grab;
        else if (value == "grabbing")               w->cursor = CursorType::Grabbing;
        else if (value == "ew-resize" || value == "col-resize") w->cursor = CursorType::EWResize;
        else if (value == "ns-resize" || value == "row-resize") w->cursor = CursorType::NSResize;
        else if (value == "nwse-resize")            w->cursor = CursorType::NWSEResize;
        else if (value == "nesw-resize")            w->cursor = CursorType::NESWResize;
        else if (value == "not-allowed")            w->cursor = CursorType::NotAllowed;
        else if (value == "wait" || value == "progress") w->cursor = CursorType::Wait;
        else if (value == "none")                   w->cursor = CursorType::None;
        else                                        w->cursor = CursorType::Default;
        return;
    }

    if (prop == "resize") {
        w->flags.resizable = (value != "none") ? 1 : 0;
        return;
    }

    if (prop == "user-select") {
        if (value == "none")       w->userSelect = UserSelect::None;
        else if (value == "text")  w->userSelect = UserSelect::Text;
        else if (value == "all")   w->userSelect = UserSelect::All;
        else                       w->userSelect = UserSelect::Auto;
        return;
    }

    if (prop == "aspect-ratio") {
        // Support "16 / 9" or "1.777" or "auto"
        if (value == "auto" || value == "none") {
            w->aspectRatio = 0.0f;
        } else {
            auto slash = value.find('/');
            if (slash != std::string::npos) {
                float num = std::strtof(value.c_str(), nullptr);
                float den = std::strtof(value.c_str() + slash + 1, nullptr);
                w->aspectRatio = (den > 0.0f) ? num / den : 0.0f;
            } else {
                w->aspectRatio = std::strtof(value.c_str(), nullptr);
            }
        }
        return;
    }

    if (prop == "scroll-behavior") {
        w->scrollSmooth = (value == "smooth");
        return;
    }

    if (prop == "box-sizing") {
        if (value == "border-box")       w->boxSizing = BoxSizing::BorderBox;
        else if (value == "content-box") w->boxSizing = BoxSizing::ContentBox;
        return;
    }

    if (prop == "align-self") {
        if (value == "auto")         w->alignSelf = AlignSelf::Auto;
        else if (value == "start" || value == "flex-start") w->alignSelf = AlignSelf::Start;
        else if (value == "center")  w->alignSelf = AlignSelf::Center;
        else if (value == "end" || value == "flex-end") w->alignSelf = AlignSelf::End;
        else if (value == "stretch") w->alignSelf = AlignSelf::Stretch;
        return;
    }

    if (prop == "text-align") {
        if (value == "left")        w->textAlign = TextAlign::Left;
        else if (value == "center") w->textAlign = TextAlign::Center;
        else if (value == "right")  w->textAlign = TextAlign::Right;
        w->propsSet |= Widget::PROP_TEXT_ALIGN;
        return;
    }

    if (prop == "position") {
        if (value == "static")        w->positionMode = PositionMode::Static;
        else if (value == "relative") w->positionMode = PositionMode::Relative;
        else if (value == "absolute") w->positionMode = PositionMode::Absolute;
        return;
    }

    if (prop == "top")    { w->posTop    = std::strtof(value.c_str(), nullptr); return; }
    if (prop == "right")  { w->posRight  = std::strtof(value.c_str(), nullptr); return; }
    if (prop == "bottom") { w->posBottom = std::strtof(value.c_str(), nullptr); return; }
    if (prop == "left")   { w->posLeft   = std::strtof(value.c_str(), nullptr); return; }

    // --- Grid properties ---
    if (prop == "grid-template-columns" || prop == "grid-template-rows") {
        bool isCols = (prop == "grid-template-columns");
        auto& layout = w->grid;
        int& count = isCols ? layout.columnCount : layout.rowCount;
        auto* tracks = isCols ? layout.columns : layout.rows;
        count = 0;
        // Tokenize by spaces
        std::istringstream iss(value);
        std::string tok;
        while (iss >> tok && count < GridLayout::MAX_TRACKS) {
            auto& t = tracks[count];
            if (tok == "auto") {
                t.type = GridTrackDef::Type::Auto;
            } else if (tok == "min-content") {
                t.type = GridTrackDef::Type::MinContent;
            } else if (tok == "max-content") {
                t.type = GridTrackDef::Type::MaxContent;
            } else if (tok.size() > 2 && tok.back() == 'r' && tok[tok.size()-2] == 'f') {
                t.type = GridTrackDef::Type::Fr;
                t.value = std::strtof(tok.c_str(), nullptr);
            } else {
                SizeValue sv;
                if (ParseSizeValue(tok, sv)) {
                    t.type = GridTrackDef::Type::Px;
                    t.value = ResolveSizeUnit(sv, rootFontSize_);
                }
            }
            count++;
        }
        w->layout.isGrid = true;
        return;
    }

    if (prop == "grid-column") {
        // "start / span N" or "start / end" or just "start"
        auto slash = value.find('/');
        if (slash != std::string::npos) {
            w->gridPlace.column = std::max(1, (int)std::strtol(value.c_str(), nullptr, 10));
            std::string rest = value.substr(slash + 1);
            while (!rest.empty() && rest[0] == ' ') rest.erase(0, 1);
            if (rest.substr(0, 4) == "span") {
                w->gridPlace.columnSpan = std::max(1, (int)std::strtol(rest.c_str() + 4, nullptr, 10));
            } else {
                int end = std::max(1, (int)std::strtol(rest.c_str(), nullptr, 10));
                w->gridPlace.columnSpan = std::max(1, end - w->gridPlace.column);
            }
        } else {
            w->gridPlace.column = std::max(1, (int)std::strtol(value.c_str(), nullptr, 10));
        }
        return;
    }
    if (prop == "grid-row") {
        auto slash = value.find('/');
        if (slash != std::string::npos) {
            w->gridPlace.row = std::max(1, (int)std::strtol(value.c_str(), nullptr, 10));
            std::string rest = value.substr(slash + 1);
            while (!rest.empty() && rest[0] == ' ') rest.erase(0, 1);
            if (rest.substr(0, 4) == "span") {
                w->gridPlace.rowSpan = std::max(1, (int)std::strtol(rest.c_str() + 4, nullptr, 10));
            } else {
                int end = std::max(1, (int)std::strtol(rest.c_str(), nullptr, 10));
                w->gridPlace.rowSpan = std::max(1, end - w->gridPlace.row);
            }
        } else {
            w->gridPlace.row = std::max(1, (int)std::strtol(value.c_str(), nullptr, 10));
        }
        return;
    }
    if (prop == "column-gap" || prop == "grid-column-gap") {
        SizeValue sv;
        if (ParseSizeValue(value, sv)) w->grid.columnGap = ResolveSizeUnit(sv, rootFontSize_);
        return;
    }
    if (prop == "row-gap" || prop == "grid-row-gap") {
        SizeValue sv;
        if (ParseSizeValue(value, sv)) w->grid.rowGap = ResolveSizeUnit(sv, rootFontSize_);
        return;
    }
    if (prop == "gap") {
        // gap: rowGap columnGap -or- gap: both
        SizeValue sv;
        std::istringstream iss(value);
        std::string tok;
        if (iss >> tok && ParseSizeValue(tok, sv)) {
            float v = ResolveSizeUnit(sv, rootFontSize_);
            w->grid.rowGap = v;
            w->grid.columnGap = v;
        }
        if (iss >> tok && ParseSizeValue(tok, sv)) {
            w->grid.columnGap = ResolveSizeUnit(sv, rootFontSize_);
        }
        return;
    }

    if (prop == "line-height") {
        SizeValue sv;
        if (ParseSizeValue(value, sv)) { w->lineHeight = sv.number; w->propsSet |= Widget::PROP_LINE_HEIGHT; }
        return;
    }

    if (prop == "letter-spacing") {
        SizeValue sv;
        if (ParseSizeValue(value, sv)) { w->letterSpacing = sv.number; w->propsSet |= Widget::PROP_LETTER_SPACING; }
        return;
    }

    if (prop == "text-decoration") {
        if (value == "none")           w->textDecoration = TextDecoration::None;
        else if (value == "underline") w->textDecoration = TextDecoration::Underline;
        else if (value == "line-through") w->textDecoration = TextDecoration::Strikethrough;
        else if (value == "overline")  w->textDecoration = TextDecoration::Overline;
        return;
    }

    if (prop == "text-overflow") {
        if (value == "ellipsis") w->textOverflow = TextOverflow::Ellipsis;
        else if (value == "hidden") w->textOverflow = TextOverflow::Hidden;
        else w->textOverflow = TextOverflow::Visible;
        w->propsSet |= Widget::PROP_TEXT_OVERFLOW;
        return;
    }

    if (prop == "white-space") {
        if (value == "nowrap") {
            w->textOverflow = TextOverflow::Hidden;
            w->wordWrap = false;
        } else if (value == "normal" || value == "pre-wrap") {
            w->wordWrap = true;
        }
        w->propsSet |= Widget::PROP_TEXT_OVERFLOW | Widget::PROP_WORD_WRAP;
        return;
    }

    if (prop == "word-wrap" || prop == "overflow-wrap") {
        w->wordWrap = (value == "break-word" || value == "anywhere");
        w->propsSet |= Widget::PROP_WORD_WRAP;
        return;
    }

    if (prop == "min-width") {
        SizeValue sv;
        if (ParseSizeValue(value, sv)) w->minWidth = sv.number;
        return;
    }
    if (prop == "max-width") {
        SizeValue sv;
        if (ParseSizeValue(value, sv)) w->maxWidth = sv.number;
        return;
    }
    if (prop == "min-height") {
        SizeValue sv;
        if (ParseSizeValue(value, sv)) w->minHeight = sv.number;
        return;
    }
    if (prop == "max-height") {
        SizeValue sv;
        if (ParseSizeValue(value, sv)) w->maxHeight = sv.number;
        return;
    }
}

// ============================================================================
// CSS property application - visual (theme-aware)
// ============================================================================

void KMLBuilder::ApplyVisualProperty(int widgetIdx, const std::string& tagName,
                                       const std::string& pseudoClass,
                                       const std::string& prop, const std::string& value) {
    Widget* w = ctx_.GetWidget(widgetIdx);
    if (!w) return;

    WidgetTag wtag = w->tag;
    PseudoState pstate = pseudoClass.empty() ? PseudoState::Normal
                                              : PseudoStateFromString(pseudoClass);

    // Get current per-widget style as base (or fall back to theme), modify, and set back
    const Style* existing = theme_.GetWidget(widgetIdx, pstate);
    Style style = existing ? *existing : theme_.Get(wtag, pstate);
    bool modified = false;

    if (prop == "background-color" || prop == "background") {
        Color c;
        if (ParseColor(value, c)) {
            style.background = {c.r, c.g, c.b, c.a};
            modified = true;
        }
    }
    else if (prop == "color") {
        Color c;
        if (ParseColor(value, c)) {
            style.textColor = {c.r, c.g, c.b, c.a};
            modified = true;
            w->propsSet |= Widget::PROP_COLOR;
        }
    }
    else if (prop == "border-width") {
        style.border.width = std::strtof(value.c_str(), nullptr);
        modified = true;
    }
    else if (prop == "border-color") {
        Color c;
        if (ParseColor(value, c)) {
            style.border.color = {c.r, c.g, c.b, c.a};
            modified = true;
        }
    }
    else if (prop == "border-radius") {
        // Parse 1-4 value shorthand (CSS order: TL TR BR BL)
        std::vector<float> vals;
        std::istringstream iss(value);
        std::string token;
        while (iss >> token) {
            vals.push_back(std::strtof(token.c_str(), nullptr));
        }
        if (vals.size() == 1) {
            style.border.SetRadius(vals[0]);
        } else if (vals.size() == 2) {
            // TL+BR, TR+BL
            style.border.SetRadius(vals[0], vals[1], vals[0], vals[1]);
        } else if (vals.size() == 3) {
            // TL, TR+BL, BR
            style.border.SetRadius(vals[0], vals[1], vals[2], vals[1]);
        } else if (vals.size() >= 4) {
            style.border.SetRadius(vals[0], vals[1], vals[2], vals[3]);
        }
        modified = true;
    }
    else if (prop == "border-top-left-radius") {
        style.border.radiusTL = std::strtof(value.c_str(), nullptr);
        modified = true;
    }
    else if (prop == "border-top-right-radius") {
        style.border.radiusTR = std::strtof(value.c_str(), nullptr);
        modified = true;
    }
    else if (prop == "border-bottom-right-radius") {
        style.border.radiusBR = std::strtof(value.c_str(), nullptr);
        modified = true;
    }
    else if (prop == "border-bottom-left-radius") {
        style.border.radiusBL = std::strtof(value.c_str(), nullptr);
        modified = true;
    }
    else if (prop == "border") {
        // Shorthand: "1px solid #555" - we only use width + color
        SizeValue sv;
        std::string remaining = value;
        // Try to parse width first
        size_t space = remaining.find(' ');
        if (space != std::string::npos) {
            std::string widthPart = remaining.substr(0, space);
            if (ParseSizeValue(widthPart, sv)) {
                style.border.width = sv.number;
            }
            // Skip "solid/dashed/etc" keyword
            remaining = remaining.substr(space + 1);
            space = remaining.find(' ');
            if (space != std::string::npos) remaining = remaining.substr(space + 1);
            Color c;
            if (ParseColor(remaining, c)) {
                style.border.color = {c.r, c.g, c.b, c.a};
            }
            modified = true;
        }
    }
    else if (prop == "font-size") {
        SizeValue sv;
        if (ParseSizeValue(value, sv)) {
            float resolved = ResolveSizeUnit(sv, style.fontSize);
            style.fontSize = resolved;
            w->fontSize = resolved;
            modified = true;
            w->propsSet |= Widget::PROP_FONT_SIZE;
        }
    }
    else if (prop == "font-weight") {
        if (value == "normal") w->fontWeight = 400;
        else if (value == "bold") w->fontWeight = 700;
        else if (value == "lighter") w->fontWeight = 300;
        else if (value == "bolder") w->fontWeight = 800;
        else {
            int fw = std::strtol(value.c_str(), nullptr, 10);
            if (fw >= 100 && fw <= 900) w->fontWeight = static_cast<uint16_t>(fw);
        }
        w->propsSet |= Widget::PROP_FONT_WEIGHT;
    }
    else if (prop == "opacity") {
        style.opacity = std::strtof(value.c_str(), nullptr);
        modified = true;
        w->propsSet |= Widget::PROP_OPACITY;
    }
    else if (prop == "box-shadow") {
        // Parse: offsetX offsetY blur spread color
        // e.g. "2px 4px 8px 0px rgba(0,0,0,0.5)"
        float vals[4] = {0, 0, 0, 0};
        const char* p = value.c_str();
        char* end = nullptr;
        for (int vi = 0; vi < 4; ++vi) {
            while (*p && (*p == ' ' || *p == '\t')) ++p;
            vals[vi] = std::strtof(p, &end);
            if (end == p) break;
            p = end;
            // Skip "px"
            while (*p && (std::isalpha(static_cast<unsigned char>(*p)))) ++p;
        }
        style.shadow.offsetX = vals[0];
        style.shadow.offsetY = vals[1];
        style.shadow.blur = vals[2];
        style.shadow.spread = vals[3];
        // Try to parse trailing color
        while (*p && *p == ' ') ++p;
        if (*p) {
            Color c;
            if (ParseColor(std::string(p), c)) {
                style.shadow.color = {c.r, c.g, c.b, c.a};
            }
        }
        style.shadow.active = true;
        modified = true;
    }
    else if (prop == "background" && value.find("linear-gradient") != std::string::npos) {
        // Parse: linear-gradient(direction, color1, color2)
        size_t start = value.find('(');
        size_t end_pos = value.rfind(')');
        if (start != std::string::npos && end_pos != std::string::npos) {
            std::string inner = value.substr(start + 1, end_pos - start - 1);
            // Split on commas
            std::vector<std::string> parts;
            size_t ci = 0;
            while (ci < inner.size()) {
                size_t comma = inner.find(',', ci);
                if (comma == std::string::npos) comma = inner.size();
                std::string part = inner.substr(ci, comma - ci);
                while (!part.empty() && part[0] == ' ') part.erase(0, 1);
                while (!part.empty() && part.back() == ' ') part.pop_back();
                parts.push_back(part);
                ci = comma + 1;
            }
            if (parts.size() >= 2) {
                float angle = 180.0f;
                int colorStart = 0;
                // Check if first part is a direction
                if (parts[0].find("deg") != std::string::npos) {
                    angle = std::strtof(parts[0].c_str(), nullptr);
                    colorStart = 1;
                } else if (parts[0] == "to bottom") { angle = 180.0f; colorStart = 1; }
                else if (parts[0] == "to top") { angle = 0.0f; colorStart = 1; }
                else if (parts[0] == "to right") { angle = 90.0f; colorStart = 1; }
                else if (parts[0] == "to left") { angle = 270.0f; colorStart = 1; }

                Color c1, c2;
                if (colorStart + 1 < static_cast<int>(parts.size())) {
                    if (ParseColor(parts[colorStart], c1) && ParseColor(parts[colorStart + 1], c2)) {
                        style.gradient.from = {c1.r, c1.g, c1.b, c1.a};
                        style.gradient.to = {c2.r, c2.g, c2.b, c2.a};
                        style.gradient.angle = angle;
                        style.gradient.active = true;
                    }
                }
            }
        }
        modified = true;
    }
    else if (prop == "transition") {
        // Parse: property duration [easing]
        // We store just duration for now
        float dur = 0.0f;
        const char* p = value.c_str();
        // Skip property name
        while (*p && *p != ' ') ++p;
        while (*p == ' ') ++p;
        if (*p) {
            dur = std::strtof(p, nullptr);
            // Handle "s" and "ms"
            const char* unit = p;
            while (*unit && (*unit == '.' || std::isdigit(static_cast<unsigned char>(*unit)))) ++unit;
            if (*unit == 'm') dur /= 1000.0f; // ms to s
        }
        style.transition.duration = dur;
        style.transition.active = (dur > 0.0f);
        modified = true;
    }
    else if (prop == "transform") {
        // Parse: rotate(Ndeg) scale(N) translate(Npx, Npx)
        const std::string& v = value;
        size_t pos = 0;
        while (pos < v.size()) {
            if (v.compare(pos, 7, "rotate(") == 0) {
                style.transform.rotate = std::strtof(v.c_str() + pos + 7, nullptr);
                style.transform.active = true;
            } else if (v.compare(pos, 6, "scale(") == 0) {
                float s = std::strtof(v.c_str() + pos + 6, nullptr);
                style.transform.scaleX = s;
                style.transform.scaleY = s;
                style.transform.active = true;
            } else if (v.compare(pos, 7, "scaleX(") == 0) {
                style.transform.scaleX = std::strtof(v.c_str() + pos + 7, nullptr);
                style.transform.active = true;
            } else if (v.compare(pos, 7, "scaleY(") == 0) {
                style.transform.scaleY = std::strtof(v.c_str() + pos + 7, nullptr);
                style.transform.active = true;
            } else if (v.compare(pos, 10, "translate(") == 0) {
                char* endp = nullptr;
                style.transform.translateX = std::strtof(v.c_str() + pos + 10, &endp);
                if (endp) {
                    while (*endp && (*endp == 'p' || *endp == 'x' || *endp == ',' || *endp == ' ')) ++endp;
                    style.transform.translateY = std::strtof(endp, nullptr);
                }
                style.transform.active = true;
            }
            size_t next = v.find(')', pos);
            if (next == std::string::npos) break;
            pos = next + 1;
            while (pos < v.size() && v[pos] == ' ') ++pos;
        }
        modified = true;
    }
    else if (prop == "caret-color") {
        Color c;
        if (ParseColor(value, c)) {
            style.caretColor = {c.r, c.g, c.b, c.a};
            modified = true;
        }
    }
    else if (prop == "accent-color") {
        Color c;
        if (ParseColor(value, c)) {
            style.accentColor = {c.r, c.g, c.b, c.a};
            modified = true;
        }
    }
    else if (prop == "placeholder-color") {
        Color c;
        if (ParseColor(value, c)) {
            style.placeholderColor = {c.r, c.g, c.b, c.a};
            modified = true;
        }
    }
    else if (prop == "text-shadow") {
        // Parse: offsetX offsetY [blur] color
        float vals[3] = {0, 0, 0};
        const char* p = value.c_str();
        char* end = nullptr;
        for (int vi = 0; vi < 3; ++vi) {
            while (*p && (*p == ' ' || *p == '\t')) ++p;
            float v2 = std::strtof(p, &end);
            if (end == p) break;
            vals[vi] = v2;
            p = end;
            while (*p && (std::isalpha(static_cast<unsigned char>(*p)))) ++p;
        }
        style.textShadow.offsetX = vals[0];
        style.textShadow.offsetY = vals[1];
        style.textShadow.blur = vals[2];
        while (*p && *p == ' ') ++p;
        if (*p) {
            Color c;
            if (ParseColor(std::string(p), c)) {
                style.textShadow.color = {c.r, c.g, c.b, c.a};
            }
        }
        style.textShadow.active = true;
        modified = true;
    }
    else if (prop == "transform-origin") {
        // Parse: "center center", "top left", "50% 50%", "10px 20px"
        std::istringstream iss(value);
        std::string xPart, yPart;
        iss >> xPart;
        iss >> yPart;
        if (yPart.empty()) yPart = xPart;
        auto parseOrigin = [](const std::string& s) -> float {
            if (s == "left" || s == "top") return 0.0f;
            if (s == "center") return 0.5f;
            if (s == "right" || s == "bottom") return 1.0f;
            if (s.back() == '%') return std::strtof(s.c_str(), nullptr) / 100.0f;
            return 0.5f; // default center for px values (would need widget size)
        };
        style.transform.originX = parseOrigin(xPart);
        style.transform.originY = parseOrigin(yPart);
        modified = true;
    }

    if (modified) {
        style.isSet = true;
        theme_.SetWidget(widgetIdx, pstate, style);
    }
}

// ============================================================================
// Stylesheet application (selector matching + cascade)
// ============================================================================

bool KMLBuilder::SimpleSelectorMatches(const KSSSimpleSelector& simple,
                                         const WidgetMeta& meta) const {
    // Element match
    if (!simple.element.empty() && simple.element != "*" && simple.element != meta.tag)
        return false;
    // Class match (all classNames must be present on the element)
    for (const auto& reqClass : simple.classNames) {
        bool found = false;
        for (const auto& cls : meta.classes) {
            if (cls == reqClass) { found = true; break; }
        }
        if (!found) return false;
    }
    // ID match
    if (!simple.id.empty() && simple.id != meta.id)
        return false;

    // Structural pseudo-classes
    if (!simple.pseudoClass.empty()) {
        const auto& pc = simple.pseudoClass;
        if (pc == "first-child" && meta.childIndex != 0) return false;
        if (pc == "last-child" && meta.childIndex != meta.siblingCount - 1) return false;
        if (pc == "only-child" && meta.siblingCount != 1) return false;
        if (pc == "empty") {
            Widget* w = ctx_.GetWidget(meta.widgetIdx);
            if (w && w->childCount > 0) return false;
        }
        if (pc == "checked") {
            Widget* w = ctx_.GetWidget(meta.widgetIdx);
            if (!w || !w->checked) return false;
        }
        // :nth-child(N) - parse N from "nth-child(N)"
        if (pc.substr(0, 10) == "nth-child(") {
            int n = std::strtol(pc.c_str() + 10, nullptr, 10);
            if (n > 0 && meta.childIndex != n - 1) return false; // 1-indexed
        }
        // hover/active/focus/disabled are pseudo-states, not structural - skip here
    }

    // :not() negation
    if (!simple.negation.empty()) {
        // Parse the negation content as a mini simple selector
        KSSSimpleSelector negSel;
        const std::string& neg = simple.negation;
        if (!neg.empty()) {
            if (neg[0] == '.') {
                negSel.classNames.push_back(neg.substr(1));
            } else if (neg[0] == '#') {
                negSel.id = neg.substr(1);
            } else {
                negSel.element = neg;
            }
            // If the negation selector matches, the overall selector does NOT match
            if (SimpleSelectorMatchesBasic(negSel, meta)) return false;
        }
    }

    // Attribute selectors
    for (const auto& attrSel : simple.attrSelectors) {
        bool found = false;
        for (const auto& attr : meta.attributes) {
            if (attr.first != attrSel.name) continue;
            switch (attrSel.mode) {
                case AttrMatchMode::Exists:
                    found = true; break;
                case AttrMatchMode::Equals:
                    found = (attr.second == attrSel.value); break;
                case AttrMatchMode::Prefix:
                    found = (attr.second.substr(0, attrSel.value.size()) == attrSel.value); break;
                case AttrMatchMode::Suffix:
                    found = (attr.second.size() >= attrSel.value.size() &&
                             attr.second.substr(attr.second.size() - attrSel.value.size()) == attrSel.value); break;
                case AttrMatchMode::Contains:
                    found = (attr.second.find(attrSel.value) != std::string::npos); break;
            }
            if (found) break;
        }
        if (!found) return false;
    }

    return true;
}

bool KMLBuilder::SimpleSelectorMatchesBasic(const KSSSimpleSelector& simple,
                                              const WidgetMeta& meta) const {
    if (!simple.element.empty() && simple.element != "*" && simple.element != meta.tag)
        return false;
    for (const auto& reqClass : simple.classNames) {
        bool found = false;
        for (const auto& cls : meta.classes) {
            if (cls == reqClass) { found = true; break; }
        }
        if (!found) return false;
    }
    if (!simple.id.empty() && simple.id != meta.id)
        return false;
    return true;
}

// Test if a compound selector matches a widget, walking the ancestor chain
bool KMLBuilder::SelectorMatches(const KSSSelector& selector, int metaIdx) const {
    if (selector.parts.empty()) return false;

    // parts[0] = target element (must match metaIdx)
    if (!SimpleSelectorMatches(selector.parts[0], metas_[metaIdx]))
        return false;

    // Walk ancestor chain for parts[1], [2], ...
    int currentMeta = metaIdx;
    for (size_t i = 1; i < selector.parts.size(); ++i) {
        // The combinator on parts[i-1] (after reversal) tells how parts[i] relates
        // Note: after reversal, parts[i-1].combinator was set on the original
        // ancestor-side selector, so it describes how the CHILD relates to PARENT.
        // Actually, after reversal parts are [target, parent, grandparent...]
        // and combinator was set on the pre-reversal part, so parts[i] has the combinator
        // that was between it and parts[i-1] in source order.
        CombinatorType comb = selector.parts[i].combinator;

        bool matched = false;

        if (comb == CombinatorType::Child) {
            // Direct parent only
            int parentMeta = metas_[currentMeta].parentMetaIdx;
            if (parentMeta >= 0 && SimpleSelectorMatches(selector.parts[i], metas_[parentMeta])) {
                currentMeta = parentMeta;
                matched = true;
            }
        } else if (comb == CombinatorType::AdjacentSibling || comb == CombinatorType::GeneralSibling) {
            // Find siblings: widgets with same parent
            int parentMeta = metas_[currentMeta].parentMetaIdx;
            if (parentMeta >= 0) {
                int myIdx = metas_[currentMeta].childIndex;
                if (comb == CombinatorType::AdjacentSibling) {
                    // Check immediately preceding sibling
                    if (myIdx > 0) {
                        for (size_t si = 0; si < metas_.size(); ++si) {
                            if (metas_[si].parentMetaIdx == parentMeta &&
                                metas_[si].childIndex == myIdx - 1 &&
                                SimpleSelectorMatches(selector.parts[i], metas_[si])) {
                                currentMeta = static_cast<int>(si);
                                matched = true;
                                break;
                            }
                        }
                    }
                } else {
                    // General sibling: any preceding sibling
                    for (size_t si = 0; si < metas_.size(); ++si) {
                        if (metas_[si].parentMetaIdx == parentMeta &&
                            metas_[si].childIndex < myIdx &&
                            SimpleSelectorMatches(selector.parts[i], metas_[si])) {
                            currentMeta = static_cast<int>(si);
                            matched = true;
                            break;
                        }
                    }
                }
            }
        } else {
            // Descendant: walk up ancestry
            currentMeta = metas_[currentMeta].parentMetaIdx;
            while (currentMeta >= 0) {
                if (SimpleSelectorMatches(selector.parts[i], metas_[currentMeta])) {
                    matched = true;
                    break;
                }
                currentMeta = metas_[currentMeta].parentMetaIdx;
            }
        }
        if (!matched) return false;
    }
    return true;
}

void KMLBuilder::ApplyDeclarations(int widgetIdx, const std::string& tag,
                                     const std::string& pseudoClass,
                                     const std::vector<KSSDeclaration>& decls) {
    for (const auto& decl : decls) {
        std::string resolvedValue = ResolveVariable(decl.value);

        // Handle 'inherit' keyword - copy from parent widget's computed value
        if (resolvedValue == "inherit") {
            Widget* w = ctx_.GetWidget(widgetIdx);
            if (w && w->parent >= 0) {
                Widget* parent = ctx_.GetWidget(w->parent);
                if (parent) {
                    // Copy common inheritable properties from parent
                    if (decl.property == "color") {
                        Style ps = ctx_.GetTheme().Resolve(*parent, w->parent);
                        const Style* existing = ctx_.GetTheme().GetWidget(widgetIdx, PseudoState::Normal);
                        Style cs = existing ? *existing : ctx_.GetTheme().Get(w->tag, PseudoState::Normal);
                        cs.textColor = ps.textColor;
                        cs.isSet = true;
                        ctx_.GetTheme().SetWidget(widgetIdx, PseudoState::Normal, cs);
                    } else if (decl.property == "font-size") {
                        w->fontSize = parent->fontSize;
                    }
                }
            }
            continue;
        }

        // Handle 'initial' keyword - reset to default
        if (resolvedValue == "initial") {
            // Skip - effectively a no-op since defaults are already set
            continue;
        }

        // Layout properties (cursor, display, gap, etc.) are on the Widget struct
        // and don't support per-state variants. Only apply for normal state.
        if (pseudoClass.empty()) {
            ApplyLayoutProperty(widgetIdx, decl.property, resolvedValue);
        }
        ApplyVisualProperty(widgetIdx, tag, pseudoClass, decl.property, resolvedValue);
    }
}

// Resolve var() references and calc() expressions in a CSS value
std::string KMLBuilder::ResolveVariable(const std::string& value) const {
    // Resolve calc() expressions first
    std::string resolved = value;

    // Handle calc(expr) - supports +, -, *, / with proper precedence, multi-operand
    size_t calcPos = resolved.find("calc(");
    if (calcPos != std::string::npos) {
        size_t start = calcPos + 5;
        int depth = 1;
        size_t end = start;
        while (end < resolved.size() && depth > 0) {
            if (resolved[end] == '(') depth++;
            if (resolved[end] == ')') depth--;
            if (depth > 0) end++;
        }
        std::string expr = resolved.substr(start, end - start);

        // Tokenize: extract values and operators
        // Values can be "100px", "50%", "2em", bare numbers
        // Operators: +, -, *, / (must be surrounded by spaces for +/-)
        struct CalcToken { float value; char op; bool isOp; };
        std::vector<CalcToken> tokens;

        size_t ti = 0;
        while (ti < expr.size()) {
            // Skip whitespace
            while (ti < expr.size() && expr[ti] == ' ') ti++;
            if (ti >= expr.size()) break;

            // Check for operator (+ and - need space before to distinguish from sign)
            if (tokens.size() > 0 && !tokens.back().isOp) {
                if (expr[ti] == '*' || expr[ti] == '/') {
                    tokens.push_back({0, expr[ti], true});
                    ti++;
                    continue;
                }
                if ((expr[ti] == '+' || expr[ti] == '-') && ti > 0 && expr[ti-1] == ' ') {
                    tokens.push_back({0, expr[ti], true});
                    ti++;
                    continue;
                }
            }

            // Parse value token
            size_t valStart = ti;
            // Handle sign
            if (expr[ti] == '-' || expr[ti] == '+') ti++;
            while (ti < expr.size() && (std::isdigit(expr[ti]) || expr[ti] == '.')) ti++;
            // Parse unit suffix
            size_t unitStart = ti;
            while (ti < expr.size() && std::isalpha(expr[ti]) && expr[ti] != ' ') ti++;
            // Also handle % sign
            if (ti < expr.size() && expr[ti] == '%') ti++;

            std::string valStr = expr.substr(valStart, ti - valStart);
            SizeValue sv;
            if (ParseSizeValue(valStr, sv)) {
                float val = ResolveSizeUnit(sv, rootFontSize_);
                if (sv.unit == SizeUnit::Percent) val = sv.number;
                tokens.push_back({val, 0, false});
            }
        }

        // Evaluate with precedence: first pass handles * and /
        std::vector<CalcToken> addSub;
        for (size_t i = 0; i < tokens.size(); i++) {
            if (tokens[i].isOp && (tokens[i].op == '*' || tokens[i].op == '/')) {
                if (!addSub.empty() && !addSub.back().isOp && i + 1 < tokens.size()) {
                    float left = addSub.back().value;
                    float right = tokens[i + 1].value;
                    addSub.back().value = (tokens[i].op == '*') ? left * right
                                                                 : (right != 0.0f ? left / right : 0.0f);
                    i++; // skip right operand
                }
            } else {
                addSub.push_back(tokens[i]);
            }
        }

        // Second pass: handle + and -
        float result = 0.0f;
        if (!addSub.empty()) {
            result = addSub[0].value;
            for (size_t i = 1; i + 1 < addSub.size(); i += 2) {
                if (addSub[i].isOp) {
                    float right = addSub[i + 1].value;
                    if (addSub[i].op == '+') result += right;
                    else result -= right;
                }
            }
        }
        // Replace calc(...) with the result as px
        char buf[32];
        std::snprintf(buf, sizeof(buf), "%.1fpx", result);
        resolved = resolved.substr(0, calcPos) + buf + resolved.substr(end + 1);
    }

    // Look for var(--name) or var(--name, fallback)
    size_t pos = resolved.find("var(");
    if (pos == std::string::npos) return resolved;

    std::string result;
    size_t i = 0;
    while (i < resolved.size()) {
        size_t varStart = resolved.find("var(", i);
        if (varStart == std::string::npos) {
            result += resolved.substr(i);
            break;
        }
        result += resolved.substr(i, varStart - i);

        size_t contentStart = varStart + 4;
        int depth = 1;
        size_t contentEnd = contentStart;
        while (contentEnd < resolved.size() && depth > 0) {
            if (resolved[contentEnd] == '(') depth++;
            if (resolved[contentEnd] == ')') depth--;
            if (depth > 0) contentEnd++;
        }

        std::string content = resolved.substr(contentStart, contentEnd - contentStart);
        // Split on comma for fallback
        std::string varName, fallback;
        size_t comma = content.find(',');
        if (comma != std::string::npos) {
            varName = content.substr(0, comma);
            fallback = content.substr(comma + 1);
            // Trim
            while (!fallback.empty() && fallback[0] == ' ') fallback.erase(0, 1);
        } else {
            varName = content;
        }
        // Trim varName
        while (!varName.empty() && varName.back() == ' ') varName.pop_back();
        while (!varName.empty() && varName[0] == ' ') varName.erase(0, 1);

        auto it = cssVariables_.find(varName);
        if (it != cssVariables_.end()) {
            result += it->second;
        } else if (!fallback.empty()) {
            result += fallback;
        }

        i = contentEnd + 1; // skip ')'
    }
    return result;
}

// Apply all stylesheet rules to the widget tree via selector matching
void KMLBuilder::ApplyStylesheet(const KSSStylesheet& stylesheet) {
    // Clear per-widget style overrides before reapplying
    theme_.ClearWidgetStyles();

    // Load CSS variables
    for (const auto& var : stylesheet.variables) {
        cssVariables_[var.property] = var.value;
    }

    // Collect all matching rules per widget, sorted by specificity
    struct Match {
        int specificity;
        std::string pseudoClass;
        const std::vector<KSSDeclaration>* decls;
    };

    for (size_t mi = 0; mi < metas_.size(); ++mi) {
        std::vector<Match> matches;

        for (const auto& rule : stylesheet.rules) {
            // Skip rules that don't match current @media conditions
            if (rule.mediaQuery.type != MediaQueryType::None) {
                bool mediaMatch = false;
                switch (rule.mediaQuery.type) {
                    case MediaQueryType::MaxWidth:
                        mediaMatch = (viewportW_ <= rule.mediaQuery.value); break;
                    case MediaQueryType::MinWidth:
                        mediaMatch = (viewportW_ >= rule.mediaQuery.value); break;
                    case MediaQueryType::MaxHeight:
                        mediaMatch = (viewportH_ <= rule.mediaQuery.value); break;
                    case MediaQueryType::MinHeight:
                        mediaMatch = (viewportH_ >= rule.mediaQuery.value); break;
                    default: break;
                }
                if (!mediaMatch) continue;
            }

            for (const auto& sel : rule.selectors) {
                if (SelectorMatches(sel, static_cast<int>(mi))) {
                    // Extract pseudo-class from target selector
                    std::string pseudo = sel.parts[0].pseudoClass;
                    // Filter out structural pseudo-classes from theme state
                    if (pseudo == "first-child" || pseudo == "last-child" ||
                        pseudo == "only-child" || pseudo == "empty" ||
                        pseudo == "checked" || pseudo == "root" ||
                        pseudo.substr(0, 10) == "nth-child(") {
                        pseudo = ""; // These are structural, not state pseudo-classes
                    }
                    matches.push_back({sel.Specificity(), pseudo, &rule.declarations});
                }
            }
        }

        // Sort by specificity (ascending - last wins like CSS)
        std::sort(matches.begin(), matches.end(),
                  [](const Match& a, const Match& b) { return a.specificity < b.specificity; });

        // Apply in order
        for (const auto& m : matches) {
            ApplyDeclarations(metas_[mi].widgetIdx, metas_[mi].tag, m.pseudoClass, *m.decls);
        }

        // Handle ::before and ::after pseudo-elements
        for (const auto& rule : stylesheet.rules) {
            for (const auto& sel : rule.selectors) {
                // Check if pseudo-class is "before" or "after"
                if (sel.parts.empty()) continue;
                const auto& pseudo = sel.parts[0].pseudoClass;
                if (pseudo != "before" && pseudo != "after") continue;

                // Match without the pseudo-element
                KSSSelector baseSel = sel;
                baseSel.parts[0].pseudoClass = "";
                if (!SelectorMatches(baseSel, static_cast<int>(mi))) continue;

                // Find content declaration
                std::string content;
                for (const auto& decl : rule.declarations) {
                    if (decl.property == "content") {
                        content = decl.value;
                        // Strip quotes
                        if (content.size() >= 2 &&
                            ((content.front() == '"' && content.back() == '"') ||
                             (content.front() == '\'' && content.back() == '\''))) {
                            content = content.substr(1, content.size() - 2);
                        }
                        break;
                    }
                }

                // Create a label widget for the pseudo-element
                int parentW = metas_[mi].widgetIdx;
                std::string pseudoId = metas_[mi].id + "_" + pseudo;
                int pseudoIdx = ctx_.CreateLabel(pseudoId.c_str(), content.c_str());
                if (pseudoIdx >= 0) {
                    if (pseudo == "before") {
                        // Insert as first child (shift others)
                        ctx_.SetParent(pseudoIdx, parentW);
                    } else {
                        ctx_.SetParent(pseudoIdx, parentW);
                    }
                    // Apply style declarations to the pseudo-element
                    for (const auto& decl : rule.declarations) {
                        if (decl.property == "content") continue;
                        std::string resolvedVal = ResolveVariable(decl.value);
                        ApplyLayoutProperty(pseudoIdx, decl.property, resolvedVal);
                        ApplyVisualProperty(pseudoIdx, "label", "", decl.property, resolvedVal);
                    }
                }
            }
        }
    }
}

// ============================================================================
// Element tree -> widget tree
// ============================================================================

// Recursively build a widget from a parsed element and its children
int KMLBuilder::BuildElement(const KMLElement& elem, int parentIdx, int parentMetaIdx) {
    // Skip text pseudo-elements
    if (elem.tag == "_text") return -1;

    int widgetIdx = CreateWidget(elem.tag, elem.id);
    if (widgetIdx < 0) {
        Error(elem.line, "Failed to create widget <" + elem.tag + ">");
        return -1;
    }

    // Set parent
    if (parentIdx >= 0) {
        ctx_.SetParent(widgetIdx, parentIdx);
    }

    // Compute child index among siblings
    int childIndex = 0;
    if (parentMetaIdx >= 0) {
        // Count how many metas share this parent
        for (size_t i = 0; i < metas_.size(); ++i) {
            if (metas_[i].parentMetaIdx == parentMetaIdx)
                ++childIndex;
        }
    }

    // Store meta for selector matching
    int metaIdx = static_cast<int>(metas_.size());
    WidgetMeta meta;
    meta.widgetIdx = widgetIdx;
    meta.tag = elem.tag;
    meta.id = elem.id;
    meta.classes = elem.classes;
    meta.parentMetaIdx = parentMetaIdx;
    meta.childIndex = childIndex;
    // Store raw attributes for attribute selector matching
    for (const auto& attr : elem.attributes) {
        meta.attributes.push_back({attr.name, attr.value});
    }
    meta.siblingCount = 0; // updated after all siblings are built
    // Capture inline style for re-application after stylesheet (CSS specificity)
    for (const auto& attr : elem.attributes) {
        if (attr.name == "style") { meta.inlineStyle = attr.value; break; }
    }
    metas_.push_back(meta);

    // Apply inline attributes
    ApplyAttributes(widgetIdx, elem);

    ++widgetsCreated_;

    // Recursively build children
    int childCount = 0;
    for (const auto& child : elem.children) {
        if (child.tag != "_text") childCount++;
    }
    for (const auto& child : elem.children) {
        BuildElement(child, widgetIdx, metaIdx);
    }

    // Update siblingCount for all children of this element
    for (size_t i = 0; i < metas_.size(); ++i) {
        if (metas_[i].parentMetaIdx == metaIdx) {
            metas_[i].siblingCount = childCount;
        }
    }

    return widgetIdx;
}

// ============================================================================
// Public API
// ============================================================================

BuildResult KMLBuilder::Build(const std::vector<KMLElement>& elements,
                                const KSSStylesheet& stylesheet) {
    metas_.clear();
    errors_.clear();
    widgetsCreated_ = 0;
    floatingPanelCount_ = 0;

    int rootIdx = -1;
    for (const auto& elem : elements) {
        int idx = BuildElement(elem, -1, -1);
        if (rootIdx < 0) rootIdx = idx;
    }

    // Apply stylesheet after all widgets exist (for descendant selectors)
    ApplyStylesheet(stylesheet);

    // Re-apply inline styles - CSS semantics: inline has highest specificity
    for (const auto& meta : metas_) {
        if (meta.inlineStyle.empty()) continue;
        size_t pos = 0;
        const auto& value = meta.inlineStyle;
        while (pos < value.size()) {
            size_t colon = value.find(':', pos);
            if (colon == std::string::npos) break;
            size_t semi = value.find(';', colon);
            if (semi == std::string::npos) semi = value.size();
            std::string prop = value.substr(pos, colon - pos);
            std::string val  = value.substr(colon + 1, semi - colon - 1);
            auto trim = [](std::string s) {
                size_t a = s.find_first_not_of(" \t");
                size_t b = s.find_last_not_of(" \t");
                return (a == std::string::npos) ? "" : s.substr(a, b - a + 1);
            };
            prop = trim(prop);
            val  = trim(val);
            for (auto& c : prop) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            ApplyLayoutProperty(meta.widgetIdx, prop, val);
            ApplyVisualProperty(meta.widgetIdx, meta.tag, "", prop, val);
            pos = semi + 1;
        }
    }

    // Inherit cascading properties from parent to child
    if (rootIdx >= 0) InheritProperties(rootIdx);

    // Resolve submenu references (MenuItem submenu="popup-id" -> submenuIdx)
    ResolveSubmenuReferences(rootIdx);

    return {rootIdx, errors_, widgetsCreated_};
}

// Build the complete widget tree from elements without a stylesheet
BuildResult KMLBuilder::Build(const std::vector<KMLElement>& elements) {
    KSSStylesheet empty;
    return Build(elements, empty);
}

// Resolve a SizeValue to pixels based on unit type
float KMLBuilder::ResolveSizeUnit(const SizeValue& sv, float elementFontSize) const {
    switch (sv.unit) {
        case SizeUnit::Px:      return sv.number;
        case SizeUnit::Em:      return sv.number * elementFontSize;
        case SizeUnit::Rem:     return sv.number * rootFontSize_;
        case SizeUnit::Vw:      return sv.number * viewportW_ / 100.0f;
        case SizeUnit::Vh:      return sv.number * viewportH_ / 100.0f;
        case SizeUnit::Percent: return sv.number; // caller handles %
        default:                return sv.number;
    }
}

// Record a build error with source line information
void KMLBuilder::Error(int line, const std::string& msg) {
    errors_.push_back({line, 0, msg});
}

// Cascade inheritable CSS properties (font, color, etc.) from parent to child
void KMLBuilder::InheritProperties(int widgetIdx) {
    Widget* w = ctx_.Pool().Get(widgetIdx);
    if (!w) return;

    Widget* parent = (w->parent >= 0) ? ctx_.Pool().Get(w->parent) : nullptr;

    if (parent) {
        // Inherit font-size
        if (!(w->propsSet & Widget::PROP_FONT_SIZE)) {
            w->fontSize = parent->fontSize;
        }
        // Inherit font-weight
        if (!(w->propsSet & Widget::PROP_FONT_WEIGHT)) {
            w->fontWeight = parent->fontWeight;
        }
        // Inherit letter-spacing
        if (!(w->propsSet & Widget::PROP_LETTER_SPACING)) {
            w->letterSpacing = parent->letterSpacing;
        }
        // Inherit line-height
        if (!(w->propsSet & Widget::PROP_LINE_HEIGHT)) {
            w->lineHeight = parent->lineHeight;
        }
        // Inherit text-align
        if (!(w->propsSet & Widget::PROP_TEXT_ALIGN)) {
            w->textAlign = parent->textAlign;
        }
        // Inherit text-overflow
        if (!(w->propsSet & Widget::PROP_TEXT_OVERFLOW)) {
            w->textOverflow = parent->textOverflow;
        }
        // Inherit word-wrap
        if (!(w->propsSet & Widget::PROP_WORD_WRAP)) {
            w->wordWrap = parent->wordWrap;
        }
        // Inherit color (text color from per-widget style)
        if (!(w->propsSet & Widget::PROP_COLOR)) {
            // Resolve parent's text color and apply to this widget's per-widget style
            Style parentStyle = theme_.Resolve(*parent, static_cast<int>(w->parent));
            // Check all pseudo-states for this widget
            for (int s = 0; s < static_cast<int>(PseudoState::Count); ++s) {
                auto ps = static_cast<PseudoState>(s);
                const Style* ws = theme_.GetWidget(widgetIdx, ps);
                if (ws) {
                    Style updated = *ws;
                    updated.textColor = parentStyle.textColor;
                    theme_.SetWidget(widgetIdx, ps, updated);
                } else if (s == 0) {
                    // Set Normal state with inherited color
                    Style base = theme_.Get(w->tag, PseudoState::Normal);
                    base.textColor = parentStyle.textColor;
                    theme_.SetWidget(widgetIdx, PseudoState::Normal, base);
                }
            }
        }
    }

    // Recurse into children
    for (int i = 0; i < w->childCount; ++i) {
        InheritProperties(w->children[i]);
    }
}

// Resolve submenu="id" attributes to widget indices
void KMLBuilder::ResolveSubmenuReferences(int rootIdx) {
    if (rootIdx < 0) return;

    // Build id->index map from metas
    std::unordered_map<std::string, int> idMap;
    for (const auto& m : metas_) {
        if (!m.id.empty()) idMap[m.id] = m.widgetIdx;
    }

    // Walk all widgets and resolve submenuIdx == -2 sentinel
    for (auto& m : metas_) {
        Widget* w = ctx_.GetWidget(m.widgetIdx);
        if (!w || w->tag != WidgetTag::MenuItem || w->submenuIdx != -2) continue;

        const char* targetId = ctx_.Strings().Lookup(w->panelTitleId);
        if (!targetId || targetId[0] == '\0') {
            w->submenuIdx = -1;
            continue;
        }

        auto it = idMap.find(targetId);
        if (it != idMap.end()) {
            w->submenuIdx = static_cast<int16_t>(it->second);
            // Clear the temporary reuse of panelTitleId
            w->panelTitleId = NullStringId;
        } else {
            Error(0, "submenu reference '" + std::string(targetId) + "' not found");
            w->submenuIdx = -1;
        }
    }
}

} // namespace markup
} // namespace ui
} // namespace koilo
