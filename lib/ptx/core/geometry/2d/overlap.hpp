// overlap.hpp
/**
 * @file overlap.hpp
 * @brief Shape-pair overlap helpers for 2D primitives.
 * @date  25/06/2025
 *
 * Provides pairwise intersection tests among Rectangle2D, Circle2D, Ellipse2D, and Triangle2D.
 * Overloads are symmetric via forwarding “inverse” overloads to the primary implementations.
 */

#pragma once

#include "rectangle.hpp"
#include "circle.hpp"
#include "ellipse.hpp"
#include "triangle.hpp"
#include "../../../registry/reflect_macros.hpp"

/**
 * @class Overlap2D
 * @brief Static utilities for testing 2D shape overlaps.
 *
 * Unless documented otherwise, functions implement a conservative overlap test:
 * - Exact tests are used where available (e.g., AABB for axis-aligned rectangles, SAT for rotated rect-rect).
 * - Some pairs currently fall back to bounding-box checks (marked "Needs finished").
 *
 * All angles are assumed to be in degrees if relied upon by the underlying shapes.
 */
class Overlap2D {
public:
    /**
     * @brief Ellipse-Ellipse overlap test.
     * @param a First ellipse.
     * @param b Second ellipse.
     * @return true if the ellipses overlap or touch; false otherwise.
     * @note Currently falls back to rectangle bounds (see implementation "Needs finished").
     */
    static bool Overlaps(const Ellipse2D& a, const Ellipse2D& b);

    /**
     * @brief Circle-Circle overlap test.
     * @param a First circle.
     * @param b Second circle.
     * @return true if the circles overlap or touch; false otherwise.
     */
    static bool Overlaps(const Circle2D& a, const Circle2D& b);

    /**
     * @brief Rectangle-Rectangle overlap test.
     * @param a First rectangle (may be rotated).
     * @param b Second rectangle (may be rotated).
     * @return true if the rectangles overlap or touch; false otherwise.
     * @details Uses AABB early-out when both rotations ~0; otherwise SAT on rect edge normals.
     */
    static bool Overlaps(const Rectangle2D& a, const Rectangle2D& b);

    /**
     * @brief Triangle-Triangle overlap test.
     * @param a First triangle.
     * @param b Second triangle.
     * @return true if the triangles overlap or touch; false otherwise.
     * @note Currently falls back to rectangle bounds (see implementation "Needs finished").
     */
    static bool Overlaps(const Triangle2D& a, const Triangle2D& b);

    /*
    rc,rt,re
    ct,ce,(cr)
    te,(tr),(tc)
    (et),(ec),(er)
    */

    /**
     * @brief Rectangle-Circle overlap test.
     * @param r Rectangle.
     * @param c Circle.
     * @return true if the shapes overlap or touch; false otherwise.
     * @details Uses closest-point on AABB of the (possibly rotated) rectangle’s bounds.
     */
    static bool Overlaps(const Rectangle2D& r, const Circle2D& c);

    /**
     * @brief Rectangle-Triangle overlap test.
     * @param r Rectangle.
     * @param t Triangle.
     * @return true if the shapes overlap or touch; false otherwise.
     * @note Currently falls back to rectangle bounds (see implementation "Needs finished").
     */
    static bool Overlaps(const Rectangle2D& r, const Triangle2D& t);

    /**
     * @brief Rectangle-Ellipse overlap test.
     * @param r Rectangle.
     * @param e Ellipse.
     * @return true if the shapes overlap or touch; false otherwise.
     * @note Currently falls back to rectangle bounds.
     */
    static bool Overlaps(const Rectangle2D& r, const Ellipse2D& e);

    /**
     * @brief Circle-Triangle overlap test.
     * @param c Circle.
     * @param t Triangle.
     * @return true if the shapes overlap or touch; false otherwise.
     * @note Currently falls back to triangle bounds.
     */
    static bool Overlaps(const Circle2D& c, const Triangle2D& t);

    /**
     * @brief Circle-Ellipse overlap test.
     * @param c Circle.
     * @param e Ellipse.
     * @return true if the shapes overlap or touch; false otherwise.
     * @note Currently falls back to ellipse bounds.
     */
    static bool Overlaps(const Circle2D& c, const Ellipse2D& e);

    /**
     * @brief Triangle-Ellipse overlap test.
     * @param t Triangle.
     * @param e Ellipse.
     * @return true if the shapes overlap or touch; false otherwise.
     * @note Currently falls back to rectangle bounds of both.
     */
    static bool Overlaps(const Triangle2D& t, const Ellipse2D& e);

    // Inverse overloads forward to the canonical implementations (symmetric coverage).
    static inline bool Overlaps(const Circle2D& c, const Rectangle2D& r) { return Overlaps(r, c); }
    static inline bool Overlaps(const Triangle2D& t, const Rectangle2D& r) { return Overlaps(r, t); }
    static inline bool Overlaps(const Ellipse2D& e, const Rectangle2D& r) { return Overlaps(r, e); }
    static inline bool Overlaps(const Triangle2D& t, const Circle2D& c) { return Overlaps(c, t); }
    static inline bool Overlaps(const Ellipse2D& e, const Circle2D& c) { return Overlaps(c, e); }
    static inline bool Overlaps(const Ellipse2D& e, const Triangle2D& t) { return Overlaps(t, e); }

    PTX_BEGIN_FIELDS(Overlap2D)
        /* TODO: PTX_FIELD(Overlap2D, member, "Doc", min, max) */
    PTX_END_FIELDS
    
    PTX_BEGIN_METHODS(Overlap2D)
        PTX_SMETHOD_AUTO(Overlap2D::Overlaps, "Overlaps"),
        PTX_SMETHOD_AUTO(Overlap2D::Overlaps, "Overlaps"),
        PTX_SMETHOD_AUTO(Overlap2D::Overlaps, "Overlaps"),
        PTX_SMETHOD_AUTO(Overlap2D::Overlaps, "Overlaps"),
        PTX_SMETHOD_AUTO(Overlap2D::Overlaps, "Overlaps"),
        PTX_SMETHOD_AUTO(Overlap2D::Overlaps, "Overlaps"),
        PTX_SMETHOD_AUTO(Overlap2D::Overlaps, "Overlaps"),
        PTX_SMETHOD_AUTO(Overlap2D::Overlaps, "Overlaps"),
        PTX_SMETHOD_AUTO(Overlap2D::Overlaps, "Overlaps"),
        PTX_SMETHOD_AUTO(Overlap2D::Overlaps, "Overlaps"),
        PTX_SMETHOD_AUTO(Overlap2D::Overlaps, "Overlaps"),
        PTX_SMETHOD_AUTO(Overlap2D::Overlaps, "Overlaps"),
        PTX_SMETHOD_AUTO(Overlap2D::Overlaps, "Overlaps"),
        PTX_SMETHOD_AUTO(Overlap2D::Overlaps, "Overlaps"),
        PTX_SMETHOD_AUTO(Overlap2D::Overlaps, "Overlaps"),
        PTX_SMETHOD_AUTO(Overlap2D::Overlaps, "Overlaps")
    PTX_END_METHODS
    
    PTX_BEGIN_DESCRIBE(Overlap2D)
        /* TODO: PTX_CTOR0(Overlap2D) or PTX_CTOR(Overlap2D, ...) */
    PTX_END_DESCRIBE(Overlap2D)
    
};
