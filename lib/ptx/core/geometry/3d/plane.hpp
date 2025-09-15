/**
 * @file Plane.h
 * @brief Defines the Plane class for representing a plane in 3D space.
 *
 * The Plane class represents a mathematical plane defined by a centroid and a normal vector.
 *
 * @date 22/12/2024
 * @version 1.0
 * @author Coela Can't
 */

#pragma once

#include "../../math/mathematics.hpp"
#include "../../math/vector3d.hpp"
#include "../../../registry/reflect_macros.hpp"

/**
 * @class Plane
 * @brief Represents a plane in 3D space defined by a centroid and a normal vector.
 *
 * The `Plane` class provides methods for initializing and describing a plane
 * in three-dimensional space using a point on the plane (centroid) and a normal vector.
 */
class Plane {
public:
    Vector3D Centroid; ///< Point on the plane, representing the centroid.
    Vector3D Normal;   ///< Normal vector defining the plane's orientation.

    /**
     * @brief Default constructor.
     *
     * Initializes the plane with a default centroid and normal vector.
     */
    Plane();

    /**
     * @brief Parameterized constructor.
     *
     * Initializes the plane with the specified centroid and normal vector.
     *
     * @param centroid A point on the plane (centroid).
     * @param normal A vector normal to the plane.
     */
    Plane(Vector3D centroid, Vector3D normal);

    /**
     * @brief Converts the Plane object to a string representation.
     *
     * @return A string describing the plane's centroid and normal vector.
     */
    ptx::UString ToString();

    PTX_BEGIN_FIELDS(Plane)
        PTX_FIELD(Plane, Centroid, "Centroid", 0, 0),
        PTX_FIELD(Plane, Normal, "Normal", 0, 0)
    PTX_END_FIELDS
    
    PTX_BEGIN_METHODS(Plane)
        PTX_METHOD_AUTO(Plane, ToString, "To string")
    PTX_END_METHODS
    
    PTX_BEGIN_DESCRIBE(Plane)
        PTX_CTOR0(Plane),
        PTX_CTOR(Plane, Vector3D, Vector3D)
    PTX_END_DESCRIBE(Plane)
    
};
