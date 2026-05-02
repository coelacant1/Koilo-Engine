// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file inertiatensor.hpp
 * @brief Closed-form inertia tensors for common shapes.
 *
 * All return tensors expressed in the shape's local frame, principal axes
 * aligned with x/y/z (so they are diagonal). Capsule/cylinder long axis is
 * local +Y to match `CapsuleShape`. Hull is approximated by its AABB -
 * full inertia integration is reserved for a later utility.
 */

#pragma once

#include <koilo/core/math/matrix3x3.hpp>
#include <koilo/core/math/vector3d.hpp>
#include <koilo/core/geometry/3d/convexhull.hpp>
#include <cmath>

namespace koilo::InertiaTensor {

/** Solid sphere: I = (2/5) m r². */
inline Matrix3x3 SolidSphere(float mass, float radius) {
    const float i = 0.4f * mass * radius * radius;
    return Matrix3x3::Diagonal(Vector3D(i, i, i));
}

/**
 * @brief Solid box of full extents (2*halfExtents). Standard formula.
 *
 * I_xx = (1/12) m (h² + d²) where h, d are full edge lengths.
 * With half-extents (hx, hy, hz): I_xx = (1/3) m (hy² + hz²).
 */
inline Matrix3x3 SolidBox(float mass, const Vector3D& halfExtents) {
    const float hx2 = halfExtents.X * halfExtents.X;
    const float hy2 = halfExtents.Y * halfExtents.Y;
    const float hz2 = halfExtents.Z * halfExtents.Z;
    const float k = mass / 3.0f;
    return Matrix3x3::Diagonal(Vector3D(k*(hy2+hz2), k*(hx2+hz2), k*(hx2+hy2)));
}

/**
 * @brief Solid cylinder along local +Y. Length = 2 * halfHeight.
 *
 * I_yy = (1/2) m r²
 * I_xx = I_zz = (1/12) m (3r² + (2h)²) = (1/12) m (3r² + 4h²)
 */
inline Matrix3x3 SolidCylinder(float mass, float radius, float halfHeight) {
    const float r2 = radius * radius;
    const float h2 = halfHeight * halfHeight;
    const float Iyy = 0.5f * mass * r2;
    const float Iperp = (1.0f / 12.0f) * mass * (3.0f * r2 + 4.0f * h2);
    return Matrix3x3::Diagonal(Vector3D(Iperp, Iyy, Iperp));
}

/**
 * @brief Solid capsule along local +Y (cylinder + 2 hemispheres).
 *
 * Mass split by volume ratio. Closed-form composite using parallel-axis
 * theorem for the hemispheres.
 */
inline Matrix3x3 SolidCapsule(float mass, float radius, float halfHeight) {
    const float r = radius;
    const float h = 2.0f * halfHeight;
    const float r3 = r * r * r;
    const float vCyl = 3.14159265358979f * r * r * h;
    const float vHemi = (2.0f / 3.0f) * 3.14159265358979f * r3; // each hemisphere
    const float vTotal = vCyl + 2.0f * vHemi;
    if (vTotal < 1e-12f) return Matrix3x3::Diagonal(Vector3D(0,0,0));

    const float mCyl  = mass * (vCyl / vTotal);
    const float mHemi = mass * (vHemi / vTotal);

    // Cylinder portion (axis = Y).
    const float Iyy_cyl = 0.5f * mCyl * r * r;
    const float Iperp_cyl = (1.0f / 12.0f) * mCyl * (3.0f * r * r + h * h);

    // Hemisphere about its own COM (axis through pole = Y for top, -Y for bottom):
    //   Iyy_hemi = (2/5) m r²
    // Perpendicular through hemisphere COM:
    //   Iperp_hemi_centroid = (83/320) m r²
    // Hemisphere COM is offset 3r/8 from flat face along Y; the flat face sits at ±h/2.
    // So COM offset from capsule center: ±(h/2 + 3r/8).
    const float Iyy_hemi = (2.0f / 5.0f) * mHemi * r * r;
    const float IperpC_hemi = (83.0f / 320.0f) * mHemi * r * r;
    const float d = 0.5f * h + 0.375f * r; // distance to capsule center
    const float Iperp_hemi = IperpC_hemi + mHemi * d * d;

    const float Iyy = Iyy_cyl + 2.0f * Iyy_hemi;
    const float Iperp = Iperp_cyl + 2.0f * Iperp_hemi;
    return Matrix3x3::Diagonal(Vector3D(Iperp, Iyy, Iperp));
}

/**
 * @brief Approximate hull inertia using its bounding box.
 *
 * Acceptable Phase-3 placeholder. A future utility can compute true volume
 * inertia via tetrahedron decomposition.
 */
inline Matrix3x3 SolidHull(float mass, const ConvexHull& hull) {
    const AABB b = hull.ComputeBounds();
    const Vector3D halfExtents(
        0.5f * (b.max.X - b.min.X),
        0.5f * (b.max.Y - b.min.Y),
        0.5f * (b.max.Z - b.min.Z)
    );
    return SolidBox(mass, halfExtents);
}

/** Returns the inverse of a diagonal inertia tensor, treating zeros as infinite. */
inline Matrix3x3 Invert(const Matrix3x3& I) {
    // Fast path for diagonal tensors (the common case).
    if (I.M[0][1]==0 && I.M[0][2]==0 && I.M[1][0]==0 &&
        I.M[1][2]==0 && I.M[2][0]==0 && I.M[2][1]==0) {
        Matrix3x3 r = Matrix3x3::Diagonal(Vector3D(0,0,0));
        if (I.M[0][0] > 1e-12f) r.M[0][0] = 1.0f / I.M[0][0];
        if (I.M[1][1] > 1e-12f) r.M[1][1] = 1.0f / I.M[1][1];
        if (I.M[2][2] > 1e-12f) r.M[2][2] = 1.0f / I.M[2][2];
        return r;
    }
    return I.Inverse();
}

} // namespace koilo::InertiaTensor
