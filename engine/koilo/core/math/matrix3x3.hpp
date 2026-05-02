// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file matrix3x3.hpp
 * @brief 3x3 matrix for rotations and inertia tensors.
 *
 * Row-major storage. Used by physics for inertia tensors and as a
 * lightweight rotation representation alongside Quaternion.
 *
 * @date 24/04/2026
 */

#pragma once

#include "vector3d.hpp"
#include "quaternion.hpp"
#include "mathematics.hpp"
#include <koilo/registry/reflect_macros.hpp>
#include <cmath>

namespace koilo {

/**
 * @class Matrix3x3
 * @brief 3x3 matrix in row-major storage. M[row][col].
 */
class Matrix3x3 {
public:
    float M[3][3];

    /** Identity. */
    Matrix3x3() {
        M[0][0]=1; M[0][1]=0; M[0][2]=0;
        M[1][0]=0; M[1][1]=1; M[1][2]=0;
        M[2][0]=0; M[2][1]=0; M[2][2]=1;
    }

    Matrix3x3(float m00, float m01, float m02,
              float m10, float m11, float m12,
              float m20, float m21, float m22) {
        M[0][0]=m00; M[0][1]=m01; M[0][2]=m02;
        M[1][0]=m10; M[1][1]=m11; M[1][2]=m12;
        M[2][0]=m20; M[2][1]=m21; M[2][2]=m22;
    }

    static Matrix3x3 Identity() { return Matrix3x3(); }

    /** Diagonal matrix from a vector (useful for principal-axis inertia tensors). */
    static Matrix3x3 Diagonal(const Vector3D& d) {
        return Matrix3x3(d.X, 0, 0,  0, d.Y, 0,  0, 0, d.Z);
    }

    /** Build a rotation matrix from a unit quaternion. */
    static Matrix3x3 FromQuaternion(const Quaternion& q) {
        const float w = q.W, x = q.X, y = q.Y, z = q.Z;
        const float xx = x*x, yy = y*y, zz = z*z;
        const float xy = x*y, xz = x*z, yz = y*z;
        const float wx = w*x, wy = w*y, wz = w*z;
        return Matrix3x3(
            1.0f - 2.0f*(yy+zz),  2.0f*(xy-wz),         2.0f*(xz+wy),
            2.0f*(xy+wz),         1.0f - 2.0f*(xx+zz),  2.0f*(yz-wx),
            2.0f*(xz-wy),         2.0f*(yz+wx),         1.0f - 2.0f*(xx+yy)
        );
    }

    Matrix3x3 Transpose() const {
        return Matrix3x3(
            M[0][0], M[1][0], M[2][0],
            M[0][1], M[1][1], M[2][1],
            M[0][2], M[1][2], M[2][2]
        );
    }

    float Determinant() const {
        return M[0][0]*(M[1][1]*M[2][2] - M[1][2]*M[2][1])
             - M[0][1]*(M[1][0]*M[2][2] - M[1][2]*M[2][0])
             + M[0][2]*(M[1][0]*M[2][1] - M[1][1]*M[2][0]);
    }

    /**
     * @brief Inverse via cofactor expansion. Returns identity if singular.
     */
    Matrix3x3 Inverse() const {
        const float det = Determinant();
        if (std::abs(det) < 1e-12f) return Matrix3x3();
        const float inv = 1.0f / det;
        Matrix3x3 r;
        r.M[0][0] =  (M[1][1]*M[2][2] - M[1][2]*M[2][1]) * inv;
        r.M[0][1] = -(M[0][1]*M[2][2] - M[0][2]*M[2][1]) * inv;
        r.M[0][2] =  (M[0][1]*M[1][2] - M[0][2]*M[1][1]) * inv;
        r.M[1][0] = -(M[1][0]*M[2][2] - M[1][2]*M[2][0]) * inv;
        r.M[1][1] =  (M[0][0]*M[2][2] - M[0][2]*M[2][0]) * inv;
        r.M[1][2] = -(M[0][0]*M[1][2] - M[0][2]*M[1][0]) * inv;
        r.M[2][0] =  (M[1][0]*M[2][1] - M[1][1]*M[2][0]) * inv;
        r.M[2][1] = -(M[0][0]*M[2][1] - M[0][1]*M[2][0]) * inv;
        r.M[2][2] =  (M[0][0]*M[1][1] - M[0][1]*M[1][0]) * inv;
        return r;
    }

    /** Matrix * column vector. */
    Vector3D Multiply(const Vector3D& v) const {
        return Vector3D(
            M[0][0]*v.X + M[0][1]*v.Y + M[0][2]*v.Z,
            M[1][0]*v.X + M[1][1]*v.Y + M[1][2]*v.Z,
            M[2][0]*v.X + M[2][1]*v.Y + M[2][2]*v.Z
        );
    }

    Matrix3x3 Multiply(const Matrix3x3& o) const {
        Matrix3x3 r;
        for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 3; ++j)
                r.M[i][j] = M[i][0]*o.M[0][j] + M[i][1]*o.M[1][j] + M[i][2]*o.M[2][j];
        return r;
    }

    Matrix3x3 Scale(float s) const {
        Matrix3x3 r;
        for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 3; ++j)
                r.M[i][j] = M[i][j] * s;
        return r;
    }

    Matrix3x3 Add(const Matrix3x3& o) const {
        Matrix3x3 r;
        for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 3; ++j)
                r.M[i][j] = M[i][j] + o.M[i][j];
        return r;
    }

    Matrix3x3 Subtract(const Matrix3x3& o) const {
        Matrix3x3 r;
        for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 3; ++j)
                r.M[i][j] = M[i][j] - o.M[i][j];
        return r;
    }

    bool IsEqual(const Matrix3x3& o, float eps = 1e-5f) const {
        for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 3; ++j)
                if (std::abs(M[i][j] - o.M[i][j]) > eps) return false;
        return true;
    }

    Vector3D operator*(const Vector3D& v) const { return Multiply(v); }
    Matrix3x3 operator*(const Matrix3x3& o) const { return Multiply(o); }
    Matrix3x3 operator*(float s) const { return Scale(s); }
    Matrix3x3 operator+(const Matrix3x3& o) const { return Add(o); }
    Matrix3x3 operator-(const Matrix3x3& o) const { return Subtract(o); }

    KL_BEGIN_FIELDS(Matrix3x3)
    KL_END_FIELDS

    KL_BEGIN_METHODS(Matrix3x3)
        KL_METHOD_AUTO(Matrix3x3, Transpose, "Transpose"),
        KL_METHOD_AUTO(Matrix3x3, Determinant, "Determinant"),
        KL_METHOD_AUTO(Matrix3x3, Inverse, "Inverse"),
        KL_METHOD_OVLD_CONST(Matrix3x3, Multiply, Vector3D, const Vector3D &),
        KL_METHOD_OVLD_CONST(Matrix3x3, Multiply, Matrix3x3, const Matrix3x3 &),
        KL_METHOD_AUTO(Matrix3x3, Scale, "Scale")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(Matrix3x3)
        KL_CTOR0(Matrix3x3)
    KL_END_DESCRIBE(Matrix3x3)
};

inline Matrix3x3 operator*(float s, const Matrix3x3& m) { return m.Scale(s); }

} // namespace koilo
