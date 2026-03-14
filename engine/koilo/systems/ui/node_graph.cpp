// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file node_graph.cpp
 * @brief Node graph Build() and Paint() implementation.
 *
 * @date 03/08/2026
 * @author Coela Can't
 */

#include "node_graph.hpp"
#include "ui_context.hpp"
#include "render/draw_list.hpp"

namespace koilo {
namespace ui {

// ============================================================================
// Canvas construction
// ============================================================================

// Build the canvas widget and register paint callback
int NodeGraph::Build(UIContext& ctx, int parentIdx, const char* id,
                     float width, float height) {
    canvasIdx_ = ctx.CreateCanvas2D(id,
        [this](void* vctx) { Paint(vctx); });
    if (canvasIdx_ < 0) return -1;
    ctx.SetParent(canvasIdx_, parentIdx);
    Widget* w = ctx.Pool().Get(canvasIdx_);
    w->widthMode = SizeMode::Fixed;
    w->localW = width;
    w->heightMode = SizeMode::Fixed;
    w->localH = height;
    return canvasIdx_;
}

// ============================================================================
// Rendering
// ============================================================================

// Paint all graph elements onto the canvas
void NodeGraph::Paint(void* vctx) const {
    auto* ctx = static_cast<CanvasDrawContext*>(vctx);
    float cw = ctx->Width();
    float ch = ctx->Height();

    // Background
    ctx->DrawRect(0, 0, cw, ch, {30, 30, 35, 255});

    // Grid
    float gridSize = 20.0f;
    Color4 gridColor{45, 45, 50, 255};
    float gxOff = std::fmod(panX_, gridSize);
    if (gxOff < 0) gxOff += gridSize;
    float gyOff = std::fmod(panY_, gridSize);
    if (gyOff < 0) gyOff += gridSize;
    for (float gx = gxOff; gx < cw; gx += gridSize) {
        ctx->DrawLine(gx, 0, gx, ch, 1.0f, gridColor);
    }
    for (float gy = gyOff; gy < ch; gy += gridSize) {
        ctx->DrawLine(0, gy, cw, gy, 1.0f, gridColor);
    }

    // Connections (bezier curves as line segments)
    for (const auto& conn : connections_) {
        const GraphNode* sn = FindNode(conn.srcNode);
        const GraphNode* dn = FindNode(conn.dstNode);
        if (!sn || !dn) continue;

        float sx, sy, dx, dy;
        OutputPortPos(*sn, conn.srcPort, sx, sy);
        InputPortPos(*dn, conn.dstPort, dx, dy);
        sx += panX_; sy += panY_;
        dx += panX_; dy += panY_;

        Color4 wireColor = PortColor(sn->outputs[conn.srcPort].type);

        // Cubic bezier: horizontal control points
        float cpDist = std::fabs(dx - sx) * 0.5f;
        if (cpDist < 30.0f) cpDist = 30.0f;
        DrawBezier(*ctx, sx, sy, sx + cpDist, sy, dx - cpDist, dy, dx, dy, 2.0f, wireColor);
    }

    // Dragging wire
    if (draggingWire_) {
        const GraphNode* sn = FindNode(wireSrcNode_);
        if (sn && wireSrcPort_ >= 0 && wireSrcPort_ < static_cast<int>(sn->outputs.size())) {
            float sx, sy;
            OutputPortPos(*sn, wireSrcPort_, sx, sy);
            sx += panX_; sy += panY_;
            Color4 wireColor = PortColor(sn->outputs[wireSrcPort_].type);
            wireColor.a = 180;
            float cpDist = std::fabs(wireEndX_ - sx) * 0.5f;
            if (cpDist < 30.0f) cpDist = 30.0f;
            DrawBezier(*ctx, sx, sy, sx + cpDist, sy,
                       wireEndX_ - cpDist, wireEndY_, wireEndX_, wireEndY_,
                       2.0f, wireColor);
        }
    }

    // Nodes
    for (const auto& node : nodes_) {
        float nx = node.x + panX_;
        float ny = node.y + panY_;
        float nw = node.w;
        float nh = node.h;

        // Node body
        ctx->DrawRoundedRect(nx, ny, nw, nh, 4.0f, node.color);
        if (node.selected) {
            ctx->DrawBorderRect(nx - 1, ny - 1, nw + 2, nh + 2,
                                {100, 180, 255, 200}, 2.0f);
        }

        // Title bar
        ctx->DrawRoundedRect(nx, ny, nw, GraphNode::TITLE_H, 4.0f, node.titleColor);
        ctx->DrawText(nx + 6, ny + 4, node.title.c_str(), {230, 230, 240, 255}, 12.0f);

        // Input ports
        for (int i = 0; i < static_cast<int>(node.inputs.size()); ++i) {
            float px, py;
            InputPortPos(node, i, px, py);
            px += panX_; py += panY_;
            Color4 pc = PortColor(node.inputs[i].type);
            ctx->DrawCircle(px, py, GraphNode::PORT_RADIUS, pc);
            ctx->DrawText(px + 8, py - 6, node.inputs[i].name.c_str(), {200, 200, 210, 255}, 11.0f);
        }

        // Output ports
        for (int i = 0; i < static_cast<int>(node.outputs.size()); ++i) {
            float px, py;
            OutputPortPos(node, i, px, py);
            px += panX_; py += panY_;
            Color4 pc = PortColor(node.outputs[i].type);
            ctx->DrawCircle(px, py, GraphNode::PORT_RADIUS, pc);
            // Right-align text
            float tw = static_cast<float>(node.outputs[i].name.size()) * 7.0f;
            ctx->DrawText(px - tw - 8, py - 6, node.outputs[i].name.c_str(), {200, 200, 210, 255}, 11.0f);
        }
    }
}

// Draw a cubic bezier curve as line segments
void NodeGraph::DrawBezier(CanvasDrawContext& ctx,
                           float x0, float y0, float cx0, float cy0,
                           float cx1, float cy1, float x1, float y1,
                           float width, Color4 color) const {
    constexpr int SEGMENTS = 20;
    float prevX = x0, prevY = y0;
    for (int i = 1; i <= SEGMENTS; ++i) {
        float t = static_cast<float>(i) / SEGMENTS;
        float u = 1.0f - t;
        float bx = u*u*u*x0 + 3*u*u*t*cx0 + 3*u*t*t*cx1 + t*t*t*x1;
        float by = u*u*u*y0 + 3*u*u*t*cy0 + 3*u*t*t*cy1 + t*t*t*y1;
        ctx.DrawLine(prevX, prevY, bx, by, width, color);
        prevX = bx;
        prevY = by;
    }
}

} // namespace ui
} // namespace koilo
