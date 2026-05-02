// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file obb.hpp
 * @brief Oriented Bounding Box.
 *
 * SAT overlap (OBB-OBB and OBB-AABB), ray test, closest-point.
 */

#pragma once

#include <koilo/core/math/vector3d.hpp>
#include <koilo/core/math/quaternion.hpp>
#include <koilo/core/math/matrix3x3.hpp>
#include <koilo/core/geometry/3d/aabb.hpp>
#include <koilo/core/geometry/ray.hpp>
#include <koilo/registry/reflect_macros.hpp>
#include <cmath>

namespace koilo {

/**
 * @class OBB
 * @brief Oriented bounding box: center + half-extents + orientation.
 *
 * Axes are derived from orientation (rotation matrix columns).
 */
class OBB {
public:
    Vector3D center;       ///< World-space center
    Vector3D halfExtents;  ///< Positive half-sizes along local x/y/z
    Quaternion orientation;///< Rotation from local to world

    OBB() : center(0,0,0), halfExtents(0.5f,0.5f,0.5f), orientation() {}
    OBB(const Vector3D& c, const Vector3D& h) : center(c), halfExtents(h), orientation() {}
    OBB(const Vector3D& c, const Vector3D& h, const Quaternion& q)
        : center(c), halfExtents(h), orientation(q) {}

    /** Local x axis in world space. */
    Vector3D AxisX() const { return orientation.RotateVector(Vector3D(1,0,0)); }
    Vector3D AxisY() const { return orientation.RotateVector(Vector3D(0,1,0)); }
    Vector3D AxisZ() const { return orientation.RotateVector(Vector3D(0,0,1)); }

    /** World-space AABB enclosing the OBB. */
    AABB EnclosingAABB() const {
        const Matrix3x3 R = Matrix3x3::FromQuaternion(orientation);
        Vector3D ext;
        ext.X = std::abs(R.M[0][0])*halfExtents.X + std::abs(R.M[0][1])*halfExtents.Y + std::abs(R.M[0][2])*halfExtents.Z;
        ext.Y = std::abs(R.M[1][0])*halfExtents.X + std::abs(R.M[1][1])*halfExtents.Y + std::abs(R.M[1][2])*halfExtents.Z;
        ext.Z = std::abs(R.M[2][0])*halfExtents.X + std::abs(R.M[2][1])*halfExtents.Y + std::abs(R.M[2][2])*halfExtents.Z;
        return AABB(center - ext, center + ext);
    }

    /** Closest point on the OBB to p (returns p if inside). */
    Vector3D ClosestPoint(const Vector3D& p) const {
        const Vector3D d = p - center;
        const Vector3D ax = AxisX(), ay = AxisY(), az = AxisZ();
        float dx = d.DotProduct(ax);
        float dy = d.DotProduct(ay);
        float dz = d.DotProduct(az);
        if (dx >  halfExtents.X) dx =  halfExtents.X;
        if (dx < -halfExtents.X) dx = -halfExtents.X;
        if (dy >  halfExtents.Y) dy =  halfExtents.Y;
        if (dy < -halfExtents.Y) dy = -halfExtents.Y;
        if (dz >  halfExtents.Z) dz =  halfExtents.Z;
        if (dz < -halfExtents.Z) dz = -halfExtents.Z;
        return center + ax*dx + ay*dy + az*dz;
    }

    bool ContainsPoint(const Vector3D& p) const {
        const Vector3D d = p - center;
        const float dx = d.DotProduct(AxisX());
        const float dy = d.DotProduct(AxisY());
        const float dz = d.DotProduct(AxisZ());
        return std::abs(dx) <= halfExtents.X &&
               std::abs(dy) <= halfExtents.Y &&
               std::abs(dz) <= halfExtents.Z;
    }

    /**
     * @struct SeparationInfo
     * @brief Rich SAT result: which axis (if any) gave minimum overlap, the
     * penetration depth, and the world-space axis direction.
     *
     * - When `intersect` is true, `axisIndex` selects the axis with the
     *   smallest overlap; `penetration` is that overlap (positive); `axis` is
     *   that direction normalized and signed so that
     *   `axis.DotProduct(other.center - this.center) >= 0`.
     * - When `intersect` is false, `axisIndex` is the first axis that proved
     *   separation, `penetration` is negative (separation gap), and `axis`
     *   carries that axis (sign convention as above).
     *
     * Axis index encoding:
     *   0..2  : this OBB face axes (X, Y, Z)
     *   3..5  : `other` OBB face axes (X, Y, Z)
     *   6..14 : edge-edge cross products: 6 + i*3 + j (i = this axis, j = other axis)
     */
    struct SeparationInfo {
        bool         intersect   = false;
        int          axisIndex   = -1;
        float        penetration = 0.0f;
        Vector3D     axis        = Vector3D(0, 0, 0);
    };

    /**
     * @brief Full 15-axis SAT producing the rich separation info.
     * @param other Other OBB to test against.
     * @param faceBias Added to edge-edge overlaps before "minimum overlap"
     * selection; pushes ties toward face-face contacts (use `1e-3f`-ish for
     * physics manifold generation; default 0 for pure geometry queries).
     * Affects selection only - the returned `penetration` is unbiased.
     */
    SeparationInfo ComputeSeparation(const OBB& other, float faceBias = 0.0f) const {
        SeparationInfo result;
        const Vector3D A[3] = { AxisX(), AxisY(), AxisZ() };
        const Vector3D B[3] = { other.AxisX(), other.AxisY(), other.AxisZ() };
        const float a[3] = { halfExtents.X, halfExtents.Y, halfExtents.Z };
        const float b[3] = { other.halfExtents.X, other.halfExtents.Y, other.halfExtents.Z };

        // Rotation matrix expressing B in A's frame, plus epsilon to handle parallel axes.
        float R[3][3], AbsR[3][3];
        for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 3; ++j) {
                R[i][j]    = A[i].DotProduct(B[j]);
                AbsR[i][j] = std::abs(R[i][j]) + 1e-6f;
            }

        const Vector3D t_world = other.center - center;
        const float t[3] = { t_world.DotProduct(A[0]), t_world.DotProduct(A[1]), t_world.DotProduct(A[2]) };

        float bestOverlap = 1e30f;
        int   bestAxis    = -1;
        Vector3D bestDir(0, 0, 0);

        auto consider = [&](float overlap, int axisIdx, const Vector3D& dirWorld, float bias) {
            // Selection picks the smallest overlap. To bias selection AWAY
            // from an axis, ADD the bias before comparing - the unbiased
            // overlap is restored on the chosen axis below.
            const float adj = overlap + bias;
            if (adj < bestOverlap) {
                bestOverlap = adj;
                bestAxis    = axisIdx;
                // Sign so axis points from this->other.
                const float s = dirWorld.DotProduct(t_world);
                bestDir = (s >= 0.0f) ? dirWorld : (dirWorld * -1.0f);
            }
        };

        // 3 face axes of this.
        for (int i = 0; i < 3; ++i) {
            const float ra = a[i];
            const float rb = b[0]*AbsR[i][0] + b[1]*AbsR[i][1] + b[2]*AbsR[i][2];
            const float overlap = (ra + rb) - std::abs(t[i]);
            if (overlap < 0.0f) {
                result.intersect   = false;
                result.axisIndex   = i;
                result.penetration = overlap;
                result.axis        = (t[i] >= 0.0f) ? A[i] : (A[i] * -1.0f);
                return result;
            }
            consider(overlap, i, A[i], 0.0f);
        }
        // 3 face axes of other.
        for (int j = 0; j < 3; ++j) {
            const float ra = a[0]*AbsR[0][j] + a[1]*AbsR[1][j] + a[2]*AbsR[2][j];
            const float rb = b[j];
            const float tj = t[0]*R[0][j] + t[1]*R[1][j] + t[2]*R[2][j];
            const float overlap = (ra + rb) - std::abs(tj);
            if (overlap < 0.0f) {
                result.intersect   = false;
                result.axisIndex   = 3 + j;
                result.penetration = overlap;
                result.axis        = (tj >= 0.0f) ? B[j] : (B[j] * -1.0f);
                return result;
            }
            consider(overlap, 3 + j, B[j], 0.0f);
        }
        // 9 edge-edge cross products.
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j) {
                const int i1 = (i+1)%3, i2 = (i+2)%3;
                const int j1 = (j+1)%3, j2 = (j+2)%3;
                const float ra = a[i1]*AbsR[i2][j] + a[i2]*AbsR[i1][j];
                const float rb = b[j1]*AbsR[i][j2] + b[j2]*AbsR[i][j1];
                const float tij = t[i2]*R[i1][j] - t[i1]*R[i2][j];
                const float overlap = (ra + rb) - std::abs(tij);
                if (overlap < 0.0f) {
                    result.intersect   = false;
                    result.axisIndex   = 6 + i*3 + j;
                    result.penetration = overlap;
                    Vector3D cross = A[i].CrossProduct(B[j]);
                    const float len = cross.Magnitude();
                    if (len > 1e-6f) cross = cross * (1.0f / len);
                    result.axis = (tij >= 0.0f) ? cross : (cross * -1.0f);
                    return result;
                }
                // Skip degenerate (parallel) edge axes for selection.
                Vector3D cross = A[i].CrossProduct(B[j]);
                const float len = cross.Magnitude();
                if (len < 1e-6f) continue;
                cross = cross * (1.0f / len);
                consider(overlap, 6 + i*3 + j, cross, faceBias);
            }
        }

        result.intersect   = true;
        result.axisIndex   = bestAxis;
        // Restore unbiased overlap for edge-axis case (face axes have bias=0).
        result.penetration = bestOverlap - ((bestAxis >= 6) ? faceBias : 0.0f);
        result.axis        = bestDir;
        return result;
    }

    /** SAT overlap with another OBB (15 axes). */
    bool Overlaps(const OBB& o) const {
        return ComputeSeparation(o).intersect;
    }

    /** SAT overlap with an AABB. */
    bool Overlaps(const AABB& a) const {
        OBB asObb;
        asObb.center = a.GetCenter();
        asObb.halfExtents = a.GetHalfSize();
        // identity orientation
        return Overlaps(asObb);
    }

    /**
     * @brief Ray vs OBB (transforms ray into local space, runs slab test).
     * @return True if hit; outDistance is t along the ray.
     */
    bool Raycast(const Ray& ray, float& outDistance, float maxDistance = 1e30f) const {
        // Transform ray into OBB local space
        const Vector3D originLocal = orientation.UnrotateVector(ray.origin - center);
        const Vector3D dirLocal    = orientation.UnrotateVector(ray.direction);

        float tmin = 0.0f;
        float tmax = maxDistance;
        const float ext[3] = { halfExtents.X, halfExtents.Y, halfExtents.Z };
        const float o[3]   = { originLocal.X, originLocal.Y, originLocal.Z };
        const float d[3]   = { dirLocal.X, dirLocal.Y, dirLocal.Z };

        for (int i = 0; i < 3; ++i) {
            if (std::abs(d[i]) < 1e-8f) {
                if (o[i] < -ext[i] || o[i] > ext[i]) return false;
            } else {
                const float invD = 1.0f / d[i];
                float t1 = (-ext[i] - o[i]) * invD;
                float t2 = ( ext[i] - o[i]) * invD;
                if (t1 > t2) { float tmp = t1; t1 = t2; t2 = tmp; }
                if (t1 > tmin) tmin = t1;
                if (t2 < tmax) tmax = t2;
                if (tmin > tmax) return false;
            }
        }
        outDistance = tmin;
        return true;
    }

    KL_BEGIN_FIELDS(OBB)
        KL_FIELD(OBB, center, "Center", 0, 0),
        KL_FIELD(OBB, halfExtents, "Half extents", 0, 0),
        KL_FIELD(OBB, orientation, "Orientation", 0, 0)
    KL_END_FIELDS

    KL_BEGIN_METHODS(OBB)
        KL_METHOD_AUTO(OBB, EnclosingAABB, "Enclosing AABB"),
        KL_METHOD_AUTO(OBB, ClosestPoint, "Closest point"),
        KL_METHOD_AUTO(OBB, ContainsPoint, "Contains point")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(OBB)
        KL_CTOR0(OBB),
        KL_CTOR(OBB, Vector3D, Vector3D)
    KL_END_DESCRIBE(OBB)
};

} // namespace koilo
