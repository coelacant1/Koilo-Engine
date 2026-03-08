// SPDX-License-Identifier: GPL-3.0-or-later
#include <koilo/core/math/matrix4x4.hpp>
#include <cmath>
#include <cstring>
#include <cstdio>

namespace koilo {

// ============================================================================
// Non-inlined Constructors
// ============================================================================

Matrix4x4::Matrix4x4(
    const float& m00, const float& m01, const float& m02, const float& m03,
    const float& m10, const float& m11, const float& m12, const float& m13,
    const float& m20, const float& m21, const float& m22, const float& m23,
    const float& m30, const float& m31, const float& m32, const float& m33
) {
    M[0][0] = m00; M[0][1] = m01; M[0][2] = m02; M[0][3] = m03;
    M[1][0] = m10; M[1][1] = m11; M[1][2] = m12; M[1][3] = m13;
    M[2][0] = m20; M[2][1] = m21; M[2][2] = m22; M[2][3] = m23;
    M[3][0] = m30; M[3][1] = m31; M[3][2] = m32; M[3][3] = m33;
}

Matrix4x4::Matrix4x4(const Vector3D& row0, const Vector3D& row1, const Vector3D& row2, const Vector3D& row3) {
    M[0][0] = row0.X; M[0][1] = row0.Y; M[0][2] = row0.Z; M[0][3] = 0.0f;
    M[1][0] = row1.X; M[1][1] = row1.Y; M[1][2] = row1.Z; M[1][3] = 0.0f;
    M[2][0] = row2.X; M[2][1] = row2.Y; M[2][2] = row2.Z; M[2][3] = 0.0f;
    M[3][0] = row3.X; M[3][1] = row3.Y; M[3][2] = row3.Z; M[3][3] = 1.0f;
}

// ============================================================================
// Non-inlined Operations
// ============================================================================

Matrix4x4 Matrix4x4::Add(const Matrix4x4& matrix) const {
    Matrix4x4 result;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            result.M[i][j] = M[i][j] + matrix.M[i][j];
        }
    }
    return result;
}

Matrix4x4 Matrix4x4::Subtract(const Matrix4x4& matrix) const {
    Matrix4x4 result;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            result.M[i][j] = M[i][j] - matrix.M[i][j];
        }
    }
    return result;
}

Matrix4x4 Matrix4x4::Multiply(const float& scalar) const {
    Matrix4x4 result;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            result.M[i][j] = M[i][j] * scalar;
        }
    }
    return result;
}

Vector2D Matrix4x4::TransformVector(const Vector2D& vector) const {
    // Treat as 2D point (z = 0, w = 1)
    float x = M[0][0] * vector.X + M[0][1] * vector.Y + M[0][3];
    float y = M[1][0] * vector.X + M[1][1] * vector.Y + M[1][3];
    return Vector2D(x, y);
}

float Matrix4x4::Determinant() const {
    // Using cofactor expansion along first row
    float det = 0.0f;

    // Submatrix determinants (3x3)
    float sub00 = M[1][1] * (M[2][2] * M[3][3] - M[2][3] * M[3][2]) -
                  M[1][2] * (M[2][1] * M[3][3] - M[2][3] * M[3][1]) +
                  M[1][3] * (M[2][1] * M[3][2] - M[2][2] * M[3][1]);

    float sub01 = M[1][0] * (M[2][2] * M[3][3] - M[2][3] * M[3][2]) -
                  M[1][2] * (M[2][0] * M[3][3] - M[2][3] * M[3][0]) +
                  M[1][3] * (M[2][0] * M[3][2] - M[2][2] * M[3][0]);

    float sub02 = M[1][0] * (M[2][1] * M[3][3] - M[2][3] * M[3][1]) -
                  M[1][1] * (M[2][0] * M[3][3] - M[2][3] * M[3][0]) +
                  M[1][3] * (M[2][0] * M[3][1] - M[2][1] * M[3][0]);

    float sub03 = M[1][0] * (M[2][1] * M[3][2] - M[2][2] * M[3][1]) -
                  M[1][1] * (M[2][0] * M[3][2] - M[2][2] * M[3][0]) +
                  M[1][2] * (M[2][0] * M[3][1] - M[2][1] * M[3][0]);

    det = M[0][0] * sub00 - M[0][1] * sub01 + M[0][2] * sub02 - M[0][3] * sub03;

    return det;
}

Matrix4x4 Matrix4x4::Inverse() const {
    float det = Determinant();

    if (std::abs(det) < 0.0001f) {
        // Matrix is singular, return identity
        return Identity();
    }

    Matrix4x4 result;
    float invDet = 1.0f / det;

    // Calculate cofactor matrix and transpose (adjugate)
    // This is a simplified implementation using the standard formula
    result.M[0][0] = (M[1][1] * M[2][2] * M[3][3] + M[1][2] * M[2][3] * M[3][1] + M[1][3] * M[2][1] * M[3][2] -
                      M[1][1] * M[2][3] * M[3][2] - M[1][2] * M[2][1] * M[3][3] - M[1][3] * M[2][2] * M[3][1]) * invDet;

    result.M[0][1] = (M[0][1] * M[2][3] * M[3][2] + M[0][2] * M[2][1] * M[3][3] + M[0][3] * M[2][2] * M[3][1] -
                      M[0][1] * M[2][2] * M[3][3] - M[0][2] * M[2][3] * M[3][1] - M[0][3] * M[2][1] * M[3][2]) * invDet;

    result.M[0][2] = (M[0][1] * M[1][2] * M[3][3] + M[0][2] * M[1][3] * M[3][1] + M[0][3] * M[1][1] * M[3][2] -
                      M[0][1] * M[1][3] * M[3][2] - M[0][2] * M[1][1] * M[3][3] - M[0][3] * M[1][2] * M[3][1]) * invDet;

    result.M[0][3] = (M[0][1] * M[1][3] * M[2][2] + M[0][2] * M[1][1] * M[2][3] + M[0][3] * M[1][2] * M[2][1] -
                      M[0][1] * M[1][2] * M[2][3] - M[0][2] * M[1][3] * M[2][1] - M[0][3] * M[1][1] * M[2][2]) * invDet;

    result.M[1][0] = (M[1][0] * M[2][3] * M[3][2] + M[1][2] * M[2][0] * M[3][3] + M[1][3] * M[2][2] * M[3][0] -
                      M[1][0] * M[2][2] * M[3][3] - M[1][2] * M[2][3] * M[3][0] - M[1][3] * M[2][0] * M[3][2]) * invDet;

    result.M[1][1] = (M[0][0] * M[2][2] * M[3][3] + M[0][2] * M[2][3] * M[3][0] + M[0][3] * M[2][0] * M[3][2] -
                      M[0][0] * M[2][3] * M[3][2] - M[0][2] * M[2][0] * M[3][3] - M[0][3] * M[2][2] * M[3][0]) * invDet;

    result.M[1][2] = (M[0][0] * M[1][3] * M[3][2] + M[0][2] * M[1][0] * M[3][3] + M[0][3] * M[1][2] * M[3][0] -
                      M[0][0] * M[1][2] * M[3][3] - M[0][2] * M[1][3] * M[3][0] - M[0][3] * M[1][0] * M[3][2]) * invDet;

    result.M[1][3] = (M[0][0] * M[1][2] * M[2][3] + M[0][2] * M[1][3] * M[2][0] + M[0][3] * M[1][0] * M[2][2] -
                      M[0][0] * M[1][3] * M[2][2] - M[0][2] * M[1][0] * M[2][3] - M[0][3] * M[1][2] * M[2][0]) * invDet;

    result.M[2][0] = (M[1][0] * M[2][1] * M[3][3] + M[1][1] * M[2][3] * M[3][0] + M[1][3] * M[2][0] * M[3][1] -
                      M[1][0] * M[2][3] * M[3][1] - M[1][1] * M[2][0] * M[3][3] - M[1][3] * M[2][1] * M[3][0]) * invDet;

    result.M[2][1] = (M[0][0] * M[2][3] * M[3][1] + M[0][1] * M[2][0] * M[3][3] + M[0][3] * M[2][1] * M[3][0] -
                      M[0][0] * M[2][1] * M[3][3] - M[0][1] * M[2][3] * M[3][0] - M[0][3] * M[2][0] * M[3][1]) * invDet;

    result.M[2][2] = (M[0][0] * M[1][1] * M[3][3] + M[0][1] * M[1][3] * M[3][0] + M[0][3] * M[1][0] * M[3][1] -
                      M[0][0] * M[1][3] * M[3][1] - M[0][1] * M[1][0] * M[3][3] - M[0][3] * M[1][1] * M[3][0]) * invDet;

    result.M[2][3] = (M[0][0] * M[1][3] * M[2][1] + M[0][1] * M[1][0] * M[2][3] + M[0][3] * M[1][1] * M[2][0] -
                      M[0][0] * M[1][1] * M[2][3] - M[0][1] * M[1][3] * M[2][0] - M[0][3] * M[1][0] * M[2][1]) * invDet;

    result.M[3][0] = (M[1][0] * M[2][2] * M[3][1] + M[1][1] * M[2][0] * M[3][2] + M[1][2] * M[2][1] * M[3][0] -
                      M[1][0] * M[2][1] * M[3][2] - M[1][1] * M[2][2] * M[3][0] - M[1][2] * M[2][0] * M[3][1]) * invDet;

    result.M[3][1] = (M[0][0] * M[2][1] * M[3][2] + M[0][1] * M[2][2] * M[3][0] + M[0][2] * M[2][0] * M[3][1] -
                      M[0][0] * M[2][2] * M[3][1] - M[0][1] * M[2][0] * M[3][2] - M[0][2] * M[2][1] * M[3][0]) * invDet;

    result.M[3][2] = (M[0][0] * M[1][2] * M[3][1] + M[0][1] * M[1][0] * M[3][2] + M[0][2] * M[1][1] * M[3][0] -
                      M[0][0] * M[1][1] * M[3][2] - M[0][1] * M[1][2] * M[3][0] - M[0][2] * M[1][0] * M[3][1]) * invDet;

    result.M[3][3] = (M[0][0] * M[1][1] * M[2][2] + M[0][1] * M[1][2] * M[2][0] + M[0][2] * M[1][0] * M[2][1] -
                      M[0][0] * M[1][2] * M[2][1] - M[0][1] * M[1][0] * M[2][2] - M[0][2] * M[1][1] * M[2][0]) * invDet;

    return result;
}

// ============================================================================
// Extraction Methods
// ============================================================================

Vector3D Matrix4x4::GetTranslation() const {
    return Vector3D(M[0][3], M[1][3], M[2][3]);
}

Vector3D Matrix4x4::GetScale() const {
    Vector3D scaleX(M[0][0], M[1][0], M[2][0]);
    Vector3D scaleY(M[0][1], M[1][1], M[2][1]);
    Vector3D scaleZ(M[0][2], M[1][2], M[2][2]);

    return Vector3D(scaleX.Magnitude(), scaleY.Magnitude(), scaleZ.Magnitude());
}

Quaternion Matrix4x4::GetRotation() const {
    // Remove scale from matrix
    Vector3D scale = GetScale();

    float m00 = M[0][0] / scale.X;
    float m01 = M[0][1] / scale.Y;
    float m02 = M[0][2] / scale.Z;
    float m10 = M[1][0] / scale.X;
    float m11 = M[1][1] / scale.Y;
    float m12 = M[1][2] / scale.Z;
    float m20 = M[2][0] / scale.X;
    float m21 = M[2][1] / scale.Y;
    float m22 = M[2][2] / scale.Z;

    // Convert rotation matrix to quaternion
    Quaternion q;
    float trace = m00 + m11 + m22;

    if (trace > 0.0f) {
        float s = std::sqrt(trace + 1.0f) * 2.0f;
        q.W = 0.25f * s;
        q.X = (m21 - m12) / s;
        q.Y = (m02 - m20) / s;
        q.Z = (m10 - m01) / s;
    } else if (m00 > m11 && m00 > m22) {
        float s = std::sqrt(1.0f + m00 - m11 - m22) * 2.0f;
        q.W = (m21 - m12) / s;
        q.X = 0.25f * s;
        q.Y = (m01 + m10) / s;
        q.Z = (m02 + m20) / s;
    } else if (m11 > m22) {
        float s = std::sqrt(1.0f + m11 - m00 - m22) * 2.0f;
        q.W = (m02 - m20) / s;
        q.X = (m01 + m10) / s;
        q.Y = 0.25f * s;
        q.Z = (m12 + m21) / s;
    } else {
        float s = std::sqrt(1.0f + m22 - m00 - m11) * 2.0f;
        q.W = (m10 - m01) / s;
        q.X = (m02 + m20) / s;
        q.Y = (m12 + m21) / s;
        q.Z = 0.25f * s;
    }

    return q.UnitQuaternion();
}

bool Matrix4x4::IsEqual(const Matrix4x4& matrix) const {
    const float epsilon = 0.0001f;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            if (std::abs(M[i][j] - matrix.M[i][j]) > epsilon) {
                return false;
            }
        }
    }
    return true;
}

bool Matrix4x4::IsIdentity() const {
    return IsEqual(Identity());
}

UString Matrix4x4::ToString() const {
    char buffer[512];
    snprintf(buffer, sizeof(buffer),
        "[%.3f %.3f %.3f %.3f]\n[%.3f %.3f %.3f %.3f]\n[%.3f %.3f %.3f %.3f]\n[%.3f %.3f %.3f %.3f]",
        M[0][0], M[0][1], M[0][2], M[0][3],
        M[1][0], M[1][1], M[1][2], M[1][3],
        M[2][0], M[2][1], M[2][2], M[2][3],
        M[3][0], M[3][1], M[3][2], M[3][3]
    );
    return UString(buffer);
}

// ============================================================================
// Static Factory Methods
// ============================================================================

Matrix4x4 Matrix4x4::Identity() {
    Matrix4x4 result;
    result.SetIdentity();
    return result;
}

Matrix4x4 Matrix4x4::Translation(const Vector3D& translation) {
    Matrix4x4 result;
    result.SetIdentity();
    result.M[0][3] = translation.X;
    result.M[1][3] = translation.Y;
    result.M[2][3] = translation.Z;
    return result;
}

Matrix4x4 Matrix4x4::Scale(const Vector3D& scale) {
    Matrix4x4 result;
    result.SetIdentity();
    result.M[0][0] = scale.X;
    result.M[1][1] = scale.Y;
    result.M[2][2] = scale.Z;
    return result;
}

Matrix4x4 Matrix4x4::Scale(const float& scale) {
    return Scale(Vector3D(scale, scale, scale));
}

Matrix4x4 Matrix4x4::Rotation(const Quaternion& rotation) {
    Quaternion q = rotation.UnitQuaternion();

    float xx = q.X * q.X;
    float yy = q.Y * q.Y;
    float zz = q.Z * q.Z;
    float xy = q.X * q.Y;
    float xz = q.X * q.Z;
    float yz = q.Y * q.Z;
    float wx = q.W * q.X;
    float wy = q.W * q.Y;
    float wz = q.W * q.Z;

    Matrix4x4 result;
    result.M[0][0] = 1.0f - 2.0f * (yy + zz);
    result.M[0][1] = 2.0f * (xy - wz);
    result.M[0][2] = 2.0f * (xz + wy);
    result.M[0][3] = 0.0f;

    result.M[1][0] = 2.0f * (xy + wz);
    result.M[1][1] = 1.0f - 2.0f * (xx + zz);
    result.M[1][2] = 2.0f * (yz - wx);
    result.M[1][3] = 0.0f;

    result.M[2][0] = 2.0f * (xz - wy);
    result.M[2][1] = 2.0f * (yz + wx);
    result.M[2][2] = 1.0f - 2.0f * (xx + yy);
    result.M[2][3] = 0.0f;

    result.M[3][0] = 0.0f;
    result.M[3][1] = 0.0f;
    result.M[3][2] = 0.0f;
    result.M[3][3] = 1.0f;

    return result;
}

Matrix4x4 Matrix4x4::RotationX(const float& angle) {
    Matrix4x4 result;
    result.SetIdentity();

    float c = std::cos(angle);
    float s = std::sin(angle);

    result.M[1][1] = c;
    result.M[1][2] = -s;
    result.M[2][1] = s;
    result.M[2][2] = c;

    return result;
}

Matrix4x4 Matrix4x4::RotationY(const float& angle) {
    Matrix4x4 result;
    result.SetIdentity();

    float c = std::cos(angle);
    float s = std::sin(angle);

    result.M[0][0] = c;
    result.M[0][2] = s;
    result.M[2][0] = -s;
    result.M[2][2] = c;

    return result;
}

Matrix4x4 Matrix4x4::RotationZ(const float& angle) {
    Matrix4x4 result;
    result.SetIdentity();

    float c = std::cos(angle);
    float s = std::sin(angle);

    result.M[0][0] = c;
    result.M[0][1] = -s;
    result.M[1][0] = s;
    result.M[1][1] = c;

    return result;
}

Matrix4x4 Matrix4x4::RotationEuler(const Vector3D& eulerXYZ) {
    // Apply rotations in order: Z, Y, X
    return RotationZ(eulerXYZ.Z).Multiply(RotationY(eulerXYZ.Y)).Multiply(RotationX(eulerXYZ.X));
}

Matrix4x4 Matrix4x4::TRS(const Vector3D& position, const Quaternion& rotation, const Vector3D& scale) {
    Matrix4x4 t = Translation(position);
    Matrix4x4 r = Rotation(rotation);
    Matrix4x4 s = Scale(scale);

    // Order: Scale -> Rotate -> Translate
    return t.Multiply(r).Multiply(s);
}

Matrix4x4 Matrix4x4::LookAt(const Vector3D& eye, const Vector3D& target, const Vector3D& up) {
    Vector3D forward = (target - eye).Normal();
    Vector3D right = forward.CrossProduct(up).Normal();
    Vector3D newUp = right.CrossProduct(forward);

    Matrix4x4 result;

    result.M[0][0] = right.X;
    result.M[0][1] = right.Y;
    result.M[0][2] = right.Z;
    result.M[0][3] = -right.DotProduct(eye);

    result.M[1][0] = newUp.X;
    result.M[1][1] = newUp.Y;
    result.M[1][2] = newUp.Z;
    result.M[1][3] = -newUp.DotProduct(eye);

    result.M[2][0] = -forward.X;
    result.M[2][1] = -forward.Y;
    result.M[2][2] = -forward.Z;
    result.M[2][3] = forward.DotProduct(eye);

    result.M[3][0] = 0.0f;
    result.M[3][1] = 0.0f;
    result.M[3][2] = 0.0f;
    result.M[3][3] = 1.0f;

    return result;
}

Matrix4x4 Matrix4x4::Perspective(const float& fov, const float& aspect, const float& near, const float& far) {
    float tanHalfFov = std::tan(fov / 2.0f);

    Matrix4x4 result;
    std::memset(result.M, 0, sizeof(result.M));

    result.M[0][0] = 1.0f / (aspect * tanHalfFov);
    result.M[1][1] = 1.0f / tanHalfFov;
    result.M[2][2] = -(far + near) / (far - near);
    result.M[2][3] = -(2.0f * far * near) / (far - near);
    result.M[3][2] = -1.0f;

    return result;
}

Matrix4x4 Matrix4x4::Orthographic(const float& left, const float& right, const float& bottom,
                                   const float& top, const float& near, const float& far) {
    Matrix4x4 result;
    result.SetIdentity();

    result.M[0][0] = 2.0f / (right - left);
    result.M[1][1] = 2.0f / (top - bottom);
    result.M[2][2] = -2.0f / (far - near);

    result.M[0][3] = -(right + left) / (right - left);
    result.M[1][3] = -(top + bottom) / (top - bottom);
    result.M[2][3] = -(far + near) / (far - near);

    return result;
}

Matrix4x4 Matrix4x4::LERP(const Matrix4x4& m1, const Matrix4x4& m2, const float& ratio) {
    Matrix4x4 result;
    float invRatio = 1.0f - ratio;

    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            result.M[i][j] = m1.M[i][j] * invRatio + m2.M[i][j] * ratio;
        }
    }

    return result;
}

// ============================================================================
// Operator Overloads
// ============================================================================

bool Matrix4x4::operator==(const Matrix4x4& matrix) const {
    return IsEqual(matrix);
}

bool Matrix4x4::operator!=(const Matrix4x4& matrix) const {
    return !IsEqual(matrix);
}

Matrix4x4 Matrix4x4::operator+(const Matrix4x4& matrix) const {
    return Add(matrix);
}

Matrix4x4 Matrix4x4::operator-(const Matrix4x4& matrix) const {
    return Subtract(matrix);
}

Matrix4x4 Matrix4x4::operator*(const float& scalar) const {
    return Multiply(scalar);
}

float* Matrix4x4::operator[](int row) {
    return M[row];
}

const float* Matrix4x4::operator[](int row) const {
    return M[row];
}

} // namespace koilo
