/**
 * @file overlap.hpp
 * @brief Shape-pair overlap helpers
 * @date  25/06/2025
 */

#pragma once

#include "rectangle.hpp"
#include "circle.hpp"
#include "ellipse.hpp"
#include "triangle.hpp"

class Overlap2D {
public:
    static bool Overlaps(const Ellipse2D& a, const Ellipse2D& b);
    static bool Overlaps(const Circle2D& a, const Circle2D& b);
    static bool Overlaps(const Rectangle2D& a, const Rectangle2D& b);
    static bool Overlaps(const Triangle2D& a, const Triangle2D& b);

    /*
    rc,rt,re
    ct,ce,(cr)
    te,(tr),(tc)
    (et),(ec),(er)
    */

    // First overlap map
    static bool Overlaps(const Rectangle2D& r, const Circle2D& c);
    static bool Overlaps(const Rectangle2D& r, const Triangle2D& t);
    static bool Overlaps(const Rectangle2D& r, const Ellipse2D& e);
    static bool Overlaps(const Circle2D& c, const Triangle2D& t);
    static bool Overlaps(const Circle2D& c, const Ellipse2D& e);
    static bool Overlaps(const Triangle2D& t, const Ellipse2D& e);

    // Inverse of overlaps for full coverage
    static inline bool Overlaps(const Circle2D& c, const Rectangle2D& r) {
        return Overlaps(r, c);
    }

    static inline bool Overlaps(const Triangle2D& t, const Rectangle2D& r) {
        return Overlaps(r, t);
    }

    static inline bool Overlaps(const Ellipse2D& e, const Rectangle2D& r) {
        return Overlaps(r, e);
    }

    static inline bool Overlaps(const Triangle2D& t, const Circle2D& c) {
        return Overlaps(c, t);
    }

    static inline bool Overlaps(const Ellipse2D& e, const Circle2D& c) {
        return Overlaps(c, e);
    }

    static inline bool Overlaps(const Ellipse2D& e, const Triangle2D& t) {
        return Overlaps(t, e);
    }

};
