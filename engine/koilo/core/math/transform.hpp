// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file Transform.h
 * @brief Defines the Transform class for managing position, rotation, and scale of objects.
 *
 * The Transform class provides functionality to represent and manipulate an object's position,
 * rotation, scale, and associated offsets in 3D space.
 *
 * @date 22/12/2024
 * @version 1.0
 * @author Coela Can't
 */

#pragma once

#include "rotation.hpp"
#include "vector3d.hpp"
#include "mathematics.hpp"
#include <koilo/core/platform/ustring.hpp>
#include <koilo/registry/reflect_macros.hpp>


namespace koilo {

/**
 * @class Transform
 * @brief Represents a 3D transformation including position, rotation, and scale.
 */
class Transform {
private:
    Quaternion baseRotation; ///< The base rotation of the object as a quaternion.
    Quaternion rotation; ///< The current rotation of the object as a quaternion.
    Vector3D position; ///< The position of the object in 3D space.
    Vector3D scale; ///< The scale of the object in 3D space.

    Vector3D scaleOffset; ///< Pivot point for scale operations.
    Vector3D rotationOffset; ///< Pivot point for rotation operations.

public:
    /**
     * @brief Default constructor.
     */
    Transform();

    /**
     * @brief Constructor initializing with Euler angles, position, and scale.
     * @param eulerXYZS The rotation in XYZ static Euler angles.
     * @param position The position vector.
     * @param scale The scale vector.
     */
    Transform(const Vector3D& eulerXYZS, const Vector3D& position, const Vector3D& scale);

    /**
     * @brief Constructor initializing with rotation, position, and scale.
     * @param rotation The rotation as a quaternion.
     * @param position The position vector.
     * @param scale The scale vector.
     */
    Transform(const Quaternion& rotation, const Vector3D& position, const Vector3D& scale);

    /**
     * @brief Constructor initializing with additional rotation and scale offsets.
     * @param eulerXYZS The rotation in XYZ static Euler angles.
     * @param position The position vector.
     * @param scale The scale vector.
     * @param rotationOffset Offset for the rotation.
     * @param scaleOffset Offset for the scale.
     */
    Transform(const Vector3D& eulerXYZS, const Vector3D& position, const Vector3D& scale, const Vector3D& rotationOffset, const Vector3D& scaleOffset);

    /**
     * @brief Constructor initializing with rotation and offsets.
     * @param rotation The rotation as a quaternion.
     * @param position The position vector.
     * @param scale The scale vector.
     * @param rotationOffset Offset for the rotation.
     * @param scaleOffset Offset for the scale.
     */
    Transform(const Quaternion& rotation, const Vector3D& position, const Vector3D& scale, const Vector3D& rotationOffset, const Vector3D& scaleOffset);

    /**
     * @brief Copy constructor.
     * @param transform The transform to copy.
     */
    Transform(const Transform& transform);

    /**
     * @brief Copy assignment operator.
     */
    Transform& operator=(const Transform& transform) = default;

    /**
     * @brief Sets the base rotation of the object.
     * @param baseRotation The base rotation as a quaternion.
     */
    void SetBaseRotation(const Quaternion& baseRotation);

    /**
     * @brief Gets the base rotation of the object.
     * @return The base rotation as a quaternion.
     */
    Quaternion GetBaseRotation() const;

    /**
     * @brief Sets the current rotation of the object.
     * @param rotation The rotation as a quaternion.
     */
    void SetRotation(const Quaternion& rotation);

    /**
     * @brief Sets the current rotation of the object using Euler angles.
     * @param eulerXYZS The rotation in XYZ static Euler angles.
     */
    void SetRotation(const Vector3D& eulerXYZS);

    /**
     * @brief Gets the current rotation of the object.
     * @return The current rotation as a quaternion.
     */
    Quaternion GetRotation() const;

    /**
     * @brief Sets the position of the object.
     * @param position The position vector.
     */
    void SetPosition(const Vector3D& position);

    /**
     * @brief Gets the position of the object.
     * @return The position vector.
     */
    Vector3D GetPosition() const;

    /**
     * @brief Sets the scale of the object.
     * @param scale The scale vector.
     */
    void SetScale(const Vector3D& scale);

    /**
     * @brief Gets the scale of the object.
     * @return The scale vector.
     */
    Vector3D GetScale() const;

    /**
     * @brief Sets the origin (pivot point) for both rotation and scale.
     * This is a convenience method that sets both rotationOffset and scaleOffset.
     * @param origin The origin point in local space.
     */
    void SetOrigin(const Vector3D& origin);

    /**
     * @brief Gets the rotation offset (rotation pivot point).
     * @return The rotation offset vector.
     */
    Vector3D GetOrigin() const;

    /**
     * @brief Sets the rotation offset of the object.
     * @param rotationOffset The rotation offset vector.
     */
    void SetRotationOffset(const Vector3D& rotationOffset);

    /**
     * @brief Gets the rotation offset of the object.
     * @return The rotation offset vector.
     */
    Vector3D GetRotationOffset() const;

    /**
     * @brief Sets the scale offset of the object.
     * @param scaleOffset The scale offset vector.
     */
    void SetScaleOffset(const Vector3D& scaleOffset);

    /**
     * @brief Gets the scale offset of the object.
     * @return The scale offset vector.
     */
    Vector3D GetScaleOffset() const;

    /**
     * @brief Rotates the object by the given Euler angles.
     * @param eulerXYZS The rotation in XYZ static Euler angles.
     */
    void Rotate(const Vector3D& eulerXYZS);

    /**
     * @brief Rotates the object by the given quaternion.
     * @param rotation The rotation as a quaternion.
     */
    void Rotate(const Quaternion& rotation);

    /**
     * @brief Translates the object by the given offset.
     * @param offset The translation offset vector.
     */
    void Translate(const Vector3D& offset);

    /**
     * @brief Scales the object by the given scale factors.
     * @param scale The scale factors as a vector.
     */
    void Scale(const Vector3D& scale);

    /**
     * @brief Converts the transform to a string representation.
     * @return A string representing the transform.
     */
    koilo::UString ToString();

    KL_BEGIN_FIELDS(Transform)
        KL_FIELD(Transform, rotation, "Rotation", 0, 0),
        KL_FIELD(Transform, position, "Position", 0, 0),
        KL_FIELD(Transform, scale, "Scale", 0, 0)
    KL_END_FIELDS

    KL_BEGIN_METHODS(Transform)
        KL_METHOD_AUTO(Transform, SetBaseRotation, "Set base rotation"),
        KL_METHOD_AUTO(Transform, GetBaseRotation, "Get base rotation"),
        /* Set rotation */ KL_METHOD_OVLD(Transform, SetRotation, void, const Quaternion &),
        /* Set rotation */ KL_METHOD_OVLD(Transform, SetRotation, void, const Vector3D &),
        KL_METHOD_AUTO(Transform, GetRotation, "Get rotation"),
        KL_METHOD_AUTO(Transform, SetPosition, "Set position"),
        KL_METHOD_AUTO(Transform, GetPosition, "Get position"),
        KL_METHOD_AUTO(Transform, SetScale, "Set scale"),
        KL_METHOD_AUTO(Transform, GetScale, "Get scale"),
        KL_METHOD_AUTO(Transform, SetOrigin, "Set origin (pivot for rotate+scale)"),
        KL_METHOD_AUTO(Transform, GetOrigin, "Get origin"),
        KL_METHOD_AUTO(Transform, SetRotationOffset, "Set rotation pivot"),
        KL_METHOD_AUTO(Transform, GetRotationOffset, "Get rotation pivot"),
        KL_METHOD_AUTO(Transform, SetScaleOffset, "Set scale pivot"),
        KL_METHOD_AUTO(Transform, GetScaleOffset, "Get scale pivot"),
        /* Rotate */ KL_METHOD_OVLD(Transform, Rotate, void, const Vector3D &),
        /* Rotate */ KL_METHOD_OVLD(Transform, Rotate, void, const Quaternion &),
        KL_METHOD_AUTO(Transform, Translate, "Translate"),
        KL_METHOD_AUTO(Transform, Scale, "Scale"),
        KL_METHOD_AUTO(Transform, ToString, "To string")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(Transform)
        KL_CTOR0(Transform),
        KL_CTOR(Transform, const Vector3D &, const Vector3D &, const Vector3D &),
        KL_CTOR(Transform, const Quaternion &, const Vector3D &, const Vector3D &),
        KL_CTOR(Transform, const Vector3D &, const Vector3D &, const Vector3D &, const Vector3D &, const Vector3D &),
        KL_CTOR(Transform, const Quaternion &, const Vector3D &, const Vector3D &, const Vector3D &, const Vector3D &),
        KL_CTOR(Transform, const Transform &)
    KL_END_DESCRIBE(Transform)

};

} // namespace koilo
