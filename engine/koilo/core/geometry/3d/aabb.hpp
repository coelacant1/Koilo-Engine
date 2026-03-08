// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file aabb.hpp
 * @brief Axis-Aligned Bounding Box for spatial queries and broad-phase collision.
 *
 * @date 21/02/2026
 * @author Coela Can't
 */

#pragma once

#include <koilo/core/math/vector3d.hpp>
#include <koilo/core/math/mathematics.hpp>
#include <koilo/core/geometry/ray.hpp>
#include <koilo/registry/reflect_macros.hpp>

namespace koilo {

/**
 * @class AABB
 * @brief Axis-aligned bounding box defined by min and max corners.
 *
 * Used for broad-phase collision, frustum culling, spatial partitioning,
 * and bounding volume queries. All faces are aligned to world axes.
 */
class AABB {
public:
    Vector3D min; ///< Minimum corner (smallest X, Y, Z)
    Vector3D max; ///< Maximum corner (largest X, Y, Z)

    /** @brief Default: degenerate box at origin. */
    AABB() : min(0, 0, 0), max(0, 0, 0) {}

    /** @brief Construct from explicit min/max corners. */
    AABB(const Vector3D& min, const Vector3D& max) : min(min), max(max) {}

    /** @brief Returns the center point of the box. */
    Vector3D GetCenter() const {
        return Vector3D(
            (min.X + max.X) * 0.5f,
            (min.Y + max.Y) * 0.5f,
            (min.Z + max.Z) * 0.5f
        );
    }

    /** @brief Returns full extents (width, height, depth). */
    Vector3D GetSize() const {
        return Vector3D(max.X - min.X, max.Y - min.Y, max.Z - min.Z);
    }

    /** @brief Returns half-extents. */
    Vector3D GetHalfSize() const {
        return GetSize() * 0.5f;
    }

    /** @brief Returns the volume of the box. */
    float GetVolume() const {
        Vector3D s = GetSize();
        return s.X * s.Y * s.Z;
    }

    /** @brief Checks if a point is inside (or on the surface of) this AABB. */
    bool Contains(const Vector3D& point) const {
        return point.X >= min.X && point.X <= max.X &&
               point.Y >= min.Y && point.Y <= max.Y &&
               point.Z >= min.Z && point.Z <= max.Z;
    }

    /** @brief Checks if this AABB overlaps with another. */
    bool Overlaps(const AABB& other) const {
        return min.X <= other.max.X && max.X >= other.min.X &&
               min.Y <= other.max.Y && max.Y >= other.min.Y &&
               min.Z <= other.max.Z && max.Z >= other.min.Z;
    }

    /**
     * @brief Ray-AABB intersection test (slab method).
     * @param ray The ray to test against.
     * @param outTMin Output: entry distance (only valid if returns true).
     * @param outTMax Output: exit distance (only valid if returns true).
     * @return True if the ray intersects the AABB.
     */
    bool RayIntersect(const Ray& ray, float& outTMin, float& outTMax) const {
        float tmin = -1e30f, tmax = 1e30f;
        for (int i = 0; i < 3; ++i) {
            float orig = (i == 0) ? ray.origin.X : (i == 1) ? ray.origin.Y : ray.origin.Z;
            float dir  = (i == 0) ? ray.direction.X : (i == 1) ? ray.direction.Y : ray.direction.Z;
            float bmin = (i == 0) ? min.X : (i == 1) ? min.Y : min.Z;
            float bmax = (i == 0) ? max.X : (i == 1) ? max.Y : max.Z;

            if (Mathematics::FAbs(dir) < 1e-8f) {
                if (orig < bmin || orig > bmax) return false;
            } else {
                float invD = 1.0f / dir;
                float t1 = (bmin - orig) * invD;
                float t2 = (bmax - orig) * invD;
                if (t1 > t2) { float tmp = t1; t1 = t2; t2 = tmp; }
                if (t1 > tmin) tmin = t1;
                if (t2 < tmax) tmax = t2;
                if (tmin > tmax) return false;
            }
        }
        if (tmax < 0) return false;
        outTMin = tmin;
        outTMax = tmax;
        return true;
    }

    /** @brief Returns the union of this AABB with another. */
    AABB Union(const AABB& other) const {
        return AABB(
            Vector3D(Mathematics::Min(min.X, other.min.X),
                     Mathematics::Min(min.Y, other.min.Y),
                     Mathematics::Min(min.Z, other.min.Z)),
            Vector3D(Mathematics::Max(max.X, other.max.X),
                     Mathematics::Max(max.Y, other.max.Y),
                     Mathematics::Max(max.Z, other.max.Z))
        );
    }

    /** @brief Expands this AABB to include a point. */
    void Encapsulate(const Vector3D& point) {
        if (point.X < min.X) min.X = point.X;
        if (point.Y < min.Y) min.Y = point.Y;
        if (point.Z < min.Z) min.Z = point.Z;
        if (point.X > max.X) max.X = point.X;
        if (point.Y > max.Y) max.Y = point.Y;
        if (point.Z > max.Z) max.Z = point.Z;
    }

    /** @brief Construct from center + half-extents. */
    static AABB FromCenterHalfSize(const Vector3D& center, const Vector3D& halfSize) {
        return AABB(
            Vector3D(center.X - halfSize.X, center.Y - halfSize.Y, center.Z - halfSize.Z),
            Vector3D(center.X + halfSize.X, center.Y + halfSize.Y, center.Z + halfSize.Z)
        );
    }

    KL_BEGIN_FIELDS(AABB)
        KL_FIELD(AABB, min, "Min", 0, 0),
        KL_FIELD(AABB, max, "Max", 0, 0)
    KL_END_FIELDS

    KL_BEGIN_METHODS(AABB)
        KL_METHOD_AUTO(AABB, GetCenter, "Get center"),
        KL_METHOD_AUTO(AABB, GetSize, "Get size"),
        KL_METHOD_AUTO(AABB, GetHalfSize, "Get half size"),
        KL_METHOD_AUTO(AABB, GetVolume, "Get volume"),
        KL_METHOD_AUTO(AABB, Contains, "Contains"),
        KL_METHOD_AUTO(AABB, Overlaps, "Overlaps"),
        KL_METHOD_AUTO(AABB, Union, "Union"),
        KL_METHOD_AUTO(AABB, Encapsulate, "Encapsulate"),
        KL_SMETHOD_AUTO(AABB::FromCenterHalfSize, "From center half size")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(AABB)
        KL_CTOR0(AABB),
        KL_CTOR(AABB, const Vector3D&, const Vector3D&)
    KL_END_DESCRIBE(AABB)
};

} // namespace koilo
