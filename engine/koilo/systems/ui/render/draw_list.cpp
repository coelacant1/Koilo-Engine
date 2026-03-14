// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file draw_list.cpp
 * @brief Implementation of UIDrawList – platform-independent UI draw command list.
 *
 * @date 03/08/2026
 * @author Coela Can't
 */

#include <koilo/systems/ui/render/draw_list.hpp>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>
#include <string>

namespace koilo {
namespace ui {

// -- Helpers ---------------------------------------------------------

/// Compute effective padding: widget.padding (KSS) takes priority,
/// falls back to style.padding (theme) per edge.
static Edges EffectivePadding(const Widget& w, const Style& s) {
    bool hasWidgetPad = (w.padding.top != 0.0f || w.padding.right != 0.0f ||
                         w.padding.bottom != 0.0f || w.padding.left != 0.0f);
    return hasWidgetPad ? w.padding : s.padding;
}

// -- Lifetime --------------------------------------------------------

// Clear all draw commands and reset scissor stack
void UIDrawList::Clear() {
    commands_.clear();
    commands_.reserve(4096);
    scissorStack_.clear();
}

// -- Primitive commands ----------------------------------------------

// Append a solid filled rectangle
void UIDrawList::AddSolidRect(float x, float y, float w, float h,
                              Color4 color) {
    if (color.a == 0 || w <= 0.0f || h <= 0.0f) return;
    DrawCmd cmd;
    cmd.type = DrawCmdType::SolidRect;
    cmd.x = x; cmd.y = y; cmd.w = w; cmd.h = h;
    cmd.color = color;
    commands_.push_back(cmd);
}

// Append a rectangular border outline
void UIDrawList::AddBorderRect(float x, float y, float w, float h,
                               Color4 color, float borderWidth) {
    if (color.a == 0 || w <= 0.0f || h <= 0.0f) return;
    DrawCmd cmd;
    cmd.type = DrawCmdType::BorderRect;
    cmd.x = x; cmd.y = y; cmd.w = w; cmd.h = h;
    cmd.color = color;
    cmd.borderWidth = borderWidth;
    commands_.push_back(cmd);
}

// Append a textured quad (glyph atlas, images)
void UIDrawList::AddTexturedRect(float x, float y, float w, float h,
                                 float u0, float v0, float u1, float v1,
                                 Color4 color, uint32_t textureHandle) {
    if (color.a == 0 || w <= 0.0f || h <= 0.0f) return;
    DrawCmd cmd;
    cmd.type = DrawCmdType::TexturedRect;
    cmd.x = x; cmd.y = y; cmd.w = w; cmd.h = h;
    cmd.u0 = u0; cmd.v0 = v0; cmd.u1 = u1; cmd.v1 = v1;
    cmd.color = color;
    cmd.textureHandle = textureHandle;
    commands_.push_back(cmd);
}

// Append a rounded rectangle with uniform corner radius
void UIDrawList::AddRoundedRect(float x, float y, float w, float h,
                                float radius, Color4 color) {
    AddRoundedRect(x, y, w, h, radius, radius, radius, radius, color);
}

// Append a rounded rectangle with per-corner radii
void UIDrawList::AddRoundedRect(float x, float y, float w, float h,
                                float rTL, float rTR, float rBR, float rBL,
                                Color4 color) {
    if (color.a == 0 || w <= 0.0f || h <= 0.0f) return;
    float maxR = std::min(w, h) * 0.5f;
    rTL = std::min(rTL, maxR); rTR = std::min(rTR, maxR);
    rBR = std::min(rBR, maxR); rBL = std::min(rBL, maxR);
    DrawCmd cmd;
    cmd.type = DrawCmdType::RoundedRect;
    cmd.x = x; cmd.y = y; cmd.w = w; cmd.h = h;
    cmd.color = color;
    cmd.cornerRadius[0] = rTL; cmd.cornerRadius[1] = rTR;
    cmd.cornerRadius[2] = rBR; cmd.cornerRadius[3] = rBL;
    commands_.push_back(cmd);
}

// Append a rounded border rectangle with uniform radius
void UIDrawList::AddRoundedBorderRect(float x, float y, float w, float h,
                                      float radius, Color4 color,
                                      float borderWidth) {
    AddRoundedBorderRect(x, y, w, h, radius, radius, radius, radius, color, borderWidth);
}

// Append a rounded border rectangle with per-corner radii
void UIDrawList::AddRoundedBorderRect(float x, float y, float w, float h,
                                      float rTL, float rTR, float rBR, float rBL,
                                      Color4 color, float borderWidth) {
    if (color.a == 0 || w <= 0.0f || h <= 0.0f) return;
    float maxR = std::min(w, h) * 0.5f;
    rTL = std::min(rTL, maxR); rTR = std::min(rTR, maxR);
    rBR = std::min(rBR, maxR); rBL = std::min(rBL, maxR);
    DrawCmd cmd;
    cmd.type = DrawCmdType::RoundedBorderRect;
    cmd.x = x; cmd.y = y; cmd.w = w; cmd.h = h;
    cmd.color = color;
    cmd.cornerRadius[0] = rTL; cmd.cornerRadius[1] = rTR;
    cmd.cornerRadius[2] = rBR; cmd.cornerRadius[3] = rBL;
    cmd.borderWidth = borderWidth;
    commands_.push_back(cmd);
}

// Append a line segment with width
void UIDrawList::AddLine(float x0, float y0, float x1, float y1,
                         float width, Color4 color) {
    if (color.a == 0 || width <= 0.0f) return;
    DrawCmd cmd;
    cmd.type = DrawCmdType::Line;
    cmd.x = x0; cmd.y = y0;
    cmd.w = x1; cmd.h = y1; // reuse w,h as endpoint
    cmd.borderWidth = width;
    cmd.color = color;
    commands_.push_back(cmd);
}

// Append a filled circle (radius stored in w)
void UIDrawList::AddFilledCircle(float cx, float cy, float radius, Color4 color) {
    if (color.a == 0 || radius <= 0.0f) return;
    DrawCmd cmd;
    cmd.type = DrawCmdType::FilledCircle;
    cmd.x = cx; cmd.y = cy;
    cmd.w = radius; // radius stored in w
    cmd.color = color;
    commands_.push_back(cmd);
}

// Append a circle outline
void UIDrawList::AddCircleOutline(float cx, float cy, float radius,
                                  float lineWidth, Color4 color) {
    if (color.a == 0 || radius <= 0.0f || lineWidth <= 0.0f) return;
    DrawCmd cmd;
    cmd.type = DrawCmdType::CircleOutline;
    cmd.x = cx; cmd.y = cy;
    cmd.w = radius;
    cmd.borderWidth = lineWidth;
    cmd.color = color;
    commands_.push_back(cmd);
}

// Append a filled triangle
void UIDrawList::AddTriangle(float x0, float y0, float x1, float y1,
                             float ax2, float ay2, Color4 color) {
    if (color.a == 0) return;
    DrawCmd cmd;
    cmd.type = DrawCmdType::Triangle;
    cmd.x = x0; cmd.y = y0;
    cmd.w = x1; cmd.h = y1;
    cmd.x2 = ax2; cmd.y2 = ay2;
    cmd.color = color;
    commands_.push_back(cmd);
}

// Append an icon quad from the icon atlas
// Push an integer scissor clip rect onto the stack
void UIDrawList::PushScissor(int x, int y, int w, int h) {
    DrawCmd cmd;
    cmd.type = DrawCmdType::PushScissor;
    cmd.scissorX = x; cmd.scissorY = y;
    cmd.scissorW = w; cmd.scissorH = h;
    commands_.push_back(cmd);
    scissorStack_.push_back({x, y, w, h});
}

// Push a float scissor clip rect (floor/ceil converted)
void UIDrawList::PushScissor(float x, float y, float w, float h) {
    // Use floor for position and ceil for size to avoid clipping pixels
    int ix = static_cast<int>(std::floor(x));
    int iy = static_cast<int>(std::floor(y));
    int iw = static_cast<int>(std::ceil(x + w)) - ix;
    int ih = static_cast<int>(std::ceil(y + h)) - iy;
    PushScissor(ix, iy, iw, ih);
}

// Pop the most recent scissor rect
void UIDrawList::PopScissor() {
    DrawCmd cmd;
    cmd.type = DrawCmdType::PopScissor;
    commands_.push_back(cmd);
    if (!scissorStack_.empty()) scissorStack_.pop_back();
}

// ============================================================================
// Widget tree → draw commands
// ============================================================================

// Build draw commands for the entire UI tree
void UIDrawList::BuildFromContext(UIContext& ctx, font::Font* font,
                                  uint32_t fontAtlasTexture) {
    Clear();
    deferredDropdowns_.clear();
    font_ = font;
    fontAtlasTexture_ = fontAtlasTexture;

    int rootIdx = ctx.Root();
    if (rootIdx < 0) return;

    const auto& pool = ctx.Pool();
    const auto& strings = ctx.Strings();
    const auto& theme = ctx.GetTheme();
    const Widget* root = pool.Get(rootIdx);
    if (!root) return;

    EmitWidget(*root, rootIdx, pool, strings, theme);

    // Render deferred dropdown popups (on top of main tree, below floating panels)
    for (auto& dd : deferredDropdowns_) {
        EmitDropdownPopup(*dd.widget, dd.rect, dd.style, *dd.strings, *dd.pool);
    }

    // Render minimized panel tray (above status bar)
    EmitMinimizedPanelTray(pool, strings,
                           ctx.ViewportWidth(), ctx.ViewportHeight());

    // Render tooltip overlay if active
    int tipIdx = ctx.GetTooltipWidget();
    if (tipIdx >= 0) {
        const Widget* tipW = pool.Get(tipIdx);
        if (tipW && tipW->tooltipId != NullStringId) {
            const char* tipText = strings.Lookup(tipW->tooltipId);
            if (tipText && tipText[0]) {
                EmitTooltip(tipText, ctx.GetTooltipX(), ctx.GetTooltipY(),
                            ctx.ViewportWidth(), ctx.ViewportHeight());
            }
        }
    }

    // Render drag-and-drop overlays
    if (ctx.IsDragging()) {
        EmitDragOverlay(ctx, strings);
    }
}

// ============================================================================
// Private widget emitters
// ============================================================================

// Recursively emit a widget and its children
void UIDrawList::EmitWidget(const Widget& widget, int widgetIdx, const WidgetPool& pool,
                            const StringTable& strings, const Theme& theme) {
    if (!widget.flags.visible) return;
    if (widget.visibility == Visibility::Hidden) return; // hidden: skip render, keep layout

    const Rect& r = widget.computedRect;
    Style style = theme.ResolveAnimated(widget, widgetIdx);

    // Apply opacity to all colors
    if (style.opacity < 1.0f && style.opacity >= 0.0f) {
        style.background.a = static_cast<uint8_t>(style.background.a * style.opacity);
        style.textColor.a  = static_cast<uint8_t>(style.textColor.a * style.opacity);
        style.border.color.a = static_cast<uint8_t>(style.border.color.a * style.opacity);
    }

    // Box shadow (rendered behind background)
    if (style.shadow.active && style.shadow.blur > 0.0f) {
        float blur = style.shadow.blur;
        float spread = style.shadow.spread;
        float sx = r.x + style.shadow.offsetX - spread;
        float sy = r.y + style.shadow.offsetY - spread;
        float sw = r.w + spread * 2.0f;
        float sh = r.h + spread * 2.0f;
        // Approximate blur with concentric rects of decreasing alpha
        int steps = std::max(1, static_cast<int>(blur / 2.0f));
        // Divide base alpha by step count to prevent additive darkening
        float baseAlpha = static_cast<float>(style.shadow.color.a) / static_cast<float>(steps + 1);
        for (int s = steps; s >= 0; --s) {
            float t = static_cast<float>(s) / static_cast<float>(steps);
            float expand = blur * t;
            uint8_t a = static_cast<uint8_t>(baseAlpha * (1.0f - t) * (1.0f - t));
            if (a == 0) continue;
            Color4 sc{style.shadow.color.r, style.shadow.color.g, style.shadow.color.b, a};
            if (style.border.HasRadius()) {
                AddRoundedRect(sx - expand, sy - expand, sw + expand * 2.0f, sh + expand * 2.0f,
                               style.border.radiusTL + expand, style.border.radiusTR + expand,
                               style.border.radiusBR + expand, style.border.radiusBL + expand, sc);
            } else {
                AddSolidRect(sx - expand, sy - expand, sw + expand * 2.0f, sh + expand * 2.0f, sc);
            }
        }
    }

    // Background (with gradient support)
    // FloatingPanel draws its own chrome in EmitFloatingPanel — skip generic bg/border
    if (widget.tag != WidgetTag::FloatingPanel) {
    if (style.gradient.active) {
        // Per-vertex color gradient - emit as two triangles with different vertex colors
        // For now, approximate with top and bottom halves
        Color4 top = style.gradient.from;
        Color4 bot = style.gradient.to;
        if (style.opacity < 1.0f) {
            top.a = static_cast<uint8_t>(top.a * style.opacity);
            bot.a = static_cast<uint8_t>(bot.a * style.opacity);
        }
        // Simple two-band gradient approximation
        float midY = r.h * 0.5f;
        Color4 mid{
            static_cast<uint8_t>((top.r + bot.r) / 2),
            static_cast<uint8_t>((top.g + bot.g) / 2),
            static_cast<uint8_t>((top.b + bot.b) / 2),
            static_cast<uint8_t>((top.a + bot.a) / 2)
        };
        AddSolidRect(r.x, r.y, r.w, midY, top);
        AddSolidRect(r.x, r.y + midY, r.w, r.h - midY, bot);
    } else if (style.background.a > 0) {
        if (style.border.HasRadius()) {
            AddRoundedRect(r.x, r.y, r.w, r.h,
                           style.border.radiusTL, style.border.radiusTR,
                           style.border.radiusBR, style.border.radiusBL, style.background);
        } else {
            AddSolidRect(r.x, r.y, r.w, r.h, style.background);
        }
    }

    // Border
    if (style.border.color.a > 0 && style.border.width > 0.0f) {
        if (style.border.HasRadius()) {
            AddRoundedBorderRect(r.x, r.y, r.w, r.h,
                                 style.border.radiusTL, style.border.radiusTR,
                                 style.border.radiusBR, style.border.radiusBL,
                                 style.border.color, style.border.width);
        } else {
            AddBorderRect(r.x, r.y, r.w, r.h,
                          style.border.color, style.border.width);
        }
    }
    } // end FloatingPanel skip

    // Content-specific rendering
    switch (widget.tag) {
    case WidgetTag::Label:
    case WidgetTag::MenuItem:
        EmitMenuItem(widget, r, style, strings);
        break;
    case WidgetTag::TreeNode:
        EmitTreeNode(widget, r, style, strings, pool, widgetIdx);
        break;
    case WidgetTag::Button:
        EmitTextCentered(widget, r, style, strings);
        break;
    case WidgetTag::TextField:
        EmitTextField(widget, r, style, strings);
        break;
    case WidgetTag::Checkbox:
        EmitCheckbox(widget, r, style);
        break;
    case WidgetTag::Slider:
        EmitSlider(widget, r, style);
        break;
    case WidgetTag::Separator:
        EmitSeparator(widget, r, style);
        break;
    case WidgetTag::ScrollView:
        EmitScrollbar(widget, r, style);
        break;
    case WidgetTag::Dropdown:
        EmitDropdown(widget, r, style, strings, pool, widgetIdx);
        break;
    case WidgetTag::Image:
        EmitImage(widget, r);
        break;
    case WidgetTag::ColorField:
        EmitColorField(widget, r, style);
        break;
    case WidgetTag::ProgressBar:
        EmitProgressBar(widget, r, style);
        break;
    case WidgetTag::ToggleSwitch:
        EmitToggleSwitch(widget, r, style);
        break;
    case WidgetTag::RadioButton:
        EmitRadioButton(widget, r, style);
        break;
    case WidgetTag::NumberSpinner:
        EmitNumberSpinner(widget, r, style, strings);
        break;
    case WidgetTag::FloatingPanel:
        EmitFloatingPanel(widget, r, style, strings);
        break;
    case WidgetTag::VirtualList:
        EmitScrollbar(widget, r, style);
        break;
    case WidgetTag::Canvas2D:
        if (widget.onPaint) {
            CanvasDrawContext cdc(*this, r.x, r.y, r.w, r.h, font_);
            widget.onPaint(&cdc);
        }
        break;
    default:
        break;
    }

    // Emit scrollbar for any scrollable element (not just ScrollView)
    if (widget.flags.scrollable && widget.tag != WidgetTag::ScrollView) {
        EmitScrollbar(widget, r, style);
    }

    // Clip children if flagged
    bool clipping = widget.flags.clipChildren;
    if (clipping) {
        PushScissor(r.x, r.y, r.w, r.h);
    }

    // Lambda: emit a child, skipping if entirely outside parent clip rect
    auto emitChild = [&](int childIdx) {
        if (childIdx < 0 || !pool.IsAlive(childIdx)) return;
        const Widget* child = pool.Get(childIdx);
        if (!child) return;
        // Cull children fully outside parent bounds when clipping is active
        if (clipping) {
            const Rect& cr = child->computedRect;
            if (cr.y + cr.h < r.y || cr.y > r.y + r.h ||
                cr.x + cr.w < r.x || cr.x > r.x + r.w) return;
        }
        EmitWidget(*child, childIdx, pool, strings, theme);
    };

    // Recurse children sorted by z-order (back to front)
    if (widget.childCount > 1) {
        int16_t sorted[MAX_CHILDREN];
        for (int16_t i = 0; i < widget.childCount; ++i)
            sorted[i] = widget.children[i];
        // Simple insertion sort by zOrder (usually nearly sorted, small N)
        for (int16_t i = 1; i < widget.childCount; ++i) {
            int16_t key = sorted[i];
            const Widget* kw = pool.Get(key);
            int16_t kz = kw ? kw->zOrder : 0;
            int16_t j = i - 1;
            while (j >= 0) {
                const Widget* jw = pool.Get(sorted[j]);
                int16_t jz = jw ? jw->zOrder : 0;
                if (jz <= kz) break;
                sorted[j + 1] = sorted[j];
                --j;
            }
            sorted[j + 1] = key;
        }
        for (int16_t i = 0; i < widget.childCount; ++i)
            emitChild(sorted[i]);
    } else {
        for (int16_t i = 0; i < widget.childCount; ++i)
            emitChild(widget.children[i]);
    }

    // Draw divider handles between children of resizable containers
    if (widget.flags.resizable) {
        EmitDividers(widget, pool);
    }

    if (clipping) {
        PopScissor();
    }
}

// Emit single-line or word-wrapped text within a widget
void UIDrawList::EmitText(const Widget& widget, const Rect& r,
                          const Style& style, const StringTable& strings) {
    if (!font_ || widget.textId == NullStringId) return;
    const char* text = strings.Lookup(widget.textId);
    if (!text || text[0] == '\0') return;

    float fontSize = style.fontSize > 0.0f ? style.fontSize : 14.0f;
    if (widget.fontSize > 0.0f) fontSize = widget.fontSize;
    float fontScale = fontSize / font_->PixelSize();

    // Content area (inset by effective padding: widget KSS padding or theme fallback)
    Edges pad = EffectivePadding(widget, style);
    float cx = r.x + pad.left;
    float cy = r.y + pad.top;
    float cw = r.w - pad.left - pad.right;
    float ch = r.h - pad.top - pad.bottom;
    float lh = widget.lineHeight > 0.0f ? widget.lineHeight : fontSize * 1.2f;
    float lineH = (font_->Ascent() - font_->Descent()) * fontScale;

    // Clip text to content area
    PushScissor(cx, cy, cw, ch);

    // Word-wrap: break text into lines at word boundaries
    if (widget.wordWrap && cw > 0) {
        float penY = cy + (ch - lineH) * 0.5f + font_->Ascent() * fontScale;
        size_t len = std::strlen(text);
        size_t lineStart = 0;

        while (lineStart < len && penY - font_->Ascent() * fontScale < cy + ch) {
            // Find how many characters fit on this line
            size_t pos = lineStart;
            size_t lastBreak = lineStart; // last word boundary that fits
            float penX = 0.0f;
            bool foundBreak = false;

            while (pos < len) {
                if (text[pos] == '\n') {
                    lastBreak = pos;
                    foundBreak = true;
                    break;
                }
                // Track word boundaries (spaces)
                if (pos > lineStart && (text[pos] == ' ' || text[pos] == '\t')) {
                    lastBreak = pos;
                    foundBreak = true;
                }
                size_t prevPos = pos;
                uint32_t cp = DecodeUTF8(text, len, pos);
                const font::GlyphRegion* glyph = font_->GetGlyph(cp);
                float advance = glyph ? glyph->advance * fontScale
                                      : fontSize * 0.5f;
                if (penX + advance > cw && prevPos > lineStart) {
                    // Exceeded line width - break at last word boundary
                    if (!foundBreak) lastBreak = prevPos; // no space found, break mid-word
                    pos = lastBreak;
                    break;
                }
                penX += advance + widget.letterSpacing;
            }

            // If we reached end of text, set lastBreak to end
            if (pos >= len) lastBreak = len;
            size_t lineEnd = (lastBreak > lineStart) ? lastBreak : pos;

            // Render this line
            std::string line(text + lineStart, lineEnd - lineStart);
            float textX = cx;
            if (widget.textAlign == TextAlign::Center) {
                auto lm = font_->MeasureText(line.c_str());
                textX = cx + (cw - lm.width * fontScale) * 0.5f;
            } else if (widget.textAlign == TextAlign::Right) {
                auto lm = font_->MeasureText(line.c_str());
                textX = cx + cw - lm.width * fontScale;
            }
            EmitTextGlyphs(line.c_str(), textX, penY, fontSize, style.textColor);

            penY += lh;
            // Skip the break character (space or newline)
            lineStart = lineEnd;
            if (lineStart < len && (text[lineStart] == ' ' || text[lineStart] == '\n' || text[lineStart] == '\t'))
                lineStart++;
        }
        PopScissor();
        return;
    }

    // Single-line path (no word-wrap) - vertically center text
    float textY = cy + (ch - lineH) * 0.5f + font_->Ascent() * fontScale;

    // Measure text for alignment and overflow
    auto metrics = font_->MeasureText(text);
    float textW = metrics.width * fontScale;

    // Handle text-overflow: ellipsis
    std::string truncated;
    const char* renderText = text;
    if (widget.textOverflow == TextOverflow::Ellipsis && textW > cw && cw > 0) {
        auto ellipsisMetrics = font_->MeasureText("...");
        float ellipsisW = ellipsisMetrics.width * fontScale;
        float maxW = cw - ellipsisW;
        if (maxW > 0) {
            size_t len = std::strlen(text);
            size_t pos = 0;
            float penX = 0.0f;
            while (pos < len) {
                size_t prevPos = pos;
                uint32_t cp = DecodeUTF8(text, len, pos);
                const font::GlyphRegion* glyph = font_->GetGlyph(cp);
                float advance = glyph ? glyph->advance * fontScale
                                      : font_->PixelSize() * fontScale * 0.5f;
                if (penX + advance > maxW) {
                    truncated = std::string(text, prevPos) + "...";
                    break;
                }
                penX += advance + widget.letterSpacing;
            }
            if (!truncated.empty()) {
                renderText = truncated.c_str();
                textW = penX + ellipsisW;
            }
        }
    }

    // Horizontal: respect text-align
    float textX = cx;
    if (widget.textAlign == TextAlign::Center)
        textX = cx + (cw - textW) * 0.5f;
    else if (widget.textAlign == TextAlign::Right)
        textX = cx + cw - textW;

    // Text shadow (rendered before main text)
    if (style.textShadow.active) {
        Color4 sc = style.textShadow.color;
        if (style.opacity < 1.0f)
            sc.a = static_cast<uint8_t>(sc.a * style.opacity);
        EmitTextGlyphs(renderText, textX + style.textShadow.offsetX,
                        textY + style.textShadow.offsetY, fontSize, sc);
    }

    EmitTextGlyphs(renderText, textX, textY, fontSize, style.textColor);

    // Text decoration
    if (widget.textDecoration != TextDecoration::None) {
        float lineH = 1.0f;
        float actualTextW = textW;
        if (static_cast<uint8_t>(widget.textDecoration) &
            static_cast<uint8_t>(TextDecoration::Underline)) {
            float underY = textY + 2.0f;
            AddSolidRect(textX, underY, actualTextW, lineH, style.textColor);
        }
        if (static_cast<uint8_t>(widget.textDecoration) &
            static_cast<uint8_t>(TextDecoration::Strikethrough)) {
            float midY = textY - font_->Ascent() * fontScale * 0.35f;
            AddSolidRect(textX, midY, actualTextW, lineH, style.textColor);
        }
        if (static_cast<uint8_t>(widget.textDecoration) &
            static_cast<uint8_t>(TextDecoration::Overline)) {
            float overY = textY - font_->Ascent() * fontScale;
            AddSolidRect(textX, overY, actualTextW, lineH, style.textColor);
        }
    }

    PopScissor();
}

// Emit a menu item with optional shortcut text and submenu chevron
void UIDrawList::EmitMenuItem(const Widget& widget, const Rect& r,
                              const Style& style, const StringTable& strings) {
    if (!font_) return;
    const char* text = widget.textId != NullStringId ? strings.Lookup(widget.textId) : nullptr;

    float fontSize = style.fontSize > 0.0f ? style.fontSize : 14.0f;
    if (widget.fontSize > 0.0f) fontSize = widget.fontSize;
    float fontScale = fontSize / font_->PixelSize();

    float lineH = (font_->Ascent() - font_->Descent()) * fontScale;

    Edges pad = EffectivePadding(widget, style);
    float cx = r.x + pad.left;
    float cy = r.y + pad.top;
    float cw = r.w - pad.left - pad.right;
    float ch = r.h - pad.top - pad.bottom;

    PushScissor(cx, cy, cw, ch);

    // Left-aligned label
    if (text && text[0] != '\0') {
        float tx = cx;
        float ty = cy + (ch - lineH) * 0.5f + font_->Ascent() * fontScale;
        EmitTextGlyphs(text, tx, ty, fontSize, style.textColor, widget.fontWeight);

        // Text decoration (underline, strikethrough, overline)
        if (widget.textDecoration != TextDecoration::None) {
            float textW = font_->MeasureText(text).width * fontScale;
            float decoH = std::max(1.0f, std::floor(fontScale));
            if (static_cast<uint8_t>(widget.textDecoration) &
                static_cast<uint8_t>(TextDecoration::Underline)) {
                float underY = ty + 2.0f;
                AddSolidRect(tx, underY, textW, decoH, style.textColor);
            }
            if (static_cast<uint8_t>(widget.textDecoration) &
                static_cast<uint8_t>(TextDecoration::Strikethrough)) {
                float midY = ty - font_->Ascent() * fontScale * 0.35f;
                AddSolidRect(tx, midY, textW, decoH, style.textColor);
            }
            if (static_cast<uint8_t>(widget.textDecoration) &
                static_cast<uint8_t>(TextDecoration::Overline)) {
                float overY = ty - font_->Ascent() * fontScale;
                AddSolidRect(tx, overY, textW, decoH, style.textColor);
            }
        }
    }

    // Right-aligned shortcut text
    const char* shortcut = widget.shortcutTextId != NullStringId
        ? strings.Lookup(widget.shortcutTextId) : nullptr;
    if (shortcut && shortcut[0] != '\0') {
        float sw = font_->MeasureText(shortcut).width * fontScale;
        float sx = cx + cw - sw;
        float sy = cy + (ch - lineH) * 0.5f + font_->Ascent() * fontScale;
        Color4 dimColor = style.textColor;
        dimColor.a = static_cast<uint8_t>(dimColor.a * 0.6f);
        EmitTextGlyphs(shortcut, sx, sy, fontSize, dimColor);
    }

    // Submenu chevron (►) on the right edge
    if (widget.submenuIdx >= 0) {
        float chevW = fontSize * 0.5f;
        float chevX = cx + cw - chevW;
        float chevY = cy + (ch - lineH) * 0.5f + font_->Ascent() * fontScale;
        EmitTextGlyphs("\xe2\x96\xb6", chevX, chevY, fontSize * 0.7f, style.textColor);
    }

    PopScissor();
}

// Emit horizontally and vertically centered text (buttons)
void UIDrawList::EmitTextCentered(const Widget& widget, const Rect& r,
                                  const Style& style,
                                  const StringTable& strings) {
    if (!font_ || widget.textId == NullStringId) return;
    const char* text = strings.Lookup(widget.textId);
    if (!text || text[0] == '\0') return;

    float fontSize = style.fontSize > 0.0f ? style.fontSize : 14.0f;
    if (widget.fontSize > 0.0f) fontSize = widget.fontSize;
    float fontScale = fontSize / font_->PixelSize();

    // Content area (inset by padding)
    Edges pad = EffectivePadding(widget, style);
    float cx = r.x + pad.left;
    float cy = r.y + pad.top;
    float cw = r.w - pad.left - pad.right;
    float ch = r.h - pad.top - pad.bottom;

    // Measure text width for horizontal centering
    auto metrics = font_->MeasureText(text);
    float textW = metrics.width * fontScale;
    float textX = cx + (cw - textW) * 0.5f;

    // Vertical centering within content area
    float lineH = (font_->Ascent() - font_->Descent()) * fontScale;
    float textY = cy + (ch - lineH) * 0.5f + font_->Ascent() * fontScale;

    // Clip text to content area
    PushScissor(cx, cy, cw, ch);
    if (style.textShadow.active) {
        Color4 sc = style.textShadow.color;
        if (style.opacity < 1.0f)
            sc.a = static_cast<uint8_t>(sc.a * style.opacity);
        EmitTextGlyphs(text, textX + style.textShadow.offsetX,
                        textY + style.textShadow.offsetY, fontSize, sc);
    }
    EmitTextGlyphs(text, textX, textY, fontSize, style.textColor, widget.fontWeight);
    PopScissor();
}

// Emit a text field with cursor and placeholder support
void UIDrawList::EmitTextField(const Widget& widget, const Rect& r,
                               const Style& style,
                               const StringTable& strings) {
    if (!font_) return;

    float fontSize = style.fontSize > 0.0f ? style.fontSize : 14.0f;
    if (widget.fontSize > 0.0f) fontSize = widget.fontSize;
    float fontScale = fontSize / font_->PixelSize();

    // Content area
    Edges pad = EffectivePadding(widget, style);
    float cx = r.x + pad.left;
    float cy = r.y + pad.top;
    float cw = r.w - pad.left - pad.right;
    float ch = r.h - pad.top - pad.bottom;

    // Vertically centered within content area
    float lineH = (font_->Ascent() - font_->Descent()) * fontScale;
    float textY = cy + (ch - lineH) * 0.5f + font_->Ascent() * fontScale;
    float textX = cx;

    // Clip to content area
    PushScissor(cx, cy, cw, ch);

    // Render text or placeholder
    const char* text = strings.Lookup(widget.textId);
    if (text && text[0] != '\0') {
        EmitTextGlyphs(text, textX, textY, fontSize, style.textColor);
    } else {
        // Show placeholder text in placeholder color
        const char* placeholder = strings.Lookup(widget.placeholderId);
        if (placeholder && placeholder[0] != '\0') {
            EmitTextGlyphs(placeholder, textX, textY, fontSize, style.placeholderColor);
        }
    }

    // Blinking cursor (visible half the time based on frame)
    if (widget.flags.focused) {
        float cursorX = cx;
        if (text && text[0] != '\0' && widget.cursorPos > 0) {
            // Measure text up to cursor position — must exactly match EmitTextGlyphs
            float targetSize = fontSize;
            size_t len = std::strlen(text);
            size_t pos = 0;
            float penX = 0.0f;
            int charIdx = 0;
            uint32_t prevCp = 0;
            while (pos < len && charIdx < widget.cursorPos) {
                uint32_t cp = DecodeUTF8(text, len, pos);
                const font::GlyphRegion* glyph = font_->GetGlyphAtSize(cp, targetSize);
                if (!glyph) {
                    penX += targetSize * 0.5f;
                    prevCp = cp;
                    ++charIdx;
                    continue;
                }
                if (prevCp != 0) {
                    penX += font_->GetKerning(prevCp, cp) * fontScale;
                }
                penX += std::floor(glyph->advance + 0.5f);
                prevCp = cp;
                ++charIdx;
            }
            cursorX = cx + penX;
        }
        float cursorH = lineH;
        float cursorY = textY - font_->Ascent() * fontScale;
        AddSolidRect(std::floor(cursorX + 0.5f), std::floor(cursorY + 0.5f),
                     1.0f, cursorH, style.caretColor);
    }

    PopScissor();
}

// Emit a tree node row with connector lines and expand/collapse box
void UIDrawList::EmitTreeNode(const Widget& widget, const Rect& r,
                               const Style& style,
                               const StringTable& strings,
                               const WidgetPool& pool, int widgetIdx) {
    if (!font_ || widget.textId == NullStringId) return;
    const char* text = strings.Lookup(widget.textId);
    if (!text || text[0] == '\0') return;

    float fontSize = style.fontSize > 0.0f ? style.fontSize : 14.0f;
    if (widget.fontSize > 0.0f) fontSize = widget.fontSize;
    float fontScale = fontSize / font_->PixelSize();

    Edges pad = EffectivePadding(widget, style);
    float cx = r.x + pad.left;
    float cy = r.y + pad.top;
    float cw = r.w - pad.left - pad.right;
    float ch = r.h - pad.top - pad.bottom;

    // Layout constants
    float indentStep = 16.0f;
    float boxSize    = 10.0f;
    float lineThick  = 1.0f;
    int   depth      = widget.treeDepth;
    bool  hasKids    = widget.treeHasChildren;
    Color4 lineColor = style.textColor;
    lineColor.a = static_cast<uint8_t>(lineColor.a * 0.35f);
    Color4 boxColor  = style.textColor;
    boxColor.a = static_cast<uint8_t>(boxColor.a * 0.5f);

    float rowCenterY = cy + ch * 0.5f;

    // --- Determine which depth-levels have continuing siblings below ---
    // For depth level d, the vertical line represents "the ancestor at depth d
    // has more children after the current row." In DFS-order flat list, a child
    // of the ancestor at depth d is at depth d+1. So continuesAtDepth[d] is true
    // if there's a future node at depth d+1 before any node at depth <= d
    // (which would exit the ancestor's subtree).
    // For the current depth: continuesAtDepth[depth] = true if there's another
    // node at the same depth before a node at depth < depth.
    bool continuesAtDepth[32] = {};
    if (widget.parent >= 0) {
        const Widget* par = pool.Get(widget.parent);
        if (par) {
            int myIdx = -1;
            for (int c = 0; c < par->childCount; ++c) {
                if (par->children[c] == widgetIdx) { myIdx = c; break; }
            }
            if (myIdx >= 0) {
                // Per-level scan for ancestor continuation
                for (int d = 0; d < depth && d < 32; ++d) {
                    for (int c = myIdx + 1; c < par->childCount; ++c) {
                        const Widget* sib = pool.Get(par->children[c]);
                        if (!sib || sib->tag != WidgetTag::TreeNode) continue;
                        if (!sib->flags.visible) continue;
                        int sd = sib->treeDepth;
                        if (sd <= d) break; // exited ancestor's subtree
                        if (sd == d + 1) {
                            continuesAtDepth[d] = true;
                            break;
                        }
                    }
                }
                // Check if current depth has more siblings
                if (depth < 32) {
                    for (int c = myIdx + 1; c < par->childCount; ++c) {
                        const Widget* sib = pool.Get(par->children[c]);
                        if (!sib || sib->tag != WidgetTag::TreeNode) continue;
                        if (!sib->flags.visible) continue;
                        int sd = sib->treeDepth;
                        if (sd < depth) break;
                        if (sd == depth) {
                            continuesAtDepth[depth] = true;
                            break;
                        }
                    }
                }
            }
        }
    }

    // --- Draw connector lines for each depth level ---
    for (int d = 0; d < depth; ++d) {
        float lineX = cx + d * indentStep + boxSize * 0.5f;
        // Vertical line: only draw if there are nodes below at this depth
        // Skip depth-1 because the parent connector below handles that level
        if (continuesAtDepth[d] && d < depth - 1) {
            AddSolidRect(lineX, r.y, lineThick, r.h, lineColor);
        }
    }

    // --- Connector from parent's vertical line to this node ---
    if (depth > 0) {
        float parentLineX = cx + (depth - 1) * indentStep + boxSize * 0.5f;
        float nodeLeftX   = cx + depth * indentStep;

        bool isLastAtDepth = !continuesAtDepth[depth];
        // Vertical segment: top of row down to row center
        AddSolidRect(parentLineX, r.y, lineThick, rowCenterY - r.y, lineColor);

        // If NOT the last child at this depth, extend vertical to bottom
        if (!isLastAtDepth) {
            AddSolidRect(parentLineX, rowCenterY, lineThick, r.y + r.h - rowCenterY, lineColor);
        }

        // Horizontal segment: from parent vertical line to this node's content
        float horizEndX = hasKids ? nodeLeftX : (nodeLeftX + boxSize * 0.5f);
        AddSolidRect(parentLineX + lineThick, rowCenterY, horizEndX - parentLineX - lineThick, lineThick, lineColor);
    }

    // --- Draw [+]/[-] box if node has children ---
    float boxX = cx + depth * indentStep;
    float boxY = rowCenterY - boxSize * 0.5f;

    if (hasKids) {
        AddBorderRect(boxX, boxY, boxSize, boxSize, boxColor, lineThick);

        // Horizontal bar (always present for +/-)
        float barPad = 2.0f;
        float barY = boxY + boxSize * 0.5f - lineThick * 0.5f;
        AddSolidRect(boxX + barPad, barY, boxSize - barPad * 2, lineThick, boxColor);

        // Vertical bar (only when collapsed = plus sign)
        if (!widget.expanded) {
            float barX = boxX + boxSize * 0.5f - lineThick * 0.5f;
            AddSolidRect(barX, boxY + barPad, lineThick, boxSize - barPad * 2, boxColor);
        }

        // Vertical line from bottom of box down to children (when expanded)
        if (widget.expanded) {
            float lineX = boxX + boxSize * 0.5f;
            AddSolidRect(lineX, boxY + boxSize, lineThick, r.y + r.h - (boxY + boxSize), lineColor);
        }
    }

    // --- Text after indent + box ---
    float textX = cx + depth * indentStep + (hasKids ? boxSize + 4.0f : boxSize * 0.5f + 4.0f);
    float textY = cy + (ch - (font_->Ascent() - font_->Descent()) * fontScale) * 0.5f
                  + font_->Ascent() * fontScale;

    PushScissor(cx, cy, cw, ch);
    EmitTextGlyphs(text, textX, textY, fontSize, style.textColor);
    PopScissor();
}

// Emit a vertical scrollbar track and thumb
void UIDrawList::EmitScrollbar(const Widget& widget, const Rect& r,
                                const Style& style) {
    // Use actual content height from layout
    float contentH = widget.contentHeight;
    if (contentH <= r.h) return; // No scrollbar needed

    float barWidth = 8.0f;
    float trackX = r.x + r.w - barWidth;
    float trackY = r.y;
    float trackH = r.h;

    // Track background
    Color4 trackColor{40, 40, 40, 120};
    AddSolidRect(trackX, trackY, barWidth, trackH, trackColor);

    // Thumb
    float viewRatio = r.h / contentH;
    if (viewRatio > 1.0f) viewRatio = 1.0f;
    float thumbH = std::max(20.0f, trackH * viewRatio);
    float maxScroll = contentH - r.h;
    float scrollT = (maxScroll > 0.0f) ? (-widget.scrollY / maxScroll) : 0.0f;
    if (scrollT < 0.0f) scrollT = 0.0f;
    if (scrollT > 1.0f) scrollT = 1.0f;
    float thumbY = trackY + scrollT * (trackH - thumbH);

    Color4 thumbColor{100, 100, 110, 180};
    AddRoundedRect(trackX, thumbY, barWidth, thumbH,
                   barWidth * 0.5f, thumbColor);
}

// Emit a dropdown with selected text, arrow, and popup menu
void UIDrawList::EmitDropdown(const Widget& widget, const Rect& r,
                               const Style& style,
                               const StringTable& strings,
                               const WidgetPool& pool, int widgetIdx) {
    if (!font_) return;

    float fontSize = style.fontSize > 0.0f ? style.fontSize : 14.0f;
    if (widget.fontSize > 0.0f) fontSize = widget.fontSize;
    float fontScale = fontSize / font_->PixelSize();

    Edges pad = EffectivePadding(widget, style);
    float cx = r.x + pad.left;
    float cy = r.y + pad.top;
    float cw = r.w - pad.left - pad.right;
    float ch = r.h - pad.top - pad.bottom;

    // Draw selected text (from child at selectedIndex, or widget text)
    const char* selectedText = nullptr;
    if (widget.selectedIndex >= 0 && widget.selectedIndex < widget.childCount) {
        const Widget* selChild = pool.Get(widget.children[widget.selectedIndex]);
        if (selChild && selChild->textId != NullStringId) {
            selectedText = strings.Lookup(selChild->textId);
        }
    }
    if (!selectedText && widget.textId != NullStringId) {
        selectedText = strings.Lookup(widget.textId);
    }
    if (selectedText && selectedText[0] != '\0') {
        float lineH = (font_->Ascent() - font_->Descent()) * fontScale;
        float textY = cy + (ch - lineH) * 0.5f + font_->Ascent() * fontScale;
        PushScissor(cx, cy, cw - 16.0f, ch);
        EmitTextGlyphs(selectedText, cx, textY, fontSize, style.textColor);
        PopScissor();
    }

    // Draw ▼ arrow on the right
    float arrowSize = 6.0f;
    float arrowX = r.x + r.w - pad.right - arrowSize - 4.0f;
    float arrowY = r.y + (r.h - arrowSize * 0.5f) * 0.5f;
    Color4 arrowColor = style.textColor;
    AddSolidRect(arrowX, arrowY, arrowSize, 2.0f, arrowColor);
    AddSolidRect(arrowX + 1.0f, arrowY + 2.0f, arrowSize - 2.0f, 2.0f, arrowColor);
    AddSolidRect(arrowX + 2.0f, arrowY + 4.0f, arrowSize - 4.0f, 1.0f, arrowColor);

    // Defer popup rendering to overlay pass so it draws on top of everything.
    if (widget.dropdownOpen && widget.childCount > 0) {
        deferredDropdowns_.push_back({&widget, r, style, &pool, &strings, widgetIdx});
    }
}

// Emit a deferred dropdown popup overlay (called after main tree pass).
void UIDrawList::EmitDropdownPopup(const Widget& widget, const Rect& r,
                                    const Style& style,
                                    const StringTable& strings,
                                    const WidgetPool& pool) {
    if (!font_) return;

    float fontSize = style.fontSize > 0.0f ? style.fontSize : 14.0f;
    if (widget.fontSize > 0.0f) fontSize = widget.fontSize;
    float fontScale = fontSize / font_->PixelSize();

    Edges pad = EffectivePadding(widget, style);
    float itemH = r.h;
    float popupH = widget.childCount * itemH;
    float popupY = r.y + r.h + 2.0f;
    float popupW = r.w;

    // Background
    Color4 popupBg = {40, 40, 50, 245};
    Color4 popupBorder = {80, 80, 100, 200};
    AddSolidRect(r.x, popupY, popupW, popupH, popupBg);
    AddBorderRect(r.x, popupY, popupW, popupH, popupBorder, 1.0f);

    // Items
    for (int i = 0; i < widget.childCount; ++i) {
        const Widget* child = pool.Get(widget.children[i]);
        if (!child) continue;
        const char* itemText = child->textId != NullStringId
            ? strings.Lookup(child->textId) : "";
        if (!itemText || itemText[0] == '\0') continue;

        float iy = popupY + i * itemH;

        // Highlight selected item
        if (i == widget.selectedIndex) {
            Color4 selBg = {74, 128, 200, 80};
            AddSolidRect(r.x + 1.0f, iy, popupW - 2.0f, itemH, selBg);
        }

        float lineH = (font_->Ascent() - font_->Descent()) * fontScale;
        float textY = iy + (itemH - lineH) * 0.5f + font_->Ascent() * fontScale;
        EmitTextGlyphs(itemText, r.x + pad.left, textY, fontSize, style.textColor);
    }
}

// Emit a textured image widget
void UIDrawList::EmitImage(const Widget& widget, const Rect& r) {
    if (widget.textureId == 0) return;
    AddTexturedRect(r.x, r.y, r.w, r.h,
                    0.0f, 0.0f, 1.0f, 1.0f,
                    {255, 255, 255, 255}, widget.textureId);
}

// Emit a color swatch with border
void UIDrawList::EmitColorField(const Widget& widget, const Rect& r,
                                 const Style& style) {
    // Color swatch inset by padding
    Edges pad = EffectivePadding(widget, style);
    float cx = r.x + pad.left;
    float cy = r.y + pad.top;
    float cw = r.w - pad.left - pad.right;
    float ch = r.h - pad.top - pad.bottom;

    if (style.border.HasRadius()) {
        AddRoundedRect(cx, cy, cw, ch,
                       style.border.radiusTL, style.border.radiusTR,
                       style.border.radiusBR, style.border.radiusBL, widget.colorValue);
    } else {
        AddSolidRect(cx, cy, cw, ch, widget.colorValue);
    }

    // Border around swatch
    Color4 borderCol{80, 80, 80, 200};
    if (style.border.HasRadius()) {
        AddRoundedBorderRect(cx, cy, cw, ch,
                             style.border.radiusTL, style.border.radiusTR,
                             style.border.radiusBR, style.border.radiusBL, borderCol, 1.0f);
    } else {
        AddBorderRect(cx, cy, cw, ch, borderCol, 1.0f);
    }
}

// Emit a checkbox with border and filled inner rect when checked
void UIDrawList::EmitCheckbox(const Widget& widget, const Rect& r,
                              const Style& style) {
    float boxSize = std::min(r.w, r.h) * 0.6f;
    float bx = r.x + (r.w - boxSize) * 0.5f;
    float by = r.y + (r.h - boxSize) * 0.5f;

    // Checkbox border
    AddBorderRect(bx, by, boxSize, boxSize,
                  style.textColor, 2.0f);

    // Check mark (filled inner rect when checked) - uses accent color
    if (widget.checked) {
        float inset = boxSize * 0.2f;
        AddSolidRect(bx + inset, by + inset,
                     boxSize - inset * 2.0f, boxSize - inset * 2.0f,
                     style.accentColor);
    }
}

// Emit a slider with track, filled portion, and thumb
void UIDrawList::EmitSlider(const Widget& widget, const Rect& r,
                            const Style& style) {
    float trackH = 4.0f;
    float trackY = r.y + (r.h - trackH) * 0.5f;

    // Normalize value to 0-1 range
    float range = widget.sliderMax - widget.sliderMin;
    float t = (range > 0.0f) ? (widget.sliderValue - widget.sliderMin) / range : 0.0f;
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;

    // Thumb sizing - keep within slider bounds
    float thumbH = std::min(r.h * 0.7f, 12.0f);
    float thumbW = thumbH;
    float slideRange = r.w - thumbW;  // travel distance for thumb center
    float thumbX = r.x + t * slideRange;
    float thumbY = r.y + (r.h - thumbH) * 0.5f;

    // Track background (inset by half thumb width so track sits between thumb stops)
    float trackX = r.x + thumbW * 0.5f;
    float trackW = r.w - thumbW;
    Color4 trackColor = style.background;
    trackColor.a = static_cast<uint8_t>(std::min(255, static_cast<int>(trackColor.a) + 40));
    AddSolidRect(trackX, trackY, trackW, trackH, trackColor);

    // Filled portion - uses accent color
    float filledW = t * trackW;
    AddSolidRect(trackX, trackY, filledW, trackH, style.accentColor);

    // Thumb
    AddSolidRect(thumbX, thumbY, thumbW, thumbH, style.textColor);
}

// Emit a horizontal separator line
void UIDrawList::EmitSeparator(const Widget& widget, const Rect& r,
                               const Style& style) {
    (void)widget;
    float lineH = std::max(1.0f, style.border.width);
    float lineY = r.y + (r.h - lineH) * 0.5f;
    AddSolidRect(r.x, lineY, r.w, lineH, style.border.color);
}

// Emit a floating panel with title bar, shadow, and chrome buttons
void UIDrawList::EmitFloatingPanel(const Widget& widget, const Rect& r,
                                    const Style& style,
                                    const StringTable& strings) {
    float tbH = Widget::TITLE_BAR_HEIGHT;

    // Drop shadow
    Color4 shadow = {0, 0, 0, 80};
    AddSolidRect(r.x + 3, r.y + 3, r.w, r.h, shadow);

    // Panel background
    Color4 bg = style.background;
    if (bg.a == 0) bg = {30, 30, 38, 245};
    AddSolidRect(r.x, r.y, r.w, r.h, bg);

    // Title bar
    Color4 titleBg = {45, 45, 55, 255};
    AddSolidRect(r.x, r.y, r.w, tbH, titleBg);

    // Title bar bottom border
    Color4 borderColor = {70, 70, 85, 255};
    AddSolidRect(r.x, r.y + tbH - 1.0f, r.w, 1.0f, borderColor);

    // Panel outer border
    AddBorderRect(r.x, r.y, r.w, r.h, borderColor, 1.0f);

    // Title text
    if (font_ && widget.panelTitleId != NullStringId) {
        const char* title = strings.Lookup(widget.panelTitleId);
        if (title && title[0]) {
            float fontSize = style.fontSize > 0 ? style.fontSize : 12.0f;
            float fontScale = fontSize / font_->PixelSize();
            float textY = r.y + (tbH - (font_->Ascent() - font_->Descent()) * fontScale) * 0.5f
                          + font_->Ascent() * fontScale;
            float textX = r.x + 8.0f;
            EmitTextGlyphs(title, textX, textY, fontSize, {210, 210, 220, 255});
        }
    }

    float btnSize = tbH - 6.0f;
    float btnY = r.y + 3.0f;
    Color4 btnColor = {180, 180, 190, 255};
    Color4 btnHoverBg = {60, 60, 70, 180};
    float lineThick = 2.0f;

    // Close button (X) — rightmost, with hover background
    float closeBtnX = r.x + r.w - btnSize - 4.0f;
    {
        // Button background area
        AddSolidRect(closeBtnX, btnY, btnSize, btnSize, btnHoverBg);

        float cx = closeBtnX + btnSize * 0.5f;
        float cy = btnY + btnSize * 0.5f;
        float arm = btnSize * 0.25f;
        // Draw X with 2px thick diagonal lines
        for (float t = -arm; t <= arm; t += 1.0f) {
            AddSolidRect(cx + t - lineThick * 0.5f, cy + t - lineThick * 0.5f, lineThick, lineThick, btnColor);
            AddSolidRect(cx + t - lineThick * 0.5f, cy - t - lineThick * 0.5f, lineThick, lineThick, btnColor);
        }
    }

    // Minimize button (—) — left of close button, with hover background
    float minBtnX = closeBtnX - btnSize - 4.0f;
    {
        AddSolidRect(minBtnX, btnY, btnSize, btnSize, btnHoverBg);

        float lineY = btnY + btnSize * 0.5f;
        float pad = btnSize * 0.2f;
        AddSolidRect(minBtnX + pad, lineY - lineThick * 0.5f,
                     btnSize - pad * 2.0f, lineThick, btnColor);
    }
}

// Emit tray of minimized panel restore buttons
void UIDrawList::EmitMinimizedPanelTray(const WidgetPool& pool,
                                         const StringTable& strings,
                                         float viewportW, float viewportH) {
    // Collect minimized panels
    struct MinPanel { int idx; StringId titleId; };
    std::vector<MinPanel> panels;
    for (size_t i = 0; i < pool.Capacity(); ++i) {
        const Widget* w = pool.Get(static_cast<int>(i));
        if (w && pool.IsAlive(static_cast<int>(i))
            && w->tag == WidgetTag::FloatingPanel && w->flags.minimized) {
            panels.push_back({static_cast<int>(i), w->panelTitleId});
        }
    }
    if (panels.empty()) return;

    // Render tray buttons centered horizontally, at the bottom above status bar
    float btnH = 22.0f;
    float btnPad = 6.0f;
    float btnGap = 4.0f;
    float fontSize = 11.0f;
    float charW = fontSize * 0.55f; // approximate character width

    // Calculate total width to center the tray
    float totalW = 0.0f;
    for (auto& p : panels) {
        const char* title = strings.Lookup(p.titleId);
        int len = title ? static_cast<int>(strlen(title)) : 0;
        float w = std::max(60.0f, len * charW + btnPad * 2.0f);
        totalW += w + btnGap;
    }
    totalW -= btnGap; // remove trailing gap

    float startX = (viewportW - totalW) * 0.5f;
    float trayY = viewportH - 26.0f - btnH - 4.0f; // above the 26px status bar

    Color4 trayBg = {40, 40, 50, 230};
    Color4 trayBorder = {70, 70, 85, 255};
    Color4 textColor = {200, 200, 210, 255};

    float cx = startX;
    for (auto& p : panels) {
        const char* title = strings.Lookup(p.titleId);
        int len = title ? static_cast<int>(strlen(title)) : 0;
        float btnW = std::max(60.0f, len * charW + btnPad * 2.0f);

        // Button background
        AddSolidRect(cx, trayY, btnW, btnH, trayBg);
        AddBorderRect(cx, trayY, btnW, btnH, trayBorder, 1.0f);

        // Button text
        if (font_ && title && title[0]) {
            float fontScale = fontSize / font_->PixelSize();
            float textY = trayY + (btnH - (font_->Ascent() - font_->Descent()) * fontScale) * 0.5f
                          + font_->Ascent() * fontScale;
            EmitTextGlyphs(title, cx + btnPad, textY, fontSize, textColor);
        }

        cx += btnW + btnGap;
    }
}

// Emit resize divider handles between sibling children
void UIDrawList::EmitDividers(const Widget& parent, const WidgetPool& pool) {
    if (parent.childCount < 2) return;
    bool isRow = (parent.layout.direction == LayoutDirection::Row);
    Color4 divColor{70, 70, 80, 200};

    int prevChild = -1;
    for (int c = 0; c < parent.childCount; ++c) {
        const Widget* child = pool.Get(parent.children[c]);
        if (!child || !child->flags.visible) continue;
        // Skip absolute-positioned children (floating panels, overlays)
        if (child->positionMode == PositionMode::Absolute) continue;

        if (prevChild >= 0) {
            const Widget* childA = pool.Get(prevChild);
            if (childA) {
                if (isRow) {
                    float dx = childA->computedRect.x + childA->computedRect.w;
                    float gap = child->computedRect.x - dx;
                    float barX = dx + (gap - 2.0f) * 0.5f;
                    float barY = std::min(childA->computedRect.y, child->computedRect.y);
                    float barH = std::max(childA->computedRect.y + childA->computedRect.h,
                                          child->computedRect.y + child->computedRect.h) - barY;
                    AddSolidRect(barX, barY, 2.0f, barH, divColor);
                } else {
                    float dy = childA->computedRect.y + childA->computedRect.h;
                    float gap = child->computedRect.y - dy;
                    float barY = dy + (gap - 2.0f) * 0.5f;
                    float barX = std::min(childA->computedRect.x, child->computedRect.x);
                    float barW = std::max(childA->computedRect.x + childA->computedRect.w,
                                          child->computedRect.x + child->computedRect.w) - barX;
                    AddSolidRect(barX, barY, barW, 2.0f, divColor);
                }
            }
        }
        prevChild = parent.children[c];
    }
}

// Emit a progress bar with rounded track and fill
void UIDrawList::EmitProgressBar(const Widget& widget, const Rect& r,
                                  const Style& style) {
    Edges pad = EffectivePadding(widget, style);
    float cx = r.x + pad.left;
    float cy = r.y + pad.top;
    float cw = r.w - pad.left - pad.right;
    float ch = r.h - pad.top - pad.bottom;

    // Track background
    float radius = ch * 0.5f;
    Color4 trackColor{40, 40, 50, 255};
    AddRoundedRect(cx, cy, cw, ch, radius, radius, radius, radius, trackColor);

    // Filled portion
    float fillW = cw * std::max(0.0f, std::min(1.0f, widget.progressValue));
    if (fillW > 0.0f) {
        Color4 fillColor = style.accentColor.a > 0 ? style.accentColor : Color4{80, 140, 200, 255};
        float fr = std::min(radius, fillW * 0.5f);
        AddRoundedRect(cx, cy, fillW, ch, fr, fr, fr, fr, fillColor);
    }
}

// Emit a toggle switch with sliding knob
void UIDrawList::EmitToggleSwitch(const Widget& widget, const Rect& r,
                                   const Style& style) {
    float trackW = r.w;
    float trackH = r.h;
    float radius = trackH * 0.5f;

    // Track
    Color4 trackColor = widget.checked
        ? (style.accentColor.a > 0 ? style.accentColor : Color4{80, 180, 100, 255})
        : Color4{60, 60, 70, 255};
    AddRoundedRect(r.x, r.y, trackW, trackH, radius, radius, radius, radius, trackColor);

    // Knob
    float knobInset = 2.0f;
    float knobD = trackH - knobInset * 2.0f;
    float knobX = widget.checked
        ? r.x + trackW - knobD - knobInset
        : r.x + knobInset;
    float knobY = r.y + knobInset;
    float kr = knobD * 0.5f;
    AddRoundedRect(knobX, knobY, knobD, knobD, kr, kr, kr, kr, Color4{240, 240, 240, 255});
}

// Emit a radio button with outer circle and inner dot
void UIDrawList::EmitRadioButton(const Widget& widget, const Rect& r,
                                  const Style& style) {
    float size = std::min(r.w, r.h);
    float cx = r.x + (r.w - size) * 0.5f;
    float cy = r.y + (r.h - size) * 0.5f;
    float radius = size * 0.5f;

    // Outer circle (border)
    AddRoundedBorderRect(cx, cy, size, size, radius, radius, radius, radius,
                         style.textColor, 2.0f);

    // Inner dot when selected
    if (widget.checked) {
        float dotSize = size * 0.5f;
        float dotR = dotSize * 0.5f;
        float dx = cx + (size - dotSize) * 0.5f;
        float dy = cy + (size - dotSize) * 0.5f;
        Color4 dotColor = style.accentColor.a > 0 ? style.accentColor : Color4{80, 140, 200, 255};
        AddRoundedRect(dx, dy, dotSize, dotSize, dotR, dotR, dotR, dotR, dotColor);
    }
}

// Emit a number spinner with -/+ buttons and centered value text
void UIDrawList::EmitNumberSpinner(const Widget& widget, const Rect& r,
                                    const Style& style, const StringTable& strings) {
    Edges pad = EffectivePadding(widget, style);
    float cx = r.x + pad.left;
    float cy = r.y + pad.top;
    float cw = r.w - pad.left - pad.right;
    float ch = r.h - pad.top - pad.bottom;

    // Button zones (square buttons matching content height)
    float btnW = ch;

    // Left button (-)
    Color4 btnColor{50, 50, 60, 255};
    AddSolidRect(cx, cy, btnW, ch, btnColor);
    // Minus sign
    float signY = cy + ch * 0.5f - 1.0f;
    AddSolidRect(cx + 4.0f, signY, btnW - 8.0f, 2.0f, style.textColor);

    // Right button (+)
    float rbx = cx + cw - btnW;
    AddSolidRect(rbx, cy, btnW, ch, btnColor);
    // Plus sign (horizontal)
    AddSolidRect(rbx + 4.0f, signY, btnW - 8.0f, 2.0f, style.textColor);
    // Plus sign (vertical)
    float plusX = rbx + btnW * 0.5f - 1.0f;
    AddSolidRect(plusX, cy + 4.0f, 2.0f, ch - 8.0f, style.textColor);

    // Value text in center
    if (font_) {
        char buf[32];
        // Show integer if step >= 1, otherwise 2 decimal places
        if (widget.spinnerStep >= 1.0f)
            std::snprintf(buf, sizeof(buf), "%d", static_cast<int>(widget.sliderValue));
        else
            std::snprintf(buf, sizeof(buf), "%.2f", widget.sliderValue);

        float fontSize = style.fontSize > 0.0f ? style.fontSize : 14.0f;
        if (widget.fontSize > 0.0f) fontSize = widget.fontSize;
        float fontScale = fontSize / font_->PixelSize();

        auto metrics = font_->MeasureText(buf);
        float textW = metrics.width * fontScale;
        float textAreaW = cw - btnW * 2.0f;
        float textX = cx + btnW + (textAreaW - textW) * 0.5f;
        float textY = cy + (ch - (font_->Ascent() - font_->Descent()) * fontScale) * 0.5f
                     + font_->Ascent() * fontScale;

        PushScissor(cx + btnW, cy, textAreaW, ch);
        EmitTextGlyphs(buf, textX, textY, fontSize, style.textColor);
        PopScissor();
    }
}

// ============================================================================
// Text rendering helpers
// ============================================================================

// Emit individual glyph quads for a text string
void UIDrawList::EmitTextGlyphs(const char* text, float x, float y,
                                float fontSize, Color4 color, uint16_t fontWeight) {
    if (!font_) return;
    // Rasterize glyphs at exact display size for pixel-perfect rendering
    float targetSize = fontSize > 0.0f ? fontSize : font_->PixelSize();

    // Snap baseline to integer pixel for consistent vertical alignment
    y = std::floor(y + 0.5f);
    // Snap starting pen X to integer pixel
    float penX = std::floor(x + 0.5f);
    size_t len = std::strlen(text);
    size_t i = 0;

    // Line height at target size
    float fontScale = targetSize / font_->PixelSize();
    float lineAdv = std::floor(font_->LineHeight() * fontScale + 0.5f);

    // Synthetic bold: extra pixel offsets for weights above normal (400)
    float boldOffset = 0.0f;
    if (fontWeight > 400) {
        boldOffset = std::max(0.5f, (fontWeight - 400) / 600.0f * (targetSize / 14.0f));
        boldOffset = std::min(boldOffset, targetSize * 0.15f);
    }

    uint32_t prevCp = 0;
    while (i < len) {
        uint32_t cp = DecodeUTF8(text, len, i);
        if (cp == '\n') {
            penX = std::floor(x + 0.5f);
            y += lineAdv;
            prevCp = 0;
            continue;
        }

        const font::GlyphRegion* glyph = font_->GetGlyphAtSize(cp, targetSize);
        if (!glyph) {
            penX += targetSize * 0.5f;
            prevCp = cp;
            continue;
        }

        // Apply kerning between previous and current glyph
        if (prevCp != 0) {
            float kern = font_->GetKerning(prevCp, cp) * fontScale;
            penX += kern;
        }

        if (glyph->width > 0 && glyph->height > 0) {
            // Glyph is rasterized at target size - render at 1:1 pixel scale
            float gx = std::floor(penX + glyph->bearingX + 0.5f);
            float gy = std::floor(y - glyph->bearingY + 0.5f);
            float gw = static_cast<float>(glyph->width);
            float gh = static_cast<float>(glyph->height);

            AddTexturedRect(gx, gy, gw, gh,
                            glyph->u0, glyph->v0,
                            glyph->u1, glyph->v1,
                            color, fontAtlasTexture_);

            // Synthetic bold: re-draw glyph at slight X offsets
            if (boldOffset > 0.0f) {
                AddTexturedRect(gx + boldOffset, gy, gw, gh,
                                glyph->u0, glyph->v0,
                                glyph->u1, glyph->v1,
                                color, fontAtlasTexture_);
                if (fontWeight >= 700) {
                    AddTexturedRect(gx - boldOffset * 0.5f, gy, gw, gh,
                                    glyph->u0, glyph->v0,
                                    glyph->u1, glyph->v1,
                                    color, fontAtlasTexture_);
                }
            }
        }

        penX += std::floor(glyph->advance + 0.5f);
        prevCp = cp;
    }
}

// Decode one UTF-8 codepoint and advance the byte index
uint32_t UIDrawList::DecodeUTF8(const char* text, size_t len, size_t& i) {
    uint8_t c = static_cast<uint8_t>(text[i]);
    uint32_t cp = 0;
    if (c < 0x80)                    { cp = c; i += 1; }
    else if ((c & 0xE0) == 0xC0) {
        if (i+1 >= len) { i = len; return 0xFFFD; }
        cp = ((c & 0x1F) << 6) | (text[i+1] & 0x3F); i += 2;
    } else if ((c & 0xF0) == 0xE0) {
        if (i+2 >= len) { i = len; return 0xFFFD; }
        cp = ((c & 0x0F) << 12) | ((text[i+1] & 0x3F) << 6) | (text[i+2] & 0x3F); i += 3;
    } else if ((c & 0xF8) == 0xF0) {
        if (i+3 >= len) { i = len; return 0xFFFD; }
        cp = ((c & 0x07) << 18) | ((text[i+1] & 0x3F) << 12) | ((text[i+2] & 0x3F) << 6) | (text[i+3] & 0x3F); i += 4;
    } else { i += 1; return 0xFFFD; }
    return cp;
}

// ============================================================================
// Overlay rendering
// ============================================================================

// Emit a tooltip popup near the mouse cursor
void UIDrawList::EmitTooltip(const char* text, float mouseX, float mouseY,
                              float vpW, float vpH) {
    if (!text || !text[0] || !font_) return;

    float fontSize = 12.0f;
    float padX = 6.0f, padY = 4.0f;

    // Measure text width
    float textW = 0.0f;
    size_t len = std::strlen(text);
    size_t i = 0;
    while (i < len) {
        uint32_t cp = DecodeUTF8(text, len, i);
        const auto* glyph = font_->GetGlyphAtSize(cp, fontSize);
        if (glyph) textW += glyph->advance;
    }

    float boxW = textW + padX * 2;
    float boxH = fontSize + padY * 2;

    // Position: below and right of cursor, clamped to viewport
    float tx = mouseX + 12.0f;
    float ty = mouseY + 20.0f;
    if (tx + boxW > vpW) tx = vpW - boxW - 2.0f;
    if (ty + boxH > vpH) ty = mouseY - boxH - 4.0f;
    if (tx < 0) tx = 2.0f;
    if (ty < 0) ty = 2.0f;

    // Shadow
    Color4 shadowCol{0, 0, 0, 120};
    AddRoundedRect(tx + 1, ty + 1, boxW, boxH, 3.0f, shadowCol);

    // Background + border
    Color4 bgCol{50, 50, 58, 240};
    Color4 borderCol{90, 90, 100, 255};
    AddRoundedRect(tx, ty, boxW, boxH, 3.0f, bgCol);
    AddRoundedBorderRect(tx, ty, boxW, boxH, 3.0f, borderCol, 1.0f);

    // Text
    Color4 textCol{230, 230, 235, 255};
    float textX = tx + padX;
    float textY = ty + padY;
    i = 0;
    while (i < len) {
        uint32_t cp = DecodeUTF8(text, len, i);
        const auto* glyph = font_->GetGlyphAtSize(cp, fontSize);
        if (!glyph) continue;

        float gx = textX + glyph->bearingX;
        float gy = textY + (fontSize - glyph->bearingY);
        float gw = static_cast<float>(glyph->width);
        float gh = static_cast<float>(glyph->height);

        if (glyph->width > 0 && glyph->height > 0) {
            float atlasW = static_cast<float>(font_->Atlas().Width());
            float atlasH = static_cast<float>(font_->Atlas().Height());
            float u0 = static_cast<float>(glyph->atlasX) / atlasW;
            float v0 = static_cast<float>(glyph->atlasY) / atlasH;
            float u1 = static_cast<float>(glyph->atlasX + glyph->width) / atlasW;
            float v1 = static_cast<float>(glyph->atlasY + glyph->height) / atlasH;
            AddTexturedRect(gx, gy, gw, gh, u0, v0, u1, v1, textCol, fontAtlasTexture_);
        }
        textX += glyph->advance;
    }
}

// Emit the drag-and-drop ghost label and drop indicators
void UIDrawList::EmitDragOverlay(const UIContext& ctx, const StringTable& strings) {
    const auto& drag = ctx.ActiveDrag();
    float mx = ctx.DragPointerX();
    float my = ctx.DragPointerY();

    // Ghost label following cursor
    const char* labelText = nullptr;
    if (drag.labelId != 0) labelText = strings.Lookup(drag.labelId);
    if (!labelText || !labelText[0]) labelText = "Drag";

    float labelW = static_cast<float>(std::strlen(labelText)) * 7.0f + 16.0f;
    float labelH = 22.0f;
    float gx = mx + 12.0f;
    float gy = my - 8.0f;

    // Semi-transparent background
    AddRoundedRect(gx, gy, labelW, labelH, 4.0f, Color4{50, 50, 60, 200});
    EmitTextGlyphs(labelText, gx + 8.0f, gy + 3.0f, 13.0f, Color4{220, 220, 230, 240});

    // Drop position indicator on tree nodes
    int hoverIdx = ctx.DragHoverTarget();
    if (hoverIdx >= 0) {
        const Widget* target = ctx.Pool().Get(hoverIdx);
        if (target && target->tag == WidgetTag::TreeNode) {
            const Rect& tr = target->computedRect;
            DropPosition pos = ctx.DragDropPosition();
            Color4 indicator{80, 160, 240, 200};
            if (pos == DropPosition::Before) {
                AddSolidRect(tr.x, tr.y - 1.0f, tr.w, 2.0f, indicator);
            } else if (pos == DropPosition::After) {
                AddSolidRect(tr.x, tr.y + tr.h - 1.0f, tr.w, 2.0f, indicator);
            } else {
                // Into: highlight border around target
                AddBorderRect(tr.x, tr.y, tr.w, tr.h, indicator, 2.0f);
            }
        }
    }
}

// Draw text at canvas-relative coordinates
void CanvasDrawContext::DrawText(float x, float y, const char* text,
                                 Color4 color, float fontSize) {
    if (!font_ || !text || text[0] == '\0') return;
    float fontScale = fontSize / font_->PixelSize();
    float textY = oy_ + y + font_->Ascent() * fontScale;
    dl_.EmitTextGlyphs(text, ox_ + x, textY, fontSize, color);
}

} // namespace ui
} // namespace koilo
