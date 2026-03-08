// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file cube.hpp
 * @brief Defines the Cube geometry primitive.
 *
 * Pure geometric representation of an axis-aligned box (position + extents).
 *
 * @date 22/12/2024
 * @version 2.0
 * @author Coela Can't
 */

#pragma once

#include <koilo/core/math/vector3d.hpp>
#include <koilo/registry/reflect_macros.hpp>


namespace koilo {

/**
 * @class Cube
 * @brief Axis-aligned box defined by center position and extents.
 */
class Cube {
private:
    Vector3D centerPosition; ///< Center position of the bounding cube.
    Vector3D maximum; ///< Maximum coordinates of the bounding cube.
    Vector3D minimum; ///< Minimum coordinates of the bounding cube.

public:
    Vector3D position = Vector3D(0, 0, 0); ///< Mutable position offset (used by colliders).

    /**
     * @brief Constructs a Cube object with a specified center and size.
     * @param centerPosition Center position of the cube.
     * @param objectSize Size of the cube.
     */
    Cube(Vector3D centerPosition, Vector3D objectSize);

    /**
     * @brief Retrieves the center position of the cube.
     * @return The center position as a Vector3D.
     */
    Vector3D GetPosition();

    /**
     * @brief Retrieves the size of the cube.
     * @return The size as a Vector3D.
     */
    Vector3D GetSize();

    /**
     * @brief Retrieves the maximum coordinates of the cube.
     * @return The maximum coordinates as a Vector3D.
     */
    Vector3D GetMaximum();

    /**
     * @brief Retrieves the minimum coordinates of the cube.
     * @return The minimum coordinates as a Vector3D.
     */
    Vector3D GetMinimum();

    KL_BEGIN_FIELDS(Cube)
        KL_FIELD(Cube, position, "Position", 0, 0)
    KL_END_FIELDS

    KL_BEGIN_METHODS(Cube)
        KL_METHOD_AUTO(Cube, GetPosition, "Get position"),
        KL_METHOD_AUTO(Cube, GetSize, "Get size"),
        KL_METHOD_AUTO(Cube, GetMaximum, "Get maximum"),
        KL_METHOD_AUTO(Cube, GetMinimum, "Get minimum")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(Cube)
        KL_CTOR(Cube, Vector3D, Vector3D)
    KL_END_DESCRIBE(Cube)

};

} // namespace koilo
