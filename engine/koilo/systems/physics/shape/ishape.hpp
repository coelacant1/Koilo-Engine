// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file ishape.hpp
 * @brief Abstract shape interface consumed by the broadphase + narrowphase.
 *
 * Decouples geometric primitives from `Collider`. `ColliderProxy` instances
 * are populated from existing colliders.
 */

#pragma once

#include <koilo/systems/physics/bodypose.hpp>
#include <koilo/core/geometry/3d/aabb.hpp>
#include <koilo/core/math/vector3d.hpp>
#include <cstdint>

namespace koilo {

/**
 * @enum ShapeType
 * @brief Concrete shape kind. Stable wire identifier - extend at the end only.
 */
enum class ShapeType : std::uint8_t {
    Sphere       = 0,
    Box          = 1,
    Capsule      = 2,
    ConvexHull   = 3,
    Plane        = 4,
    TriangleMesh = 5,
    HeightField  = 6,
    Custom       = 255
};

/**
 * @class IShape
 * @brief Geometric query interface used by collision detection.
 *
 * Implementations are pure data + closed-form queries. World-space queries
 * receive a BodyPose so the shape itself can stay in local space.
 */
class IShape {
public:
    virtual ~IShape() = default;

    /** Concrete shape kind for fast-path dispatch. */
    virtual ShapeType Type() const = 0;

    /** Convex shapes support GJK/EPA narrowphase. Concave shapes do not. */
    virtual bool IsConvex() const { return true; }

    /** Local-space AABB (no transform applied). */
    virtual AABB LocalAABB() const = 0;

    /** World-space AABB after applying the given body pose. */
    virtual AABB WorldAABB(const BodyPose& pose) const = 0;

    /**
     * @brief Support point in world space along @p dirWorld.
     *
     * Required for GJK/EPA. For non-convex shapes, may return an arbitrary
     * point - narrowphase must check IsConvex() before calling.
     */
    virtual Vector3D Support(const Vector3D& dirWorld, const BodyPose& pose) const = 0;
};

} // namespace koilo
