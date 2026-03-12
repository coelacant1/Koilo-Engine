// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file auto_inspector.cpp
 * @brief Reflection-driven inspector UI generator implementation.
 * @date 03/08/2026
 * @author Coela
 */

#include "auto_inspector.hpp"
#include <koilo/registry/global_registry.hpp>
#include <koilo/registry/type_registry.hpp>
#include <cstdio>
#include <cstring>
#include <cmath>

namespace koilo {
namespace ui {

// -- helpers ------------------------------------------------------------

static constexpr size_t ID_BUF = 128;
static constexpr float  LABEL_W  = 140.0f;
static constexpr float  ROW_H    = 24.0f;
static constexpr float  SPACING  = 4.0f;
static constexpr float  CAT_PAD  = 6.0f;

/// Build a deterministic widget ID from class + field name.
static void MakeId(char* buf, const char* cls, const char* field,
                   const char* suffix = nullptr) {
    if (suffix)
        snprintf(buf, ID_BUF, "insp_%s_%s_%s", cls, field, suffix);
    else
        snprintf(buf, ID_BUF, "insp_%s_%s", cls, field);
}

/// Create a label-value row: [ label | value widget ].
/// Returns the row panel index.  value widget is placed by caller.
static int CreateRow(UIContext& ctx, int parent, const char* cls,
                     const char* fieldName, const char* label) {
    char id[ID_BUF];
    MakeId(id, cls, fieldName, "row");
    int row = ctx.CreatePanel(id);
    if (row < 0) return -1;
    ctx.SetParent(row, parent);
    ctx.SetLayout(row, LayoutDirection::Row, Alignment::Start,
                  Alignment::Center, SPACING);
    ctx.SetSizeMode(row, SizeMode::FillRemaining, SizeMode::Fixed);
    ctx.SetSize(row, 0, ROW_H);

    MakeId(id, cls, fieldName, "lbl");
    int lbl = ctx.CreateLabel(id, label);
    if (lbl >= 0) {
        ctx.SetParent(lbl, row);
        ctx.SetSize(lbl, LABEL_W, ROW_H);
    }
    return row;
}

/// Write a float value as text into a buffer.
static void FloatToStr(char* buf, size_t sz, float v) {
    snprintf(buf, sz, "%.4g", static_cast<double>(v));
}

/// Write an int value as text into a buffer.
static void IntToStr(char* buf, size_t sz, int v) {
    snprintf(buf, sz, "%d", v);
}

// -- per-field widget generators ----------------------------------------

static int CreateSliderField(UIContext& ctx, int row, const char* cls,
                             const char* name, void* instance,
                             const FieldDecl& f) {
    char id[ID_BUF];
    MakeId(id, cls, name, "sld");
    float min = static_cast<float>(f.min_value);
    float max = static_cast<float>(f.max_value);

    float val = 0.0f;
    const void* ptr = f.access.get_cptr(instance);
    switch (f.kind) {
        case FieldKind::Float:  val = *static_cast<const float*>(ptr); break;
        case FieldKind::Double: val = static_cast<float>(*static_cast<const double*>(ptr)); break;
        case FieldKind::Int:    val = static_cast<float>(*static_cast<const int*>(ptr)); break;
        default: break;
    }

    int sld = ctx.CreateSlider(id, min, max, val);
    if (sld < 0) return -1;
    ctx.SetParent(sld, row);
    ctx.SetSizeMode(sld, SizeMode::FillRemaining, SizeMode::Fixed);
    ctx.SetSize(sld, 0, ROW_H);

    // Two-way binding callback
    FieldAccess acc = f.access;
    FieldKind kind = f.kind;
    ctx.SetOnChange(sld, [acc, kind, instance](Widget& w) {
        void* dst = acc.get_ptr(instance);
        switch (kind) {
            case FieldKind::Float:
                *static_cast<float*>(dst) = w.sliderValue;
                break;
            case FieldKind::Double:
                *static_cast<double*>(dst) = static_cast<double>(w.sliderValue);
                break;
            case FieldKind::Int:
                *static_cast<int*>(dst) = static_cast<int>(std::round(w.sliderValue));
                break;
            default: break;
        }
    });
    return sld;
}

static int CreateCheckboxField(UIContext& ctx, int row, const char* cls,
                               const char* name, void* instance,
                               const FieldDecl& f) {
    char id[ID_BUF];
    MakeId(id, cls, name, "chk");

    bool val = *static_cast<const bool*>(f.access.get_cptr(instance));
    int chk = ctx.CreateCheckbox(id, val);
    if (chk < 0) return -1;
    ctx.SetParent(chk, row);
    ctx.SetSizeMode(chk, SizeMode::FillRemaining, SizeMode::Fixed);
    ctx.SetSize(chk, 0, ROW_H);

    FieldAccess acc = f.access;
    ctx.SetOnChange(chk, [acc, instance](Widget& w) {
        *static_cast<bool*>(acc.get_ptr(instance)) = w.checked;
    });
    return chk;
}

static int CreateValueLabel(UIContext& ctx, int row, const char* cls,
                            const char* name, void* instance,
                            const FieldDecl& f) {
    char id[ID_BUF];
    char valBuf[64];
    MakeId(id, cls, name, "val");

    const void* ptr = f.access.get_cptr(instance);
    switch (f.kind) {
        case FieldKind::Float:
            FloatToStr(valBuf, sizeof(valBuf), *static_cast<const float*>(ptr));
            break;
        case FieldKind::Double:
            FloatToStr(valBuf, sizeof(valBuf),
                       static_cast<float>(*static_cast<const double*>(ptr)));
            break;
        case FieldKind::Int:
            IntToStr(valBuf, sizeof(valBuf), *static_cast<const int*>(ptr));
            break;
        case FieldKind::UInt8:
            snprintf(valBuf, sizeof(valBuf), "%u",
                     static_cast<unsigned>(*static_cast<const uint8_t*>(ptr)));
            break;
        case FieldKind::UInt16:
            snprintf(valBuf, sizeof(valBuf), "%u",
                     static_cast<unsigned>(*static_cast<const uint16_t*>(ptr)));
            break;
        case FieldKind::UInt32:
            snprintf(valBuf, sizeof(valBuf), "%u",
                     *static_cast<const uint32_t*>(ptr));
            break;
        case FieldKind::SizeT:
            snprintf(valBuf, sizeof(valBuf), "%zu",
                     *static_cast<const size_t*>(ptr));
            break;
        default:
            snprintf(valBuf, sizeof(valBuf), "(complex)");
            break;
    }

    int lbl = ctx.CreateLabel(id, valBuf);
    if (lbl < 0) return -1;
    ctx.SetParent(lbl, row);
    ctx.SetSizeMode(lbl, SizeMode::FillRemaining, SizeMode::Fixed);
    ctx.SetSize(lbl, 0, ROW_H);
    return lbl;
}

static int CreateColorField(UIContext& ctx, int parent, const char* cls,
                            const char* name, void* instance,
                            const FieldDecl& f) {
    // Create three sliders for R, G, B (uint8 fields)
    char id[ID_BUF];
    MakeId(id, cls, name, "cpanel");
    int panel = ctx.CreatePanel(id);
    if (panel < 0) return -1;
    ctx.SetParent(panel, parent);
    ctx.SetLayout(panel, LayoutDirection::Column, Alignment::Start,
                  Alignment::Stretch, 2.0f);
    ctx.SetSizeMode(panel, SizeMode::FillRemaining, SizeMode::FitContent);

    const char* channels[] = {"R", "G", "B"};
    for (int c = 0; c < 3; ++c) {
        char chId[ID_BUF];
        snprintf(chId, ID_BUF, "insp_%s_%s_%s", cls, name, channels[c]);
        int sld = ctx.CreateSlider(chId, 0.0f, 255.0f,
            static_cast<float>(static_cast<const uint8_t*>(
                f.access.get_cptr(instance))[c]));
        if (sld < 0) continue;
        ctx.SetParent(sld, panel);
        ctx.SetSizeMode(sld, SizeMode::FillRemaining, SizeMode::Fixed);
        ctx.SetSize(sld, 0, ROW_H);

        FieldAccess acc = f.access;
        int channel = c;
        ctx.SetOnChange(sld, [acc, instance, channel](Widget& w) {
            uint8_t* dst = static_cast<uint8_t*>(acc.get_ptr(instance));
            dst[channel] = static_cast<uint8_t>(std::round(w.sliderValue));
        });
    }
    return panel;
}

// -- main generator -----------------------------------------------------

InspectorResult GenerateInspector(const ClassDesc* desc, void* instance,
                                  UIContext& ctx, int parentIdx) {
    InspectorResult result;
    if (!desc || !instance) return result;

    // Create root panel
    char rootId[ID_BUF];
    snprintf(rootId, ID_BUF, "insp_%s_root", desc->name);
    int root = ctx.CreatePanel(rootId);
    if (root < 0) return result;

    ctx.SetLayout(root, LayoutDirection::Column, Alignment::Start,
                  Alignment::Stretch, SPACING);
    ctx.SetSizeMode(root, SizeMode::FillRemaining, SizeMode::FitContent);
    ctx.SetPadding(root, CAT_PAD, CAT_PAD, CAT_PAD, CAT_PAD);

    int target = (parentIdx >= 0) ? parentIdx : ctx.Root();
    if (target >= 0) ctx.SetParent(root, target);

    result.rootWidget = root;

    // Title label
    char titleId[ID_BUF];
    snprintf(titleId, ID_BUF, "insp_%s_title", desc->name);
    int title = ctx.CreateLabel(titleId, desc->name);
    if (title >= 0) {
        ctx.SetParent(title, root);
        ctx.SetSizeMode(title, SizeMode::FillRemaining, SizeMode::Fixed);
        ctx.SetSize(title, 0, ROW_H + 4.0f);
    }

    // Separator
    char sepId[ID_BUF];
    snprintf(sepId, ID_BUF, "insp_%s_sep", desc->name);
    int sep = ctx.CreateSeparator(sepId);
    if (sep >= 0) ctx.SetParent(sep, root);

    // Track current category for grouping
    const char* currentCat = nullptr;
    int catPanel = root;

    const FieldDecl* fields = desc->fields.data;
    const size_t count = desc->fields.count;

    for (size_t i = 0; i < count; ++i) {
        const FieldDecl& f = fields[i];

        // Skip hidden fields
        if (f.hint == FieldHint::Hidden) continue;

        // Category grouping - create a new sub-panel when category changes
        bool newCat = false;
        if (f.category) {
            if (!currentCat || strcmp(currentCat, f.category) != 0) {
                newCat = true;
                currentCat = f.category;
            }
        } else if (currentCat) {
            newCat = true;
            currentCat = nullptr;
        }

        if (newCat) {
            char catId[ID_BUF];
            const char* catLabel = currentCat ? currentCat : "General";
            snprintf(catId, ID_BUF, "insp_%s_cat_%s", desc->name, catLabel);

            catPanel = ctx.CreatePanel(catId);
            if (catPanel < 0) { catPanel = root; continue; }
            ctx.SetParent(catPanel, root);
            ctx.SetLayout(catPanel, LayoutDirection::Column, Alignment::Start,
                          Alignment::Stretch, SPACING);
            ctx.SetSizeMode(catPanel, SizeMode::FillRemaining, SizeMode::FitContent);
            ctx.SetPadding(catPanel, 2.0f, 0.0f, 2.0f, 8.0f);

            // Category label
            char catLblId[ID_BUF];
            snprintf(catLblId, ID_BUF, "insp_%s_catlbl_%s", desc->name, catLabel);
            int catLbl = ctx.CreateLabel(catLblId, catLabel);
            if (catLbl >= 0) {
                ctx.SetParent(catLbl, catPanel);
                ctx.SetSizeMode(catLbl, SizeMode::FillRemaining, SizeMode::Fixed);
                ctx.SetSize(catLbl, 0, ROW_H);
            }
        }

        // Determine widget type from kind + hint
        const char* cls = desc->name;
        const char* name = f.name;
        const char* label = f.description ? f.description : f.name;

        bool hasRange = (f.min_value != 0.0 || f.max_value != 0.0) &&
                        f.min_value < f.max_value;
        bool isReadOnly = (f.hint == FieldHint::ReadOnly);
        int widgetIdx = -1;

        switch (f.hint) {
            case FieldHint::ColorPicker: {
                int row = CreateRow(ctx, catPanel, cls, name, label);
                if (row >= 0) widgetIdx = CreateColorField(ctx, row, cls, name, instance, f);
                break;
            }
            case FieldHint::Slider:
            case FieldHint::Angle:
            case FieldHint::Normalized: {
                FieldDecl adjusted = f;
                if (f.hint == FieldHint::Angle) {
                    adjusted.min_value = 0.0;
                    adjusted.max_value = 360.0;
                } else if (f.hint == FieldHint::Normalized) {
                    adjusted.min_value = 0.0;
                    adjusted.max_value = 1.0;
                }
                int row = CreateRow(ctx, catPanel, cls, name, label);
                if (row >= 0) widgetIdx = CreateSliderField(ctx, row, cls, name, instance, adjusted);
                break;
            }
            default:
                // Use FieldKind to pick default widget
                switch (f.kind) {
                    case FieldKind::Bool: {
                        int row = CreateRow(ctx, catPanel, cls, name, label);
                        if (row >= 0) widgetIdx = CreateCheckboxField(ctx, row, cls, name, instance, f);
                        break;
                    }
                    case FieldKind::Float:
                    case FieldKind::Double:
                    case FieldKind::Int: {
                        int row = CreateRow(ctx, catPanel, cls, name, label);
                        if (row < 0) break;
                        if (hasRange && !isReadOnly) {
                            widgetIdx = CreateSliderField(ctx, row, cls, name, instance, f);
                        } else {
                            widgetIdx = CreateValueLabel(ctx, row, cls, name, instance, f);
                        }
                        break;
                    }
                    case FieldKind::Complex: {
                        // Nested object - try recursive inspection
#if KL_HAS_RTTI
                        if (f.type) {
                            const ClassDesc* nested = ClassForType(*f.type);
                            if (nested) {
                                // Create a foldout for the sub-object
                                char foldId[ID_BUF];
                                MakeId(foldId, cls, name, "fold");
                                int fold = ctx.CreateWidget(WidgetTag::TreeNode, foldId);
                                if (fold >= 0) {
                                    ctx.SetParent(fold, catPanel);
                                    ctx.SetText(fold, label);
                                    ctx.SetSizeMode(fold, SizeMode::FillRemaining,
                                                    SizeMode::FitContent);
                                    void* subObj = f.access.get_ptr(instance);
                                    InspectorResult sub = GenerateInspector(
                                        nested, subObj, ctx, fold);
                                    result.fieldCount += sub.fieldCount;
                                    widgetIdx = fold;
                                }
                                break;
                            }
                        }
#endif
                        // Fallback: show as "(complex)" label
                        int row = CreateRow(ctx, catPanel, cls, name, label);
                        if (row >= 0) widgetIdx = CreateValueLabel(ctx, row, cls, name, instance, f);
                        break;
                    }
                    default: {
                        // UInt8/16/32/SizeT - display as label
                        int row = CreateRow(ctx, catPanel, cls, name, label);
                        if (row >= 0) widgetIdx = CreateValueLabel(ctx, row, cls, name, instance, f);
                        break;
                    }
                }
                break;
        }

        if (widgetIdx >= 0) {
            if (isReadOnly) ctx.SetEnabled(widgetIdx, false);
            result.fieldCount++;
        }
    }

    return result;
}

// -- refresh ------------------------------------------------------------

void RefreshInspector(const ClassDesc* desc, void* instance,
                      UIContext& ctx, int inspectorRoot) {
    if (!desc || !instance || inspectorRoot < 0) return;

    const FieldDecl* fields = desc->fields.data;
    const size_t count = desc->fields.count;
    const char* cls = desc->name;

    for (size_t i = 0; i < count; ++i) {
        const FieldDecl& f = fields[i];
        if (f.hint == FieldHint::Hidden) continue;

        const char* name = f.name;
        const void* ptr = f.access.get_cptr(instance);
        char id[ID_BUF];

        switch (f.kind) {
            case FieldKind::Float:
            case FieldKind::Double:
            case FieldKind::Int: {
                bool hasRange = (f.min_value != 0.0 || f.max_value != 0.0) &&
                                f.min_value < f.max_value;
                bool useSlider = hasRange ||
                                 f.hint == FieldHint::Slider ||
                                 f.hint == FieldHint::Angle ||
                                 f.hint == FieldHint::Normalized;

                if (useSlider) {
                    MakeId(id, cls, name, "sld");
                    int idx = ctx.FindWidget(id);
                    if (idx >= 0) {
                        Widget* w = ctx.GetWidget(idx);
                        if (w) {
                            float v = 0;
                            if (f.kind == FieldKind::Float)
                                v = *static_cast<const float*>(ptr);
                            else if (f.kind == FieldKind::Double)
                                v = static_cast<float>(*static_cast<const double*>(ptr));
                            else
                                v = static_cast<float>(*static_cast<const int*>(ptr));
                            w->sliderValue = v;
                        }
                    }
                } else {
                    MakeId(id, cls, name, "val");
                    int idx = ctx.FindWidget(id);
                    if (idx >= 0) {
                        char buf[64];
                        if (f.kind == FieldKind::Float)
                            FloatToStr(buf, sizeof(buf), *static_cast<const float*>(ptr));
                        else if (f.kind == FieldKind::Double)
                            FloatToStr(buf, sizeof(buf),
                                       static_cast<float>(*static_cast<const double*>(ptr)));
                        else
                            IntToStr(buf, sizeof(buf), *static_cast<const int*>(ptr));
                        ctx.SetText(idx, buf);
                    }
                }
                break;
            }
            case FieldKind::Bool: {
                MakeId(id, cls, name, "chk");
                int idx = ctx.FindWidget(id);
                if (idx >= 0) {
                    Widget* w = ctx.GetWidget(idx);
                    if (w) w->checked = *static_cast<const bool*>(ptr);
                }
                break;
            }
            default: {
                // UInt variants, SizeT - update display labels
                MakeId(id, cls, name, "val");
                int idx = ctx.FindWidget(id);
                if (idx >= 0) {
                    char buf[64];
                    switch (f.kind) {
                        case FieldKind::UInt8:
                            snprintf(buf, sizeof(buf), "%u",
                                     static_cast<unsigned>(*static_cast<const uint8_t*>(ptr)));
                            break;
                        case FieldKind::UInt16:
                            snprintf(buf, sizeof(buf), "%u",
                                     static_cast<unsigned>(*static_cast<const uint16_t*>(ptr)));
                            break;
                        case FieldKind::UInt32:
                            snprintf(buf, sizeof(buf), "%u",
                                     *static_cast<const uint32_t*>(ptr));
                            break;
                        case FieldKind::SizeT:
                            snprintf(buf, sizeof(buf), "%zu",
                                     *static_cast<const size_t*>(ptr));
                            break;
                        default: continue;
                    }
                    ctx.SetText(idx, buf);
                }
                break;
            }
        }
    }
}

} // namespace ui
} // namespace koilo
