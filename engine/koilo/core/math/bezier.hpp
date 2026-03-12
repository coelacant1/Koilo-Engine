// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file bezier.hpp
 * @brief Quadratic and cubic Bézier curve primitives.
 *
 * General-purpose Bézier evaluation, subdivision, flattening, and bounding
 * boxes. Used by the font rasterizer for glyph outlines and available for
 * path/spline tools throughout the engine.
 *
 * @date 03/08/2026
 * @author Coela Can't
 */

#pragma once

#include <koilo/core/math/vector2d.hpp>
#include <vector>
#include "../../registry/reflect_macros.hpp"

namespace koilo {

// --- Quadratic Bézier (TTF glyph curves) ----------------------------

struct QuadBezier {
    Vector2D p0;  // start (on-curve)
    Vector2D p1;  // control (off-curve)
    Vector2D p2;  // end (on-curve)

    /// Evaluate at parameter t ∈ [0,1] using De Casteljau.
    Vector2D Evaluate(float t) const;

    /// First derivative at t.
    Vector2D Derivative(float t) const;

    /// Split at parameter t -> two sub-curves.
    void Split(float t, QuadBezier& left, QuadBezier& right) const;

    /// Axis-aligned bounding box.
    void BoundingBox(Vector2D& outMin, Vector2D& outMax) const;

    /// Adaptive subdivision to line segments within tolerance.
    void Flatten(std::vector<Vector2D>& out, float tolerance = 0.5f) const;

    KL_BEGIN_FIELDS(QuadBezier)
        KL_FIELD(QuadBezier, p0, "P0", 0, 0)
    KL_END_FIELDS

    KL_BEGIN_METHODS(QuadBezier)
        /* No reflected methods. */
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(QuadBezier)
        /* No reflected ctors. */
    KL_END_DESCRIBE(QuadBezier)

};

// --- Cubic Bézier (OTF/CFF, general paths) --------------------------

struct CubicBezier {
    Vector2D p0;  // start
    Vector2D p1;  // control 1
    Vector2D p2;  // control 2
    Vector2D p3;  // end

    /// Evaluate at parameter t ∈ [0,1].
    Vector2D Evaluate(float t) const;

    /// First derivative at t.
    Vector2D Derivative(float t) const;

    /// Split at parameter t -> two sub-curves (De Casteljau).
    void Split(float t, CubicBezier& left, CubicBezier& right) const;

    /// Axis-aligned bounding box.
    void BoundingBox(Vector2D& outMin, Vector2D& outMax) const;

    /// Adaptive subdivision to line segments.
    void Flatten(std::vector<Vector2D>& out, float tolerance = 0.5f) const;

    KL_BEGIN_FIELDS(CubicBezier)
        KL_FIELD(CubicBezier, p0, "P0", 0, 0)
    KL_END_FIELDS

    KL_BEGIN_METHODS(CubicBezier)
        /* No reflected methods. */
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(CubicBezier)
        /* No reflected ctors. */
    KL_END_DESCRIBE(CubicBezier)

};

} // namespace koilo
