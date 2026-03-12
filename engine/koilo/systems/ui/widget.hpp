// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file widget.hpp
 * @brief Core widget system for the Koilo UI engine.
 *
 * Retained-mode widget tree with tag-based type dispatch, pre-allocated
 * widget pool, and FNV-1a string interning. No vtable dispatch.
 *
 * @date 03/08/2026
 * @author Coela
 */

#pragma once

#include <cstdint>
#include <cstring>
#include <string>
#include <functional>
#include <koilo/registry/reflect_macros.hpp>

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

/// Global string intern table. Fixed capacity, open addressing, heap-allocated.
class StringTable {
public:
    static constexpr size_t CAPACITY = 8192;
    static constexpr size_t MAX_LEN  = 63;
    static constexpr size_t ENTRY_SIZE = MAX_LEN + 1;

    StringTable() {
        hashes_ = new uint32_t[CAPACITY]();
        strings_ = new char[CAPACITY * ENTRY_SIZE]();
    }

    ~StringTable() {
        delete[] hashes_;
        delete[] strings_;
    }

    StringTable(const StringTable&) = delete;
    StringTable& operator=(const StringTable&) = delete;

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
                char* dest = strings_ + i * ENTRY_SIZE;
                memcpy(dest, str, len);
                dest[len] = '\0';
                count_++;
                return hash;
            }
            if (hashes_[i] == hash && strcmp(strings_ + i * ENTRY_SIZE, str) == 0) {
                return hash;
            }
        }
        return NullStringId; // table full
    }

    const char* Lookup(StringId id) const {
        if (id == NullStringId) return "";
        uint32_t idx = id & (CAPACITY - 1);
        for (size_t probe = 0; probe < CAPACITY; ++probe) {
            uint32_t i = (idx + probe) & (CAPACITY - 1);
            if (hashes_[i] == 0) return "";
            if (hashes_[i] == id) return strings_ + i * ENTRY_SIZE;
        }
        return "";
    }

    size_t Count() const { return count_; }

private:
    uint32_t* hashes_ = nullptr;
    char* strings_ = nullptr;
    size_t count_ = 0;
};

// --- Widget Types ---

enum class WidgetTag : uint8_t {
    Panel = 0,
    Label,
    Button,
    TextField,
    Slider,
    Checkbox,
    Dropdown,
    ScrollView,
    Separator,
    Image,
    TreeNode,
    ColorField,
    Viewport,
    DockContainer,
    SplitPane,
    TabBar,
    PopupMenu,
    MenuItem,
    ProgressBar,
    ToggleSwitch,
    RadioButton,
    NumberSpinner,
    FloatingPanel,
    Count
};

// --- Layout ---

enum class LayoutDirection : uint8_t {
    Row = 0,
    Column
};

enum class Alignment : uint8_t {
    Start = 0,
    Center,
    End,
    Stretch,
    SpaceBetween,
    SpaceAround,
    SpaceEvenly
};

enum class SizeMode : uint8_t {
    Fixed = 0,
    Percent,
    FitContent,
    FillRemaining,
    Auto,          // Default: acts like Fixed but allows cross-axis stretch
    MinContent,    // Shrink to smallest possible without overflow
    MaxContent     // Expand to fit all content without wrapping
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
    Pointer,     // hand/link
    Text,        // I-beam
    Crosshair,
    Move,
    Grab,
    Grabbing,
    EWResize,    // horizontal resize ↔
    NSResize,    // vertical resize ↕
    NWSEResize,  // diagonal resize ↘
    NESWResize,  // diagonal resize ↙
    NotAllowed,
    Wait,
    None         // hidden cursor
};

enum class AlignSelf : uint8_t {
    Auto = 0,  // inherit parent's crossAlign
    Start,
    Center,
    End,
    Stretch
};

enum class Visibility : uint8_t {
    Visible = 0,
    Hidden,    // invisible but preserves layout space
    Collapse   // invisible and removed from layout (like display:none)
};

enum class UserSelect : uint8_t {
    Auto = 0,  // default behavior
    None,      // prevent text selection
    Text,      // allow text selection
    All        // select all on click
};

struct LayoutParams {
    LayoutDirection direction = LayoutDirection::Column;
    Alignment mainAlign = Alignment::Start;
    Alignment crossAlign = Alignment::Stretch;
    float spacing = 0.0f;
    bool wrap = false;
    bool isGrid = false;
};

/// Grid track definition: a column or row size
struct GridTrackDef {
    enum class Type : uint8_t { Px, Fr, Auto, MinContent, MaxContent };
    Type type = Type::Auto;
    float value = 0.0f; // px value or fr fraction
};

/// Grid layout parameters (stored on parent)
struct GridLayout {
    static constexpr int MAX_TRACKS = 16;
    GridTrackDef columns[MAX_TRACKS]{};
    GridTrackDef rows[MAX_TRACKS]{};
    int columnCount = 0;
    int rowCount = 0;
    float columnGap = 0.0f;
    float rowGap = 0.0f;
};

/// Grid placement (stored on child)
struct GridPlacement {
    int column = 0;     // 0-based column start (0 = auto)
    int row = 0;        // 0-based row start (0 = auto)
    int columnSpan = 1; // how many columns to span
    int rowSpan = 1;    // how many rows to span
};

struct Edges {
    float top = 0.0f;
    float right = 0.0f;
    float bottom = 0.0f;
    float left = 0.0f;
};

struct Anchors {
    float minX = 0.0f;
    float minY = 0.0f;
    float maxX = 0.0f;
    float maxY = 0.0f;
};

struct Rect {
    float x = 0.0f;
    float y = 0.0f;
    float w = 0.0f;
    float h = 0.0f;

    bool Contains(float px, float py) const {
        return px >= x && px < x + w && py >= y && py < y + h;
    }
};

// --- Color ---

struct Color4 {
    uint8_t r = 0, g = 0, b = 0, a = 255;
    Color4() = default;
    Color4(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) : r(r), g(g), b(b), a(a) {}
};

// --- Widget ---

/// Flags packed into a single byte for common boolean state.
struct WidgetFlags {
    uint16_t visible    : 1;
    uint16_t enabled    : 1;
    uint16_t focusable  : 1;
    uint16_t focused    : 1;
    uint16_t hovered    : 1;
    uint16_t pressed    : 1;
    uint16_t dirty      : 1;  // needs layout recompute
    uint16_t clipChildren : 1;
    uint16_t pointerEvents : 1; // 1 = normal (default), 0 = none (pass-through)
    uint16_t resizable  : 1;    // 1 = children can be resized via divider drag
    uint16_t scrollable : 1;    // 1 = element scrolls like ScrollView
    uint16_t focusVisible : 1;  // 1 = focus from keyboard (Tab), show focus ring
    uint16_t selected   : 1;    // 1 = widget is selected (tree nodes, list items)
    uint16_t minimized  : 1;    // 1 = floating panel is minimized to tray
};

static constexpr int MAX_CHILDREN = 64;

struct Widget {
    // Identity
    StringId id = NullStringId;
    WidgetTag tag = WidgetTag::Panel;

    // Tree
    int16_t parent = -1;
    int16_t children[MAX_CHILDREN]{};
    uint8_t childCount = 0;

    // Local rect (user-specified position/size)
    float localX = 0.0f;
    float localY = 0.0f;
    float localW = 100.0f;
    float localH = 30.0f;

    // Size modes
    SizeMode widthMode  = SizeMode::Auto;
    SizeMode heightMode = SizeMode::Auto;

    // Min/max constraints (0 = no constraint)
    float minWidth  = 0.0f;
    float maxWidth  = 0.0f;
    float minHeight = 0.0f;
    float maxHeight = 0.0f;

    // Anchors (relative to parent, 0.0-1.0)
    Anchors anchors{};

    // Computed global rect (set by layout pass)
    Rect computedRect{};

    // Layout params (for children)
    LayoutParams layout{};
    GridLayout grid{};       // Grid track definitions (when layout.isGrid)
    GridPlacement gridPlace{}; // Grid placement (for children of grid containers)
    Edges padding{};
    Edges margin{};
    // Bitfield: which margin edges are 'auto' (for centering)
    struct { uint8_t top:1, right:1, bottom:1, left:1; } marginAuto = {0,0,0,0};

    // Flex properties
    float flexGrow = 0.0f;
    float flexShrink = 1.0f;
    float flexBasis = 0.0f; // 0 = auto (use localW/localH)
    AlignSelf alignSelf = AlignSelf::Auto;
    BoxSizing boxSizing = BoxSizing::BorderBox;

    // Position mode
    PositionMode positionMode = PositionMode::Static;
    float posTop = 0.0f;
    float posRight = 0.0f;
    float posBottom = 0.0f;
    float posLeft = 0.0f;

    // Text properties
    TextDecoration textDecoration = TextDecoration::None;
    TextOverflow textOverflow = TextOverflow::Visible;
    float lineHeight = 0.0f;   // 0 = auto (1.2 * fontSize)
    float letterSpacing = 0.0f;
    bool wordWrap = false;     // true = wrap text at word boundaries

    // State flags
    WidgetFlags flags{};

    // Z-order within parent (higher = on top)
    int16_t zOrder = 0;

    // Cursor style
    CursorType cursor = CursorType::Default;

    // Visibility (hidden preserves layout space, unlike flags.visible)
    Visibility visibility = Visibility::Visible;

    // User select behavior
    UserSelect userSelect = UserSelect::Auto;

    // Aspect ratio (0 = none, e.g. 16.0f/9.0f = 1.777...)
    float aspectRatio = 0.0f;

    // Scroll offset (for ScrollView)
    float scrollX = 0.0f;
    float scrollY = 0.0f;
    float contentHeight = 0.0f; // Total content height (set by layout for scrollbar)
    bool scrollSmooth = false;  // true = smooth scroll animation

    // ---- Tag-specific data (union would save space but complicates reflection) ----

    // Label / Button text
    StringId textId = NullStringId;
    float fontSize = 14.0f;
    TextAlign textAlign = TextAlign::Left;
    uint16_t fontWeight = 400; // CSS font-weight: 100-900 (400=normal, 700=bold)

    // Slider
    float sliderValue = 0.0f;
    float sliderMin = 0.0f;
    float sliderMax = 1.0f;

    // Checkbox
    bool checked = false;

    // TextField
    int cursorPos = 0;
    StringId placeholderId = NullStringId;

    // TreeNode
    bool expanded = false;
    int treeDepth = 0;
    bool treeHasChildren = false;

    // FloatingPanel
    static constexpr float TITLE_BAR_HEIGHT = 24.0f;
    static constexpr float RESIZE_EDGE = 5.0f;
    StringId panelTitleId = NullStringId;
    int dockedParentIdx = -1;   // original parent before undock (-1 = never docked)
    int dockedChildPos = -1;    // original position in parent's children array

    // Image / Viewport
    uint32_t textureId = 0;

    // Color field
    Color4 colorValue{255, 255, 255, 255};

    // Dropdown
    int selectedIndex = 0;
    bool dropdownOpen = false;

    // ProgressBar
    float progressValue = 0.0f; // 0.0 - 1.0

    // ToggleSwitch (reuses 'checked' from Checkbox)

    // RadioButton
    StringId radioGroup = NullStringId; // group name for mutual exclusion

    // NumberSpinner (reuses sliderValue/sliderMin/sliderMax from Slider)
    float spinnerStep = 1.0f; // increment per click/scroll

    // Event callbacks (script function names)
    StringId onClickId    = NullStringId;
    StringId onChangeId   = NullStringId;
    StringId onActivateId = NullStringId;

    // C++ callbacks
    std::function<void(Widget&)> onClickCpp;
    std::function<void(Widget&)> onChangeCpp;

    // Accessibility
    StringId ariaLabel = NullStringId;  // screen reader text

    // Tooltip
    StringId tooltipId = NullStringId;  // hover tooltip text

    // Bitmask: which inheritable properties were explicitly set by KSS/inline style.
    // If a bit is NOT set, the property inherits from the parent widget.
    enum InheritProp : uint16_t {
        PROP_COLOR         = 1 << 0,
        PROP_FONT_SIZE     = 1 << 1,
        PROP_FONT_WEIGHT   = 1 << 2,
        PROP_LETTER_SPACING= 1 << 3,
        PROP_LINE_HEIGHT   = 1 << 4,
        PROP_TEXT_ALIGN    = 1 << 5,
        PROP_TEXT_OVERFLOW  = 1 << 6,
        PROP_WORD_WRAP     = 1 << 7,
        PROP_OPACITY       = 1 << 8,
    };
    uint16_t propsSet = 0;

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
};

// --- Widget Pool ---

class WidgetPool {
public:
    static constexpr size_t DEFAULT_CAPACITY = 4096;

    explicit WidgetPool(size_t capacity = DEFAULT_CAPACITY)
        : capacity_(capacity) {
        widgets_ = new Widget[capacity_]();
        alive_ = new bool[capacity_]();
    }

    ~WidgetPool() {
        delete[] widgets_;
        delete[] alive_;
    }

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

    Widget* Get(int index) {
        if (index < 0 || static_cast<size_t>(index) >= capacity_) return nullptr;
        if (!alive_[index]) return nullptr;
        return &widgets_[index];
    }

    const Widget* Get(int index) const {
        if (index < 0 || static_cast<size_t>(index) >= capacity_) return nullptr;
        if (!alive_[index]) return nullptr;
        return &widgets_[index];
    }

    bool IsAlive(int index) const {
        if (index < 0 || static_cast<size_t>(index) >= capacity_) return false;
        return alive_[index];
    }

    size_t Count() const { return count_; }
    size_t Capacity() const { return capacity_; }

private:
    Widget* widgets_ = nullptr;
    bool* alive_ = nullptr;
    size_t capacity_ = 0;
    size_t count_ = 0;
    size_t nextFree_ = 0;
};

} // namespace ui
} // namespace koilo
