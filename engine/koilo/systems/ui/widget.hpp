// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file widget.hpp
 * @brief Core widget system for the Koilo UI engine.
 *
 * Retained-mode widget tree with tag-based type dispatch, pre-allocated
 * widget pool, and FNV-1a string interning. No vtable dispatch.
 *
 * @date 03/08/2026
 * @author Coela Can't
 */

#pragma once

#include <cstdint>
#include <cstring>
#include <cmath>
#include <string>
#include <functional>
#include <vector>
#include <koilo/registry/reflect_macros.hpp>
#include <koilo/systems/ui/icon_ids.hpp>
#include "drag_drop.hpp"

namespace koilo {
namespace ui {

// --- String Interning ---

/// Interned string handle. Zero is the null/empty string.
using StringId = uint32_t;
static constexpr StringId NullStringId = 0;

/// FNV-1a hash for string interning.
inline constexpr uint32_t Fnv1a(const char* s, uint32_t h = 2166136261u) {
    return (*s == '\0') ? h : Fnv1a(s + 1, (h ^ static_cast<uint32_t>(*s)) * 16777619u);
}

/**
 * @class StringTable
 * @brief Global string intern table with fixed capacity, open addressing, and heap-allocated storage.
 */
class StringTable {
public:
    static constexpr size_t CAPACITY = 8192;      ///< Maximum number of interned strings.
    static constexpr size_t MAX_LEN  = 63;         ///< Maximum length of a single interned string.
    static constexpr size_t ENTRY_SIZE = MAX_LEN + 1; ///< Storage size per entry including null terminator.

    StringTable()
        : hashes_(CAPACITY, 0)
        , strings_(CAPACITY * ENTRY_SIZE, '\0') {
    }

    ~StringTable() = default;

    StringTable(const StringTable&) = delete;
    StringTable& operator=(const StringTable&) = delete;

    /**
     * @brief Intern a string, returning its unique hash identifier.
     * @param str Null-terminated string to intern.
     * @return The StringId hash, or NullStringId if the string is empty or the table is full.
     */
    StringId Intern(const char* str) {
        if (!str || str[0] == '\0') return NullStringId;
        uint32_t hash = Fnv1a(str);
        if (hash == NullStringId) hash = 1;
        uint32_t idx = hash & (CAPACITY - 1);
        for (size_t probe = 0; probe < CAPACITY; ++probe) {
            uint32_t i = (idx + probe) & (CAPACITY - 1);
            if (hashes_[i] == 0) {
                hashes_[i] = hash;
                size_t len = strlen(str);
                if (len > MAX_LEN) len = MAX_LEN;
                char* dest = &strings_[i * ENTRY_SIZE];
                memcpy(dest, str, len);
                dest[len] = '\0';
                count_++;
                return hash;
            }
            if (hashes_[i] == hash && strcmp(&strings_[i * ENTRY_SIZE], str) == 0) {
                return hash;
            }
        }
        return NullStringId; // table full
    }

    /**
     * @brief Look up a previously interned string by its identifier.
     * @param id The StringId to look up.
     * @return Pointer to the interned string, or "" if not found.
     */
    const char* Lookup(StringId id) const {
        if (id == NullStringId) return "";
        uint32_t idx = id & (CAPACITY - 1);
        for (size_t probe = 0; probe < CAPACITY; ++probe) {
            uint32_t i = (idx + probe) & (CAPACITY - 1);
            if (hashes_[i] == 0) return "";
            if (hashes_[i] == id) return &strings_[i * ENTRY_SIZE];
        }
        return "";
    }

    /** @brief Get the number of interned strings. */
    size_t Count() const { return count_; }

private:
    std::vector<uint32_t> hashes_;  ///< Hash table for string lookups.
    std::vector<char> strings_;    ///< Contiguous storage for interned string data.
    size_t count_ = 0;            ///< Current number of interned strings.

    KL_BEGIN_FIELDS(StringTable)
        /* No reflected fields - internal hash table. */
    KL_END_FIELDS

    KL_BEGIN_METHODS(StringTable)
        KL_METHOD_AUTO(StringTable, Count, "Count")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(StringTable)
        /* Non-copyable - no reflected ctors. */
    KL_END_DESCRIBE(StringTable)

};

// --- Widget Types ---

enum class WidgetTag : uint8_t {
    Panel = 0,     ///< Generic container panel.
    Label,         ///< Static text label.
    Button,        ///< Clickable button.
    TextField,     ///< Editable text input.
    Slider,        ///< Draggable value slider.
    Checkbox,      ///< Boolean toggle checkbox.
    Dropdown,      ///< Dropdown selection menu.
    ScrollView,    ///< Scrollable container.
    Separator,     ///< Visual divider line.
    Image,         ///< Texture/image display.
    TreeNode,      ///< Collapsible tree hierarchy node.
    ColorField,    ///< Color picker input.
    Viewport,      ///< 3D/2D viewport render target.
    DockContainer, ///< Dockable panel container.
    SplitPane,     ///< Resizable split panel.
    TabBar,        ///< Tab navigation bar.
    PopupMenu,     ///< Context/popup menu.
    MenuItem,      ///< Single item in a menu.
    ProgressBar,   ///< Progress indicator bar.
    ToggleSwitch,  ///< On/off toggle switch.
    RadioButton,   ///< Mutually exclusive radio option.
    NumberSpinner, ///< Numeric up/down spinner.
    FloatingPanel, ///< Detachable floating window.
    VirtualList,   ///< Virtualized scrolling list.
    Canvas2D,      ///< Custom 2D drawing surface.
    Count          ///< Total number of widget types.
};

// --- Layout ---

enum class LayoutDirection : uint8_t {
    Row = 0,
    Column
};

enum class Alignment : uint8_t {
    Start = 0,        ///< Align items to the start of the axis.
    Center,           ///< Center items along the axis.
    End,              ///< Align items to the end of the axis.
    Stretch,          ///< Stretch items to fill the axis.
    SpaceBetween,     ///< Equal space between items, no edge space.
    SpaceAround,      ///< Equal space around each item.
    SpaceEvenly       ///< Equal space between and around items.
};

enum class SizeMode : uint8_t {
    Fixed = 0,       ///< Fixed pixel size.
    Percent,         ///< Percentage of parent size.
    FitContent,      ///< Shrink to fit content.
    FillRemaining,   ///< Expand to fill remaining space.
    Auto,            ///< Default: acts like Fixed but allows cross-axis stretch.
    MinContent,      ///< Shrink to smallest possible without overflow.
    MaxContent       ///< Expand to fit all content without wrapping.
};

enum class TextAlign : uint8_t {
    Left = 0,
    Center,
    Right
};

enum class PositionMode : uint8_t {
    Static = 0,
    Relative,
    Absolute
};

enum class TextOverflow : uint8_t {
    Visible = 0,
    Hidden,
    Ellipsis
};

enum class TextDecoration : uint8_t {
    None = 0,
    Underline   = 1 << 0,
    Strikethrough = 1 << 1,
    Overline    = 1 << 2
};

enum class BoxSizing : uint8_t {
    ContentBox = 0,
    BorderBox
};

enum class CursorType : uint8_t {
    Default = 0,
    Pointer,     ///< Hand/link cursor.
    Text,        ///< I-beam text cursor.
    Crosshair,   ///< Crosshair cursor.
    Move,        ///< Move cursor.
    Grab,        ///< Open hand grab cursor.
    Grabbing,    ///< Closed hand grabbing cursor.
    EWResize,    ///< Horizontal resize <->.
    NSResize,    ///< Vertical resize ↕.
    NWSEResize,  ///< Diagonal resize ↘.
    NESWResize,  ///< Diagonal resize ↙.
    NotAllowed,  ///< Not-allowed cursor.
    Wait,        ///< Busy/wait cursor.
    None         ///< Hidden cursor.
};

enum class AlignSelf : uint8_t {
    Auto = 0,  ///< Inherit parent's crossAlign.
    Start,     ///< Align self to start.
    Center,    ///< Center self.
    End,       ///< Align self to end.
    Stretch    ///< Stretch self to fill.
};

enum class Visibility : uint8_t {
    Visible = 0,
    Hidden,    ///< Invisible but preserves layout space.
    Collapse   ///< Invisible and removed from layout (like display:none).
};

enum class UserSelect : uint8_t {
    Auto = 0,  ///< Default behavior.
    None,      ///< Prevent text selection.
    Text,      ///< Allow text selection.
    All        ///< Select all on click.
};

/**
 * @class LayoutParams
 * @brief Layout parameters controlling child arrangement within a container.
 */
struct LayoutParams {
    LayoutDirection direction = LayoutDirection::Column; ///< Direction of child layout flow.
    Alignment mainAlign = Alignment::Start;   ///< Alignment along the main axis.
    Alignment crossAlign = Alignment::Stretch; ///< Alignment along the cross axis.
    float spacing = 0.0f;  ///< Spacing between children in pixels.
    bool wrap = false;     ///< Whether children wrap to the next line.
    bool isGrid = false;   ///< True if this container uses grid layout.

    KL_BEGIN_FIELDS(LayoutParams)
        KL_FIELD(LayoutParams, direction, "Direction", 0, 0)
    KL_END_FIELDS

    KL_BEGIN_METHODS(LayoutParams)
        /* No reflected methods. */
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(LayoutParams)
        /* No reflected ctors. */
    KL_END_DESCRIBE(LayoutParams)

};

/**
 * @class GridTrackDef
 * @brief Grid track definition: a column or row size specification.
 */
struct GridTrackDef {
    enum class Type : uint8_t { Px, Fr, Auto, MinContent, MaxContent };
    Type type = Type::Auto;  ///< Track sizing strategy.
    float value = 0.0f;      ///< Pixel value or fractional unit (fr).

    KL_BEGIN_FIELDS(GridTrackDef)
        KL_FIELD(GridTrackDef, type, "Type", 0, 0)
    KL_END_FIELDS

    KL_BEGIN_METHODS(GridTrackDef)
        /* No reflected methods. */
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(GridTrackDef)
        /* No reflected ctors. */
    KL_END_DESCRIBE(GridTrackDef)

};

/**
 * @class GridLayout
 * @brief Grid layout parameters stored on a parent container.
 */
struct GridLayout {
    static constexpr int MAX_TRACKS = 16; ///< Maximum number of grid tracks per axis.
    GridTrackDef columns[MAX_TRACKS]{};   ///< Column track definitions.
    GridTrackDef rows[MAX_TRACKS]{};      ///< Row track definitions.
    int columnCount = 0;   ///< Number of defined columns.
    int rowCount = 0;      ///< Number of defined rows.
    float columnGap = 0.0f; ///< Gap between columns in pixels.
    float rowGap = 0.0f;    ///< Gap between rows in pixels.

    KL_BEGIN_FIELDS(GridLayout)
        /* No reflected fields. */
    KL_END_FIELDS

    KL_BEGIN_METHODS(GridLayout)
        /* No reflected methods. */
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(GridLayout)
        /* No reflected ctors. */
    KL_END_DESCRIBE(GridLayout)

};

/**
 * @class GridPlacement
 * @brief Grid placement for a child within a grid container.
 */
struct GridPlacement {
    int column = 0;     ///< 0-based column start (0 = auto).
    int row = 0;        ///< 0-based row start (0 = auto).
    int columnSpan = 1; ///< Number of columns to span.
    int rowSpan = 1;    ///< Number of rows to span.

    KL_BEGIN_FIELDS(GridPlacement)
        KL_FIELD(GridPlacement, column, "Column", -2147483648, 2147483647)
    KL_END_FIELDS

    KL_BEGIN_METHODS(GridPlacement)
        /* No reflected methods. */
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(GridPlacement)
        /* No reflected ctors. */
    KL_END_DESCRIBE(GridPlacement)

};

/**
 * @class Edges
 * @brief Inset values for each side of a rectangular region (padding, margin, border).
 */
struct Edges {
    float top = 0.0f;
    float right = 0.0f;
    float bottom = 0.0f;
    float left = 0.0f;

    KL_BEGIN_FIELDS(Edges)
        KL_FIELD(Edges, top, "Top", __FLT_MIN__, __FLT_MAX__),
        KL_FIELD(Edges, right, "Right", __FLT_MIN__, __FLT_MAX__),
        KL_FIELD(Edges, bottom, "Bottom", __FLT_MIN__, __FLT_MAX__),
        KL_FIELD(Edges, left, "Left", __FLT_MIN__, __FLT_MAX__)
    KL_END_FIELDS

    KL_BEGIN_METHODS(Edges)
        /* No reflected methods. */
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(Edges)
        /* No reflected ctors. */
    KL_END_DESCRIBE(Edges)

};

/**
 * @class Anchors
 * @brief Anchor offsets relative to a parent, expressed in the 0.0–1.0 range.
 */
struct Anchors {
    float minX = 0.0f;
    float minY = 0.0f;
    float maxX = 0.0f;
    float maxY = 0.0f;

    KL_BEGIN_FIELDS(Anchors)
        KL_FIELD(Anchors, minX, "Min x", __FLT_MIN__, __FLT_MAX__),
        KL_FIELD(Anchors, minY, "Min y", __FLT_MIN__, __FLT_MAX__),
        KL_FIELD(Anchors, maxX, "Max x", __FLT_MIN__, __FLT_MAX__),
        KL_FIELD(Anchors, maxY, "Max y", __FLT_MIN__, __FLT_MAX__)
    KL_END_FIELDS

    KL_BEGIN_METHODS(Anchors)
        /* No reflected methods. */
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(Anchors)
        /* No reflected ctors. */
    KL_END_DESCRIBE(Anchors)

};

/**
 * @class Rect
 * @brief Axis-aligned rectangle defined by position and size.
 */
struct Rect {
    float x = 0.0f;
    float y = 0.0f;
    float w = 0.0f;
    float h = 0.0f;

    /**
     * @brief Test whether a point lies inside this rectangle.
     * @param px X coordinate of the point.
     * @param py Y coordinate of the point.
     * @return True if the point is inside the rectangle.
     */
    bool Contains(float px, float py) const {
        return px >= x && px < x + w && py >= y && py < y + h;
    }

    KL_BEGIN_FIELDS(Rect)
        KL_FIELD(Rect, x, "X", __FLT_MIN__, __FLT_MAX__),
        KL_FIELD(Rect, y, "Y", __FLT_MIN__, __FLT_MAX__),
        KL_FIELD(Rect, w, "W", __FLT_MIN__, __FLT_MAX__),
        KL_FIELD(Rect, h, "H", __FLT_MIN__, __FLT_MAX__)
    KL_END_FIELDS

    KL_BEGIN_METHODS(Rect)
        /* No reflected methods. */
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(Rect)
        /* No reflected ctors. */
    KL_END_DESCRIBE(Rect)

};

// --- Color ---

/**
 * @class Color4
 * @brief RGBA color with 8-bit per channel, plus HSV and hex conversion utilities.
 */
struct Color4 {
    uint8_t r = 0, g = 0, b = 0, a = 255;
    Color4() = default;
    Color4(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) : r(r), g(g), b(b), a(a) {}

    /// Create from HSV (h: 0-360, s: 0-1, v: 0-1).
    static Color4 FromHSV(float h, float s, float v, uint8_t a = 255) {
        float c = v * s;
        float x = c * (1.0f - std::fabs(std::fmod(h / 60.0f, 2.0f) - 1.0f));
        float m = v - c;
        float r1, g1, b1;
        if      (h < 60)  { r1 = c; g1 = x; b1 = 0; }
        else if (h < 120) { r1 = x; g1 = c; b1 = 0; }
        else if (h < 180) { r1 = 0; g1 = c; b1 = x; }
        else if (h < 240) { r1 = 0; g1 = x; b1 = c; }
        else if (h < 300) { r1 = x; g1 = 0; b1 = c; }
        else              { r1 = c; g1 = 0; b1 = x; }
        return {static_cast<uint8_t>((r1 + m) * 255.0f + 0.5f),
                static_cast<uint8_t>((g1 + m) * 255.0f + 0.5f),
                static_cast<uint8_t>((b1 + m) * 255.0f + 0.5f), a};
    }

    /// Convert to HSV (h: 0-360, s: 0-1, v: 0-1).
    void ToHSV(float& h, float& s, float& v) const {
        float rf = r / 255.0f, gf = g / 255.0f, bf = b / 255.0f;
        float maxC = std::fmax(rf, std::fmax(gf, bf));
        float minC = std::fmin(rf, std::fmin(gf, bf));
        float delta = maxC - minC;
        v = maxC;
        s = (maxC > 0.0f) ? delta / maxC : 0.0f;
        if (delta < 1e-6f) { h = 0.0f; return; }
        if (maxC == rf)       h = 60.0f * std::fmod((gf - bf) / delta, 6.0f);
        else if (maxC == gf)  h = 60.0f * ((bf - rf) / delta + 2.0f);
        else                  h = 60.0f * ((rf - gf) / delta + 4.0f);
        if (h < 0.0f) h += 360.0f;
    }

    /// Create from hex string (e.g., "#FF8800" or "FF8800FF").
    static Color4 FromHex(const char* hex) {
        if (!hex) return {};
        if (hex[0] == '#') ++hex;
        unsigned int val = 0;
        int len = 0;
        for (const char* p = hex; *p && len < 8; ++p, ++len) {
            char c = *p;
            unsigned int d = 0;
            if (c >= '0' && c <= '9') d = c - '0';
            else if (c >= 'a' && c <= 'f') d = c - 'a' + 10;
            else if (c >= 'A' && c <= 'F') d = c - 'A' + 10;
            else break;
            val = (val << 4) | d;
        }
        if (len == 6) return {static_cast<uint8_t>((val >> 16) & 0xFF),
                              static_cast<uint8_t>((val >> 8) & 0xFF),
                              static_cast<uint8_t>(val & 0xFF)};
        if (len == 8) return {static_cast<uint8_t>((val >> 24) & 0xFF),
                              static_cast<uint8_t>((val >> 16) & 0xFF),
                              static_cast<uint8_t>((val >> 8) & 0xFF),
                              static_cast<uint8_t>(val & 0xFF)};
        return {};
    }
};

// --- Widget ---

/**
 * @class WidgetFlags
 * @brief Packed bitfield flags for common boolean widget state.
 */
struct WidgetFlags {
    uint16_t visible    : 1; ///< Widget is visible.
    uint16_t enabled    : 1; ///< Widget accepts interaction.
    uint16_t focusable  : 1; ///< Widget can receive keyboard focus.
    uint16_t focused    : 1; ///< Widget currently has focus.
    uint16_t hovered    : 1; ///< Pointer is over the widget.
    uint16_t pressed    : 1; ///< Widget is being pressed.
    uint16_t dirty      : 1; ///< Needs layout recompute.
    uint16_t clipChildren : 1; ///< Clip child rendering to bounds.
    uint16_t pointerEvents : 1; ///< 1 = normal (default), 0 = pass-through.
    uint16_t resizable  : 1;    ///< Children can be resized via divider drag.
    uint16_t scrollable : 1;    ///< Element scrolls like ScrollView.
    uint16_t focusVisible : 1;  ///< Focus from keyboard (Tab), show focus ring.
    uint16_t selected   : 1;    ///< Widget is selected (tree nodes, list items).
    uint16_t minimized  : 1;    ///< Floating panel is minimized to tray.

    KL_BEGIN_FIELDS(WidgetFlags)
        /* No reflected fields. */
    KL_END_FIELDS

    KL_BEGIN_METHODS(WidgetFlags)
        /* No reflected methods. */
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(WidgetFlags)
        /* No reflected ctors. */
    KL_END_DESCRIBE(WidgetFlags)

};

static constexpr int MAX_CHILDREN = 64; ///< Maximum number of children per widget.

/**
 * @class Widget
 * @brief Core UI widget node in the retained-mode widget tree.
 *
 * Stores identity, layout, style, tag-specific data (label, slider, checkbox, etc.),
 * and event callbacks. Uses tag-based type dispatch instead of vtable inheritance.
 */
struct Widget {
    // -- Identity ------------------------------------------------
    StringId id = NullStringId; ///< Interned string identifier.
    WidgetTag tag = WidgetTag::Panel; ///< Widget type tag.

    // -- Tree ----------------------------------------------------
    int16_t parent = -1;              ///< Pool index of the parent widget (-1 = root).
    int16_t children[MAX_CHILDREN]{}; ///< Pool indices of child widgets.
    uint8_t childCount = 0;           ///< Number of active children.

    // -- Local rect (user-specified position/size) ---------------
    float localX = 0.0f; ///< Local X position within parent.
    float localY = 0.0f; ///< Local Y position within parent.
    float localW = 100.0f; ///< Local width in pixels.
    float localH = 30.0f;  ///< Local height in pixels.

    // -- Size modes ----------------------------------------------
    SizeMode widthMode  = SizeMode::Auto; ///< Width sizing strategy.
    SizeMode heightMode = SizeMode::Auto; ///< Height sizing strategy.

    // -- Min/max constraints (0 = no constraint) -----------------
    float minWidth  = 0.0f; ///< Minimum width constraint.
    float maxWidth  = 0.0f; ///< Maximum width constraint.
    float minHeight = 0.0f; ///< Minimum height constraint.
    float maxHeight = 0.0f; ///< Maximum height constraint.

    // -- Anchors (relative to parent, 0.0-1.0) ------------------
    Anchors anchors{}; ///< Anchor points relative to parent bounds.

    // -- Computed global rect (set by layout pass) ---------------
    Rect computedRect{}; ///< Final screen-space rectangle after layout.

    // -- Layout params (for children) ----------------------------
    LayoutParams layout{}; ///< Child layout configuration.
    GridLayout grid{};       ///< Grid track definitions (when layout.isGrid).
    GridPlacement gridPlace{}; ///< Grid placement for children of grid containers.
    Edges padding{};  ///< Inner padding edges.
    Edges margin{};   ///< Outer margin edges.
    // Bitfield: which margin edges are 'auto' (for centering)
    struct { uint8_t top:1, right:1, bottom:1, left:1; } marginAuto = {0,0,0,0};

    // -- Flex properties ------------------------------------------
    float flexGrow = 0.0f;   ///< Flex grow factor.
    float flexShrink = 1.0f; ///< Flex shrink factor.
    float flexBasis = 0.0f;  ///< Flex basis size (0 = auto, uses localW/localH).
    AlignSelf alignSelf = AlignSelf::Auto; ///< Per-widget cross-axis alignment override.
    BoxSizing boxSizing = BoxSizing::BorderBox; ///< Box sizing model.

    // -- Position mode --------------------------------------------
    PositionMode positionMode = PositionMode::Static; ///< CSS-style position mode.
    float posTop = 0.0f;    ///< Top offset for positioned elements.
    float posRight = 0.0f;  ///< Right offset for positioned elements.
    float posBottom = 0.0f; ///< Bottom offset for positioned elements.
    float posLeft = 0.0f;   ///< Left offset for positioned elements.

    // -- Text properties ------------------------------------------
    TextDecoration textDecoration = TextDecoration::None; ///< Text decoration style.
    TextOverflow textOverflow = TextOverflow::Visible;    ///< Text overflow behavior.
    float lineHeight = 0.0f;    ///< Line height in pixels (0 = auto, 1.2 * fontSize).
    float letterSpacing = 0.0f; ///< Extra spacing between characters.
    bool wordWrap = false;      ///< True to wrap text at word boundaries.

    // -- State flags ----------------------------------------------
    WidgetFlags flags{}; ///< Packed boolean state flags.

    // -- Z-order --------------------------------------------------
    int16_t zOrder = 0; ///< Z-order within parent (higher = on top).

    // -- Cursor style ---------------------------------------------
    CursorType cursor = CursorType::Default; ///< Mouse cursor when hovering this widget.

    // -- Visibility -----------------------------------------------
    Visibility visibility = Visibility::Visible; ///< Visibility mode (hidden preserves layout space).

    // -- User select behavior -------------------------------------
    UserSelect userSelect = UserSelect::Auto; ///< Text selection behavior.

    // -- Aspect ratio ---------------------------------------------
    float aspectRatio = 0.0f; ///< Aspect ratio constraint (0 = none, e.g. 16/9 = 1.777).

    // -- Scroll offset (for ScrollView) ---------------------------
    float scrollX = 0.0f;       ///< Horizontal scroll offset.
    float scrollY = 0.0f;       ///< Vertical scroll offset.
    float contentHeight = 0.0f; ///< Total content height (set by layout for scrollbar).
    bool scrollSmooth = false;  ///< True to enable smooth scroll animation.

    // ---- Tag-specific data ----------------------------------------

    // -- Label / Button text --------------------------------------
    StringId textId = NullStringId; ///< Interned text content.
    float fontSize = 14.0f;         ///< Font size in pixels.
    TextAlign textAlign = TextAlign::Left; ///< Horizontal text alignment.
    uint16_t fontWeight = 400; ///< CSS font-weight: 100–900 (400=normal, 700=bold).

    // -- Icon (rendered before label) -----------------------------
    IconId iconId = IconId::None; ///< Icon rendered before label in TreeNode/MenuItem/Button.

    // -- Slider ---------------------------------------------------
    float sliderValue = 0.0f; ///< Current slider value.
    float sliderMin = 0.0f;   ///< Slider minimum value.
    float sliderMax = 1.0f;   ///< Slider maximum value.

    // -- Checkbox -------------------------------------------------
    bool checked = false; ///< Checkbox checked state.

    // -- TextField ------------------------------------------------
    int cursorPos = 0;                       ///< Text cursor position.
    StringId placeholderId = NullStringId;    ///< Interned placeholder text.

    // -- TreeNode -------------------------------------------------
    bool expanded = false;       ///< Whether the tree node is expanded.
    int treeDepth = 0;           ///< Nesting depth in the tree.
    bool treeHasChildren = false; ///< Whether this node has child nodes.

    // -- TreeNode multi-column and action icons ---------------------
    static constexpr int MAX_TREE_COLUMNS = 4; ///< Maximum columns in a tree node.
    static constexpr int MAX_ACTION_ICONS = 4; ///< Maximum action icons per tree node.

    /**
     * @class TreeColumn
     * @brief A single column within a multi-column tree node.
     */
    struct TreeColumn {
        StringId textId = NullStringId;
        float    width  = 0.0f;          ///< Column width in px (0 = auto)

        KL_BEGIN_FIELDS(TreeColumn)
            KL_FIELD(TreeColumn, textId, "Text id", 0, 0),
            KL_FIELD(TreeColumn, width, "Width", __FLT_MIN__, __FLT_MAX__)
        KL_END_FIELDS

        KL_BEGIN_METHODS(TreeColumn)
            /* No reflected methods. */
        KL_END_METHODS

        KL_BEGIN_DESCRIBE(TreeColumn)
            /* No reflected ctors. */
        KL_END_DESCRIBE(TreeColumn)

    };

    /**
     * @class ActionIcon
     * @brief An interactive icon button attached to a tree node.
     */
    struct ActionIcon {
        IconId icon = IconId::None;    ///< Icon identifier.
        bool   toggled = false;        ///< Toggle state (e.g. eye on/off).
        std::function<void(Widget&, int /*actionIdx*/)> onClick; ///< Click callback.

        KL_BEGIN_FIELDS(ActionIcon)
            KL_FIELD(ActionIcon, icon, "Icon", 0, 0)
        KL_END_FIELDS

        KL_BEGIN_METHODS(ActionIcon)
            /* No reflected methods. */
        KL_END_METHODS

        KL_BEGIN_DESCRIBE(ActionIcon)
            /* No reflected ctors. */
        KL_END_DESCRIBE(ActionIcon)

    };
    TreeColumn treeColumns[MAX_TREE_COLUMNS]{}; ///< Multi-column data for tree nodes.
    ActionIcon actionIcons[MAX_ACTION_ICONS]{}; ///< Action icon buttons on tree nodes.
    uint8_t treeColumnCount = 0; ///< Number of active tree columns.
    uint8_t actionIconCount = 0; ///< Number of active action icons.

    // -- FloatingPanel --------------------------------------------
    static constexpr float TITLE_BAR_HEIGHT = 24.0f; ///< Height of the floating panel title bar.
    static constexpr float RESIZE_EDGE = 5.0f;       ///< Edge hit area width for resize handles.
    StringId panelTitleId = NullStringId; ///< Interned title text for floating panels.
    int dockedParentIdx = -1;  ///< Original parent before undock (-1 = never docked).
    int dockedChildPos = -1;   ///< Original position in parent's children array.

    // -- Image / Viewport -----------------------------------------
    uint32_t textureId = 0; ///< GPU texture handle for Image or Viewport.

    // -- Color field ----------------------------------------------
    Color4 colorValue{255, 255, 255, 255}; ///< Current color value.

    // -- Dropdown -------------------------------------------------
    int selectedIndex = 0;    ///< Currently selected dropdown index.
    bool dropdownOpen = false; ///< Whether the dropdown popup is open.

    // -- ProgressBar ----------------------------------------------
    float progressValue = 0.0f; ///< Progress value in the 0.0–1.0 range.

    // -- ToggleSwitch (reuses 'checked' from Checkbox) ------------

    // -- RadioButton ----------------------------------------------
    StringId radioGroup = NullStringId; ///< Group name for mutual exclusion.

    // -- NumberSpinner (reuses sliderValue/sliderMin/sliderMax) ----
    float spinnerStep = 1.0f; ///< Increment per click/scroll.

    // -- MenuItem sub-menu and shortcut ---------------------------
    int16_t submenuIdx = -1;               ///< Pool index of child PopupMenu (-1 = none).
    StringId shortcutTextId = NullStringId; ///< Display text for keyboard shortcut (e.g., "Ctrl+Z").

    // -- VirtualList ----------------------------------------------
    int virtualItemCount = 0;        ///< Total number of items in the data source.
    float virtualItemHeight = 24.0f; ///< Fixed height per item in pixels.
    /// Called to populate a widget for a given data index.
    std::function<void(Widget& /*itemWidget*/, int /*dataIndex*/)> virtualBind;

    // -- Canvas2D -------------------------------------------------
    /// Paint callback (CanvasDrawContext* passed as void* to avoid circular include).
    std::function<void(void* /*CanvasDrawContext*/ )> onPaint;

    // -- Event callbacks (script function names) ------------------
    StringId onClickId    = NullStringId; ///< Script callback for click events.
    StringId onChangeId   = NullStringId; ///< Script callback for value change events.
    StringId onActivateId = NullStringId; ///< Script callback for activation events.

    // -- C++ callbacks --------------------------------------------
    std::function<void(Widget&)> onClickCpp;  ///< Native click callback.
    std::function<void(Widget&)> onChangeCpp; ///< Native value change callback.

    // -- Drag-and-drop callbacks ----------------------------------
    std::function<DragPayload(int /*widgetIdx*/)> onDragBegin;   ///< Return payload to start drag.
    std::function<bool(const DragPayload&)>       onCanDrop;     ///< Can this widget accept a drop?
    std::function<void(const DragPayload&, DropPosition)> onDrop; ///< Handle a dropped payload.
    uint32_t acceptDropTypes = 0; ///< Bitmask of accepted DragType tags (0 = no drops accepted).

    // -- Accessibility --------------------------------------------
    StringId ariaLabel = NullStringId; ///< Screen reader text.

    // -- Tooltip --------------------------------------------------
    StringId tooltipId = NullStringId; ///< Hover tooltip text.

    // -- Inheritable property tracking -----------------------------
    /// Bitmask: which inheritable properties were explicitly set by KSS/inline style.
    /// If a bit is NOT set, the property inherits from the parent widget.
    enum InheritProp : uint16_t {
        PROP_COLOR         = 1 << 0,  ///< Text color was explicitly set.
        PROP_FONT_SIZE     = 1 << 1,  ///< Font size was explicitly set.
        PROP_FONT_WEIGHT   = 1 << 2,  ///< Font weight was explicitly set.
        PROP_LETTER_SPACING= 1 << 3,  ///< Letter spacing was explicitly set.
        PROP_LINE_HEIGHT   = 1 << 4,  ///< Line height was explicitly set.
        PROP_TEXT_ALIGN    = 1 << 5,  ///< Text alignment was explicitly set.
        PROP_TEXT_OVERFLOW  = 1 << 6, ///< Text overflow was explicitly set.
        PROP_WORD_WRAP     = 1 << 7,  ///< Word wrap was explicitly set.
        PROP_OPACITY       = 1 << 8,  ///< Opacity was explicitly set.
    };
    uint16_t propsSet = 0; ///< Bitmask of explicitly set inheritable properties.

    /// Returns true if this widget participates in layout flow.
    /// Collapse and display:none both remove from layout.
    bool IsLayoutVisible() const {
        return flags.visible && visibility != Visibility::Collapse;
    }

    Widget() {
        flags.visible = 1;
        flags.enabled = 1;
        flags.focusable = 0;
        flags.focused = 0;
        flags.hovered = 0;
        flags.pressed = 0;
        flags.dirty = 1;
        flags.clipChildren = 0;
        flags.pointerEvents = 1;
        flags.resizable = 0;
        flags.scrollable = 0;
        flags.focusVisible = 0;
        memset(children, -1, sizeof(children));
    }

    KL_BEGIN_FIELDS(Widget)
        KL_FIELD(Widget, marginAuto, "Margin auto", 0, 0),
        KL_FIELD(Widget, propsSet, "Props set", 0, 65535)
    KL_END_FIELDS

    KL_BEGIN_METHODS(Widget)
        /* No reflected methods. */
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(Widget)
        KL_CTOR0(Widget)
    KL_END_DESCRIBE(Widget)

};

// --- Widget Pool ---

/**
 * @class WidgetPool
 * @brief Pre-allocated pool for Widget instances with O(1) allocation and deallocation.
 */
class WidgetPool {
public:
    static constexpr size_t DEFAULT_CAPACITY = 4096; ///< Default pool capacity.

    /**
     * @brief Construct a widget pool with the given capacity.
     * @param capacity Maximum number of widgets the pool can hold.
     */
    explicit WidgetPool(size_t capacity = DEFAULT_CAPACITY)
        : widgets_(capacity), alive_(capacity, false), capacity_(capacity) {
    }

    ~WidgetPool() = default;

    WidgetPool(const WidgetPool&) = delete;
    WidgetPool& operator=(const WidgetPool&) = delete;

    /// Allocate a widget from the pool. Returns index or -1 if full.
    int Allocate() {
        // Start searching from last freed position for cache locality
        for (size_t i = 0; i < capacity_; ++i) {
            size_t idx = (nextFree_ + i) % capacity_;
            if (!alive_[idx]) {
                alive_[idx] = true;
                widgets_[idx] = Widget(); // reset to defaults
                count_++;
                nextFree_ = (idx + 1) % capacity_;
                return static_cast<int>(idx);
            }
        }
        return -1;
    }

    /// Free a widget back to the pool.
    void Free(int index) {
        if (index < 0 || static_cast<size_t>(index) >= capacity_) return;
        if (!alive_[index]) return;
        alive_[index] = false;
        count_--;
        nextFree_ = static_cast<size_t>(index);
    }

    /**
     * @brief Get a widget by pool index.
     * @param index Pool index of the widget.
     * @return Pointer to the widget, or nullptr if invalid or dead.
     */
    Widget* Get(int index) {
        if (index < 0 || static_cast<size_t>(index) >= capacity_) return nullptr;
        if (!alive_[index]) return nullptr;
        return &widgets_[index];
    }

    /** @brief Get a const widget by pool index. */
    const Widget* Get(int index) const {
        if (index < 0 || static_cast<size_t>(index) >= capacity_) return nullptr;
        if (!alive_[index]) return nullptr;
        return &widgets_[index];
    }

    /**
     * @brief Check whether a widget at the given index is alive.
     * @param index Pool index to check.
     * @return True if the widget is allocated and alive.
     */
    bool IsAlive(int index) const {
        if (index < 0 || static_cast<size_t>(index) >= capacity_) return false;
        return alive_[index];
    }

    /** @brief Get the number of live widgets. */
    size_t Count() const { return count_; }
    /** @brief Get the total pool capacity. */
    size_t Capacity() const { return capacity_; }

private:
    std::vector<Widget> widgets_;  ///< Contiguous widget storage.
    std::vector<bool> alive_;      ///< Per-slot alive flags.
    size_t capacity_ = 0;        ///< Total number of slots.
    size_t count_ = 0;           ///< Number of currently allocated widgets.
    size_t nextFree_ = 0;        ///< Hint for next free slot search.

    KL_BEGIN_FIELDS(WidgetPool)
        KL_FIELD(WidgetPool, capacity_, "Capacity", 0, 0)
    KL_END_FIELDS

    KL_BEGIN_METHODS(WidgetPool)
        KL_METHOD_AUTO(WidgetPool, Free, "Free"),
        KL_METHOD_AUTO(WidgetPool, IsAlive, "Is alive"),
        KL_METHOD_AUTO(WidgetPool, Count, "Count"),
        KL_METHOD_AUTO(WidgetPool, Capacity, "Capacity")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(WidgetPool)
        /* Non-copyable - no reflected ctors. */
    KL_END_DESCRIBE(WidgetPool)

};

} // namespace ui
} // namespace koilo
