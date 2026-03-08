// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file Matrix4x4.h
 * @brief Defines the Matrix4x4 class for 4x4 matrix operations and transformations.
 *
 * The Matrix4x4 class provides methods for 4x4 matrix operations commonly used
 * in 3D graphics, including multiplication, transpose, inverse, and transformation
 * matrix construction (translation, rotation, scale, projection).
 *
 * @date 22/12/2024
 * @version 1.0
 * @author Coela Can't
 */

#pragma once

#include "mathematics.hpp"
#include "vector3d.hpp"
#include "vector2d.hpp"
#include "quaternion.hpp"
#include <koilo/registry/reflect_macros.hpp>
#include <cstring>


namespace koilo {

/**
 * @class Matrix4x4
 * @brief Represents a 4x4 matrix for 3D transformations.
 *
 * The Matrix4x4 class stores matrix data in row-major order and provides
 * operations for matrix arithmetic, transformations, and projections commonly
 * used in 3D graphics and physics simulations.
 */
class Matrix4x4 {
public:
    float M[4][4]; ///< 4x4 matrix data stored in row-major order [row][column]

    /**
     * @brief Default constructor. Initializes the matrix to identity.
     */
    Matrix4x4();

    /**
     * @brief Copy constructor. Clones another matrix.
     * @param matrix The matrix to copy from.
     */
    Matrix4x4(const Matrix4x4& matrix);

    /**
     * @brief Constructs a matrix from 16 individual float values (row-major order).
     * @param m00 Element at row 0, column 0
     * @param m01 Element at row 0, column 1
     * @param m02 Element at row 0, column 2
     * @param m03 Element at row 0, column 3
     * @param m10 Element at row 1, column 0
     * @param m11 Element at row 1, column 1
     * @param m12 Element at row 1, column 2
     * @param m13 Element at row 1, column 3
     * @param m20 Element at row 2, column 0
     * @param m21 Element at row 2, column 1
     * @param m22 Element at row 2, column 2
     * @param m23 Element at row 2, column 3
     * @param m30 Element at row 3, column 0
     * @param m31 Element at row 3, column 1
     * @param m32 Element at row 3, column 2
     * @param m33 Element at row 3, column 3
     */
    Matrix4x4(
        const float& m00, const float& m01, const float& m02, const float& m03,
        const float& m10, const float& m11, const float& m12, const float& m13,
        const float& m20, const float& m21, const float& m22, const float& m23,
        const float& m30, const float& m31, const float& m32, const float& m33
    );

    /**
     * @brief Constructs a matrix from four Vector3D row vectors and an optional fourth row.
     * @param row0 First row (X, Y, Z components, W=0)
     * @param row1 Second row (X, Y, Z components, W=0)
     * @param row2 Third row (X, Y, Z components, W=0)
     * @param row3 Fourth row (X, Y, Z components, W=1)
     */
    Matrix4x4(const Vector3D& row0, const Vector3D& row1, const Vector3D& row2, const Vector3D& row3);

    /**
     * @brief Sets the matrix to the identity matrix.
     */
    void SetIdentity();

    /**
     * @brief Adds two matrices element-wise.
     * @param matrix The matrix to add.
     * @return The resulting matrix after addition.
     */
    Matrix4x4 Add(const Matrix4x4& matrix) const;

    /**
     * @brief Subtracts a matrix from this matrix element-wise.
     * @param matrix The matrix to subtract.
     * @return The resulting matrix after subtraction.
     */
    Matrix4x4 Subtract(const Matrix4x4& matrix) const;

    /**
     * @brief Multiplies this matrix by another matrix.
     * @param matrix The right-hand side matrix.
     * @return The resulting matrix product.
     */
    Matrix4x4 Multiply(const Matrix4x4& matrix) const;

    /**
     * @brief Scales the matrix by a scalar value.
     * @param scalar The scalar to multiply.
     * @return The resulting scaled matrix.
     */
    Matrix4x4 Multiply(const float& scalar) const;

    /**
     * @brief Transforms a 3D vector by this matrix (assumes w=1 for position).
     * @param vector The 3D vector to transform.
     * @return The transformed 3D vector.
     */
    Vector3D TransformVector(const Vector3D& vector) const;

    /**
     * @brief Transforms a 3D direction vector by this matrix (assumes w=0, ignores translation).
     * @param direction The 3D direction vector to transform.
     * @return The transformed 3D direction vector.
     */
    Vector3D TransformDirection(const Vector3D& direction) const;

    /**
     * @brief Transforms a 2D vector by this matrix (assumes z=0, w=1).
     * @param vector The 2D vector to transform.
     * @return The transformed 2D vector.
     */
    Vector2D TransformVector(const Vector2D& vector) const;

    /**
     * @brief Transposes the matrix (swaps rows and columns).
     * @return The transposed matrix.
     */
    Matrix4x4 Transpose() const;

    /**
     * @brief Computes the determinant of the matrix.
     * @return The determinant value.
     */
    float Determinant() const;

    /**
     * @brief Computes the inverse of the matrix if it exists.
     * @return The inverse matrix. Returns identity if matrix is singular.
     */
    Matrix4x4 Inverse() const;

    /**
     * @brief Extracts the translation component from the matrix.
     * @return A Vector3D representing the translation (last column's xyz).
     */
    Vector3D GetTranslation() const;

    /**
     * @brief Extracts the scale component from the matrix.
     * @return A Vector3D representing the scale along each axis.
     */
    Vector3D GetScale() const;

    /**
     * @brief Extracts the rotation component as a quaternion.
     * @return A Quaternion representing the rotation.
     */
    Quaternion GetRotation() const;

    /**
     * @brief Checks if two matrices are equal element-wise.
     * @param matrix The matrix to compare with.
     * @return True if equal, false otherwise.
     */
    bool IsEqual(const Matrix4x4& matrix) const;

    /**
     * @brief Checks if the matrix is an identity matrix.
     * @return True if identity, false otherwise.
     */
    bool IsIdentity() const;

    /**
     * @brief Converts the matrix to a string representation.
     * @return A string representing the matrix.
     */
    koilo::UString ToString() const;

    // --- Static factory methods ---

    /**
     * @brief Creates an identity matrix.
     * @return A 4x4 identity matrix.
     */
    static Matrix4x4 Identity();

    /**
     * @brief Creates a translation matrix.
     * @param translation The translation vector.
     * @return A 4x4 translation matrix.
     */
    static Matrix4x4 Translation(const Vector3D& translation);

    /**
     * @brief Creates a scale matrix.
     * @param scale The scale vector.
     * @return A 4x4 scale matrix.
     */
    static Matrix4x4 Scale(const Vector3D& scale);

    /**
     * @brief Creates a uniform scale matrix.
     * @param scale The uniform scale factor.
     * @return A 4x4 uniform scale matrix.
     */
    static Matrix4x4 Scale(const float& scale);

    /**
     * @brief Creates a rotation matrix from a quaternion.
     * @param rotation The rotation quaternion.
     * @return A 4x4 rotation matrix.
     */
    static Matrix4x4 Rotation(const Quaternion& rotation);

    /**
     * @brief Creates a rotation matrix around the X-axis.
     * @param angle The rotation angle in radians.
     * @return A 4x4 rotation matrix.
     */
    static Matrix4x4 RotationX(const float& angle);

    /**
     * @brief Creates a rotation matrix around the Y-axis.
     * @param angle The rotation angle in radians.
     * @return A 4x4 rotation matrix.
     */
    static Matrix4x4 RotationY(const float& angle);

    /**
     * @brief Creates a rotation matrix around the Z-axis.
     * @param angle The rotation angle in radians.
     * @return A 4x4 rotation matrix.
     */
    static Matrix4x4 RotationZ(const float& angle);

    /**
     * @brief Creates a rotation matrix from Euler angles (XYZ order).
     * @param eulerXYZ The Euler angles in radians (X, Y, Z).
     * @return A 4x4 rotation matrix.
     */
    static Matrix4x4 RotationEuler(const Vector3D& eulerXYZ);

    /**
     * @brief Creates a transformation matrix from position, rotation, and scale.
     * @param position The translation vector.
     * @param rotation The rotation quaternion.
     * @param scale The scale vector.
     * @return A 4x4 transformation matrix.
     */
    static Matrix4x4 TRS(const Vector3D& position, const Quaternion& rotation, const Vector3D& scale);

    /**
     * @brief Creates a look-at view matrix.
     * @param eye The camera position.
     * @param target The target position to look at.
     * @param up The up direction vector.
     * @return A 4x4 view matrix.
     */
    static Matrix4x4 LookAt(const Vector3D& eye, const Vector3D& target, const Vector3D& up);

    /**
     * @brief Creates a perspective projection matrix.
     * @param fov Field of view in radians.
     * @param aspect Aspect ratio (width / height).
     * @param near Near clipping plane distance.
     * @param far Far clipping plane distance.
     * @return A 4x4 perspective projection matrix.
     */
    static Matrix4x4 Perspective(const float& fov, const float& aspect, const float& near, const float& far);

    /**
     * @brief Creates an orthographic projection matrix.
     * @param left Left clipping plane.
     * @param right Right clipping plane.
     * @param bottom Bottom clipping plane.
     * @param top Top clipping plane.
     * @param near Near clipping plane.
     * @param far Far clipping plane.
     * @return A 4x4 orthographic projection matrix.
     */
    static Matrix4x4 Orthographic(const float& left, const float& right, const float& bottom,
                                   const float& top, const float& near, const float& far);

    /**
     * @brief Performs linear interpolation between two matrices.
     * @param m1 The start matrix.
     * @param m2 The end matrix.
     * @param ratio The interpolation factor (0 to 1).
     * @return The interpolated matrix.
     */
    static Matrix4x4 LERP(const Matrix4x4& m1, const Matrix4x4& m2, const float& ratio);

    // --- Operator overloads ---

    /**
     * @brief Equality operator.
     * @param matrix The matrix to compare with.
     * @return True if equal, false otherwise.
     */
    bool operator ==(const Matrix4x4& matrix) const;

    /**
     * @brief Inequality operator.
     * @param matrix The matrix to compare with.
     * @return True if not equal, false otherwise.
     */
    bool operator !=(const Matrix4x4& matrix) const;

    /**
     * @brief Assignment operator.
     * @param matrix The matrix to copy.
     * @return A reference to this matrix.
     */
    Matrix4x4 operator =(const Matrix4x4& matrix);

    /**
     * @brief Addition operator.
     * @param matrix The right-hand side matrix.
     * @return The resulting matrix after addition.
     */
    Matrix4x4 operator +(const Matrix4x4& matrix) const;

    /**
     * @brief Subtraction operator.
     * @param matrix The right-hand side matrix.
     * @return The resulting matrix after subtraction.
     */
    Matrix4x4 operator -(const Matrix4x4& matrix) const;

    /**
     * @brief Matrix multiplication operator.
     * @param matrix The right-hand side matrix.
     * @return The resulting matrix product.
     */
    Matrix4x4 operator *(const Matrix4x4& matrix) const;

    /**
     * @brief Scalar multiplication operator.
     * @param scalar The scalar to multiply.
     * @return The resulting scaled matrix.
     */
    Matrix4x4 operator *(const float& scalar) const;

    /**
     * @brief Vector transformation operator (3D).
     * @param vector The 3D vector to transform.
     * @return The transformed vector.
     */
    Vector3D operator *(const Vector3D& vector) const;

    /**
     * @brief Access operator for matrix elements.
     * @param row The row index.
     * @return A pointer to the row.
     */
    float* operator [](int row);

    /**
     * @brief Const access operator for matrix elements.
     * @param row The row index.
     * @return A const pointer to the row.
     */
    const float* operator [](int row) const;

    KL_BEGIN_FIELDS(Matrix4x4)
        /* No reflected fields. */
    KL_END_FIELDS

    KL_BEGIN_METHODS(Matrix4x4)
        KL_METHOD_AUTO(Matrix4x4, SetIdentity, "Set identity"),
        /* Add */ KL_METHOD_OVLD_CONST(Matrix4x4, Add, Matrix4x4, const Matrix4x4 &),
        /* Subtract */ KL_METHOD_OVLD_CONST(Matrix4x4, Subtract, Matrix4x4, const Matrix4x4 &),
        /* Multiply */ KL_METHOD_OVLD_CONST(Matrix4x4, Multiply, Matrix4x4, const Matrix4x4 &),
        /* Multiply */ KL_METHOD_OVLD_CONST(Matrix4x4, Multiply, Matrix4x4, const float &),
        /* Transform vector */ KL_METHOD_OVLD_CONST(Matrix4x4, TransformVector, Vector3D, const Vector3D &),
        KL_METHOD_AUTO(Matrix4x4, TransformDirection, "Transform direction"),
        /* Transform vector */ KL_METHOD_OVLD_CONST(Matrix4x4, TransformVector, Vector2D, const Vector2D &),
        KL_METHOD_AUTO(Matrix4x4, Transpose, "Transpose"),
        KL_METHOD_AUTO(Matrix4x4, Determinant, "Determinant"),
        KL_METHOD_AUTO(Matrix4x4, Inverse, "Inverse"),
        KL_METHOD_AUTO(Matrix4x4, GetTranslation, "Get translation"),
        KL_METHOD_AUTO(Matrix4x4, GetScale, "Get scale"),
        KL_METHOD_AUTO(Matrix4x4, GetRotation, "Get rotation"),
        KL_METHOD_AUTO(Matrix4x4, IsEqual, "Is equal"),
        KL_METHOD_AUTO(Matrix4x4, IsIdentity, "Is identity"),
        KL_METHOD_AUTO(Matrix4x4, ToString, "To string"),
        KL_SMETHOD_AUTO(Matrix4x4::Identity, "Identity"),
        /* Translation */ KL_SMETHOD_OVLD(Matrix4x4, Translation, Matrix4x4, const Vector3D &),
        /* Scale */ KL_SMETHOD_OVLD(Matrix4x4, Scale, Matrix4x4, const Vector3D &),
        /* Scale */ KL_SMETHOD_OVLD(Matrix4x4, Scale, Matrix4x4, const float &),
        /* Rotation */ KL_SMETHOD_OVLD(Matrix4x4, Rotation, Matrix4x4, const Quaternion &),
        KL_SMETHOD_AUTO(Matrix4x4::RotationX, "Rotation x"),
        KL_SMETHOD_AUTO(Matrix4x4::RotationY, "Rotation y"),
        KL_SMETHOD_AUTO(Matrix4x4::RotationZ, "Rotation z"),
        KL_SMETHOD_AUTO(Matrix4x4::RotationEuler, "Rotation euler"),
        KL_SMETHOD_AUTO(Matrix4x4::TRS, "Trs"),
        KL_SMETHOD_AUTO(Matrix4x4::LookAt, "Look at"),
        KL_SMETHOD_AUTO(Matrix4x4::Perspective, "Perspective"),
        KL_SMETHOD_AUTO(Matrix4x4::Orthographic, "Orthographic"),
        KL_SMETHOD_AUTO(Matrix4x4::LERP, "Lerp")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(Matrix4x4)
        KL_CTOR0(Matrix4x4),
        KL_CTOR(Matrix4x4, const Matrix4x4 &),
        KL_CTOR(Matrix4x4, const float &, const float &, const float &, const float &, const float &, const float &, const float &, const float &, const float &, const float &, const float &, const float &, const float &, const float &, const float &, const float &),
        KL_CTOR(Matrix4x4, const Vector3D &, const Vector3D &, const Vector3D &, const Vector3D &)
    KL_END_DESCRIBE(Matrix4x4)

};

// ============================================================================
// Inline hot-path implementations
// ============================================================================

inline Matrix4x4::Matrix4x4() {
    M[0][0] = 1.0f; M[0][1] = 0.0f; M[0][2] = 0.0f; M[0][3] = 0.0f;
    M[1][0] = 0.0f; M[1][1] = 1.0f; M[1][2] = 0.0f; M[1][3] = 0.0f;
    M[2][0] = 0.0f; M[2][1] = 0.0f; M[2][2] = 1.0f; M[2][3] = 0.0f;
    M[3][0] = 0.0f; M[3][1] = 0.0f; M[3][2] = 0.0f; M[3][3] = 1.0f;
}

inline Matrix4x4::Matrix4x4(const Matrix4x4& matrix) {
    std::memcpy(M, matrix.M, sizeof(M));
}

inline void Matrix4x4::SetIdentity() {
    M[0][0] = 1.0f; M[0][1] = 0.0f; M[0][2] = 0.0f; M[0][3] = 0.0f;
    M[1][0] = 0.0f; M[1][1] = 1.0f; M[1][2] = 0.0f; M[1][3] = 0.0f;
    M[2][0] = 0.0f; M[2][1] = 0.0f; M[2][2] = 1.0f; M[2][3] = 0.0f;
    M[3][0] = 0.0f; M[3][1] = 0.0f; M[3][2] = 0.0f; M[3][3] = 1.0f;
}

inline Matrix4x4 Matrix4x4::Multiply(const Matrix4x4& m) const {
    Matrix4x4 r;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            r.M[i][j] = M[i][0] * m.M[0][j] + M[i][1] * m.M[1][j]
                       + M[i][2] * m.M[2][j] + M[i][3] * m.M[3][j];
        }
    }
    return r;
}

inline Vector3D Matrix4x4::TransformVector(const Vector3D& v) const {
    float x = M[0][0] * v.X + M[0][1] * v.Y + M[0][2] * v.Z + M[0][3];
    float y = M[1][0] * v.X + M[1][1] * v.Y + M[1][2] * v.Z + M[1][3];
    float z = M[2][0] * v.X + M[2][1] * v.Y + M[2][2] * v.Z + M[2][3];
    float w = M[3][0] * v.X + M[3][1] * v.Y + M[3][2] * v.Z + M[3][3];
    if (w != 0.0f && w != 1.0f) {
        return Vector3D(x / w, y / w, z / w);
    }
    return Vector3D(x, y, z);
}

inline Vector3D Matrix4x4::TransformDirection(const Vector3D& d) const {
    return Vector3D(
        M[0][0] * d.X + M[0][1] * d.Y + M[0][2] * d.Z,
        M[1][0] * d.X + M[1][1] * d.Y + M[1][2] * d.Z,
        M[2][0] * d.X + M[2][1] * d.Y + M[2][2] * d.Z
    );
}

inline Matrix4x4 Matrix4x4::Transpose() const {
    Matrix4x4 r;
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
            r.M[i][j] = M[j][i];
    return r;
}

inline Matrix4x4 Matrix4x4::operator*(const Matrix4x4& m) const { return Multiply(m); }
inline Vector3D Matrix4x4::operator*(const Vector3D& v) const { return TransformVector(v); }
inline Matrix4x4 Matrix4x4::operator=(const Matrix4x4& m) {
    if (this != &m) std::memcpy(M, m.M, sizeof(M));
    return *this;
}

} // namespace koilo
