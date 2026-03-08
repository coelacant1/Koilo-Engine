// SPDX-License-Identifier: GPL-3.0-or-later
// overlap.hpp
/**
 * @file overlap.hpp
 * @brief Shape-pair overlap helpers for 2D primitives.
 * @date  25/06/2025
 *
 * Provides pairwise intersection tests among Rectangle2D, Circle2D, Ellipse2D, and Triangle2D.
 * Overloads are symmetric via forwarding "inverse" overloads to the primary implementations.
 */

#pragma once

#include "rectangle.hpp"
#include "circle.hpp"
#include "ellipse.hpp"
#include "triangle.hpp"
#include <koilo/registry/reflect_macros.hpp>

namespace koilo {

/**
 * @class Overlap2D
 * @brief Static utilities for testing 2D shape overlaps.
 */
class Overlap2D {
public:
    static bool Overlaps(const Ellipse2D& a, const Ellipse2D& b);
    static bool Overlaps(const Circle2D& a, const Circle2D& b);
    static bool Overlaps(const Rectangle2D& a, const Rectangle2D& b);
    static bool Overlaps(const Triangle2D& a, const Triangle2D& b);
    static bool Overlaps(const Rectangle2D& r, const Circle2D& c);
    static bool Overlaps(const Rectangle2D& r, const Triangle2D& t);
    static bool Overlaps(const Rectangle2D& r, const Ellipse2D& e);
    static bool Overlaps(const Circle2D& c, const Triangle2D& t);
    static bool Overlaps(const Circle2D& c, const Ellipse2D& e);
    static bool Overlaps(const Triangle2D& t, const Ellipse2D& e);

    // Inverse overloads (symmetric)
    static inline bool Overlaps(const Circle2D& c, const Rectangle2D& r) { return Overlaps(r, c); }
    static inline bool Overlaps(const Triangle2D& t, const Rectangle2D& r) { return Overlaps(r, t); }
    static inline bool Overlaps(const Ellipse2D& e, const Rectangle2D& r) { return Overlaps(r, e); }
    static inline bool Overlaps(const Triangle2D& t, const Circle2D& c) { return Overlaps(c, t); }
    static inline bool Overlaps(const Ellipse2D& e, const Circle2D& c) { return Overlaps(c, e); }
    static inline bool Overlaps(const Ellipse2D& e, const Triangle2D& t) { return Overlaps(t, e); }

    KL_BEGIN_FIELDS(Overlap2D)
    KL_END_FIELDS

    KL_BEGIN_METHODS(Overlap2D)
        /* Overlaps */ KL_SMETHOD_OVLD(Overlap2D, Overlaps, bool, const Ellipse2D &, const Ellipse2D &),
        /* Overlaps */ KL_SMETHOD_OVLD(Overlap2D, Overlaps, bool, const Circle2D &, const Circle2D &),
        /* Overlaps */ KL_SMETHOD_OVLD(Overlap2D, Overlaps, bool, const Rectangle2D &, const Rectangle2D &),
        /* Overlaps */ KL_SMETHOD_OVLD(Overlap2D, Overlaps, bool, const Triangle2D &, const Triangle2D &),
        /* Overlaps */ KL_SMETHOD_OVLD(Overlap2D, Overlaps, bool, const Rectangle2D &, const Circle2D &),
        /* Overlaps */ KL_SMETHOD_OVLD(Overlap2D, Overlaps, bool, const Rectangle2D &, const Triangle2D &),
        /* Overlaps */ KL_SMETHOD_OVLD(Overlap2D, Overlaps, bool, const Rectangle2D &, const Ellipse2D &),
        /* Overlaps */ KL_SMETHOD_OVLD(Overlap2D, Overlaps, bool, const Circle2D &, const Triangle2D &),
        /* Overlaps */ KL_SMETHOD_OVLD(Overlap2D, Overlaps, bool, const Circle2D &, const Ellipse2D &),
        /* Overlaps */ KL_SMETHOD_OVLD(Overlap2D, Overlaps, bool, const Triangle2D &, const Ellipse2D &)
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(Overlap2D)
    KL_END_DESCRIBE(Overlap2D)

};

} // namespace koilo
