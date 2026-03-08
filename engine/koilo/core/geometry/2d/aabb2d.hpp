// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file aabb2d.hpp
 * @brief 2D Axis-Aligned Bounding Box - lightweight, no inheritance, no rotation.
 *
 * Mirrors the API of the 3D AABB class for consistent geometry API design.
 *
 * @date 21/02/2026
 * @author Coela Can't
 */

#pragma once

#include <koilo/core/math/vector2d.hpp>
#include <koilo/registry/reflect_macros.hpp>
#include <algorithm>

namespace koilo {

/**
 * @class AABB2D
 * @brief Axis-aligned 2D bounding box defined by min and max corners.
 *
 * Used for broad-phase 2D overlap, UI hit-testing, spatial partitioning,
 * and any case where rotation is not needed.
 */
class AABB2D {
public:
    Vector2D min; ///< Minimum corner (smallest X, Y)
    Vector2D max; ///< Maximum corner (largest X, Y)

    /** @brief Default: degenerate box at origin. */
    AABB2D() : min(0, 0), max(0, 0) {}

    /** @brief Construct from explicit min/max corners. */
    AABB2D(const Vector2D& min, const Vector2D& max) : min(min), max(max) {}

    /** @brief Returns the center point of the box. */
    Vector2D GetCenter() const {
        return Vector2D((min.X + max.X) * 0.5f, (min.Y + max.Y) * 0.5f);
    }

    /** @brief Returns full size (width, height). */
    Vector2D GetSize() const {
        return Vector2D(max.X - min.X, max.Y - min.Y);
    }

    /** @brief Returns half-size. */
    Vector2D GetHalfSize() const {
        return Vector2D((max.X - min.X) * 0.5f, (max.Y - min.Y) * 0.5f);
    }

    /** @brief Returns the area of the box. */
    float GetArea() const {
        return (max.X - min.X) * (max.Y - min.Y);
    }

    /** @brief Checks if a point is inside (or on edge of) this AABB. */
    bool Contains(const Vector2D& point) const {
        return point.X >= min.X && point.X <= max.X &&
               point.Y >= min.Y && point.Y <= max.Y;
    }

    /** @brief Checks if this AABB overlaps with another. */
    bool Overlaps(const AABB2D& other) const {
        return min.X <= other.max.X && max.X >= other.min.X &&
               min.Y <= other.max.Y && max.Y >= other.min.Y;
    }

    /** @brief Returns the union of this AABB with another. */
    AABB2D Union(const AABB2D& other) const {
        return AABB2D(
            Vector2D(std::min(min.X, other.min.X), std::min(min.Y, other.min.Y)),
            Vector2D(std::max(max.X, other.max.X), std::max(max.Y, other.max.Y))
        );
    }

    /** @brief Expands this AABB to include a point. */
    void Encapsulate(const Vector2D& point) {
        if (point.X < min.X) min.X = point.X;
        if (point.Y < min.Y) min.Y = point.Y;
        if (point.X > max.X) max.X = point.X;
        if (point.Y > max.Y) max.Y = point.Y;
    }

    /** @brief Construct from center + half-size. */
    static AABB2D FromCenterHalfSize(const Vector2D& center, const Vector2D& halfSize) {
        return AABB2D(
            Vector2D(center.X - halfSize.X, center.Y - halfSize.Y),
            Vector2D(center.X + halfSize.X, center.Y + halfSize.Y)
        );
    }

    /** @brief Returns the closest point on this box to a given point. */
    Vector2D ClosestPoint(const Vector2D& point) const {
        return Vector2D(
            std::max(min.X, std::min(point.X, max.X)),
            std::max(min.Y, std::min(point.Y, max.Y))
        );
    }

    KL_BEGIN_FIELDS(AABB2D)
        KL_FIELD(AABB2D, min, "Min", 0, 0),
        KL_FIELD(AABB2D, max, "Max", 0, 0)
    KL_END_FIELDS

    KL_BEGIN_METHODS(AABB2D)
        KL_METHOD_AUTO(AABB2D, GetCenter, "Get center"),
        KL_METHOD_AUTO(AABB2D, GetSize, "Get size"),
        KL_METHOD_AUTO(AABB2D, GetHalfSize, "Get half size"),
        KL_METHOD_AUTO(AABB2D, GetArea, "Get area"),
        KL_METHOD_AUTO(AABB2D, Contains, "Contains"),
        KL_METHOD_AUTO(AABB2D, Encapsulate, "Encapsulate"),
        KL_METHOD_AUTO(AABB2D, ClosestPoint, "Closest point"),
        KL_SMETHOD_AUTO(AABB2D::FromCenterHalfSize, "From center half size")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(AABB2D)
        KL_CTOR0(AABB2D),
        KL_CTOR(AABB2D, const Vector2D&, const Vector2D&)
    KL_END_DESCRIBE(AABB2D)
};

} // namespace koilo
