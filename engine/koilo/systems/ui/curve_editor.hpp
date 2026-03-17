// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file curve_editor.hpp
 * @brief Bezier curve editor widget for animation and easing curves.
 *
 * Uses Canvas2D to render an interactive cubic Bezier curve editor
 * with draggable control points, grid background, and value readout.
 * Suitable for animation curves, easing functions, and color gradients.
 *
 * @date 03/08/2026
 * @author Coela Can't
 */

#pragma once

#include <koilo/systems/ui/ui_context.hpp>
#include <koilo/systems/ui/render/draw_list.hpp>
#include <cmath>
#include <vector>
#include <algorithm>
#include <cstdio>
#include "../../registry/reflect_macros.hpp"

namespace koilo {
namespace ui {

/** @class CurvePoint @brief A single control point on the curve. */
struct CurvePoint {
    float x, y;               ///< Normalized position [0,1]
    float handleInX, handleInY;   ///< Left tangent handle offset
    float handleOutX, handleOutY; ///< Right tangent handle offset
    bool selected = false;        ///< Whether this point is currently selected

    KL_BEGIN_FIELDS(CurvePoint)
        KL_FIELD(CurvePoint, y, "Y", 0, 0)
    KL_END_FIELDS

    KL_BEGIN_METHODS(CurvePoint)
        /* No reflected methods. */
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(CurvePoint)
        /* No reflected ctors. */
    KL_END_DESCRIBE(CurvePoint)

};

/** @class CurveEditor @brief Interactive bezier curve editor rendered on a Canvas2D widget. */
class CurveEditor {
public:
    /**
     * @brief Build the curve editor inside a parent widget.
     * @param ctx     UIContext
     * @param parentIdx  Parent widget pool index
     * @param id      Widget id prefix
     * @param height  Canvas height in pixels
     * @return  The pool index of the created canvas widget
     */
    int Build(UIContext& ctx, int parentIdx, const char* id, float height = 200.0f);

    /** @brief Set the curve to a linear preset. */
    void SetLinear() {
        points_.clear();
        points_.push_back({0.0f, 0.0f, 0.0f, 0.0f, 0.33f, 0.0f});
        points_.push_back({1.0f, 1.0f, -0.33f, 0.0f, 0.0f, 0.0f});
    }

    /** @brief Set the curve to an ease-in-out preset. */
    void SetEaseInOut() {
        points_.clear();
        points_.push_back({0.0f, 0.0f, 0.0f, 0.0f, 0.42f, 0.0f});
        points_.push_back({1.0f, 1.0f, -0.42f, 0.0f, 0.0f, 0.0f});
    }

    /** @brief Set the curve to an ease-in preset. */
    void SetEaseIn() {
        points_.clear();
        points_.push_back({0.0f, 0.0f, 0.0f, 0.0f, 0.42f, 0.0f});
        points_.push_back({1.0f, 1.0f, -0.15f, 0.0f, 0.0f, 0.0f});
    }

    /** @brief Set the curve to an ease-out preset. */
    void SetEaseOut() {
        points_.clear();
        points_.push_back({0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f});
        points_.push_back({1.0f, 1.0f, -0.42f, 0.0f, 0.0f, 0.0f});
    }

    /** @brief Add a control point at normalized (x, y). */
    void AddPoint(float x, float y) {
        CurvePoint pt{x, y, -0.1f, 0.0f, 0.1f, 0.0f, false};
        // Insert sorted by x
        auto it = std::lower_bound(points_.begin(), points_.end(), pt,
            [](const CurvePoint& a, const CurvePoint& b) { return a.x < b.x; });
        points_.insert(it, pt);
    }

    /** @brief Remove the selected point (if not an endpoint). */
    void RemoveSelected();

    /** @brief Evaluate the curve at normalized x ∈ [0,1]. */
    float Evaluate(float x) const;

    /**
     * @brief Handle mouse interaction in canvas-local coordinates.
     * @param localX  Pointer X position relative to canvas
     * @param localY  Pointer Y position relative to canvas
     * @param canvasW  Canvas width
     * @param canvasH  Canvas height
     * @param pressed  Whether pointer is down
     * @param justPressed  True on initial press
     */
    void HandleInput(float localX, float localY, float canvasW, float canvasH,
                     bool pressed, bool justPressed);

    /** @brief Get the list of control points. */
    const std::vector<CurvePoint>& Points() const { return points_; }
    /** @brief Get the pool index of the canvas widget. */
    int CanvasIndex() const { return canvasIdx_; }

private:
    UIContext* ctx_ = nullptr;              ///< Owning UI context
    int canvasIdx_ = -1;                    ///< Pool index of the canvas widget
    std::vector<CurvePoint> points_;        ///< Control points on the curve
    int dragging_ = -1;                     ///< Index of the point being dragged, or -1
    int dragHandle_ = 0;                    ///< Drag target: 0=point, 1=handleOut, -1=handleIn

    static float CubicBezier(float t, float p0, float p1, float p2, float p3) {
        float mt = 1.0f - t;
        return mt * mt * mt * p0
             + 3.0f * mt * mt * t * p1
             + 3.0f * mt * t * t * p2
             + t * t * t * p3;
    }

    void Paint(void* rawCtx);

    KL_BEGIN_FIELDS(CurveEditor)
        /* No reflected fields. */
    KL_END_FIELDS

    KL_BEGIN_METHODS(CurveEditor)
        KL_METHOD_AUTO(CurveEditor, SetEaseInOut, "Set ease in out"),
        KL_METHOD_AUTO(CurveEditor, SetEaseIn, "Set ease in"),
        KL_METHOD_AUTO(CurveEditor, SetEaseOut, "Set ease out"),
        KL_METHOD_AUTO(CurveEditor, CanvasIndex, "Canvas index")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(CurveEditor)
        /* No reflected ctors. */
    KL_END_DESCRIBE(CurveEditor)

};

} // namespace ui
} // namespace koilo
