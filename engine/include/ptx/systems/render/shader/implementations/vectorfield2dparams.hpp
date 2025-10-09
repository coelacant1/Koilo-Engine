// vectorfield2dparams.hpp
#pragma once

#include <cstdint>
#include <vector>
#include "../../../../core/math/vector2d.hpp"
#include "../../../../registry/reflect_macros.hpp"

/**
 * @file vectorfield2dparams.hpp
 * @brief Raw state for a 2D vector field grid using contiguous std::vector storage.
 *
 * Holds fixed-size grids of X/Y vector components and a scalar density channel
 * for both "previous" and "current" states. Memory is allocated in the ctor and
 * freed in the dtor.
 */
struct VectorField2DParams {
    Vector2D size      {0.0f, 0.0f};  ///< Logical width/height in world units.
    Vector2D position  {0.0f, 0.0f};  ///< Center position in world units.
    float    rotation  {0.0f};        ///< Rotation in degrees.
    bool     showDensity {true};      ///< Optional UI/visualization flag.

    const uint16_t countX;            ///< Grid resolution in X (columns).
    const uint16_t countY;            ///< Grid resolution in Y (rows).

    std::vector<int8_t> vecXP;  ///< Previous X component buffer (size = countX*countY).
    std::vector<int8_t> vecYP;  ///< Previous Y component buffer (size = countX*countY).
    std::vector<int8_t> vecDP;  ///< Previous density buffer     (size = countX*countY).
    std::vector<int8_t> vecX;   ///< Current  X component buffer (size = countX*countY).
    std::vector<int8_t> vecY;   ///< Current  Y component buffer (size = countX*countY).
    std::vector<int8_t> vecD;   ///< Current  density buffer     (size = countX*countY).

    /**
     * @brief Allocate buffers for a countX × countY grid.
     * @param x Number of columns.
     * @param y Number of rows.
     */
        explicit VectorField2DParams(uint16_t x, uint16_t y)
                : countX(x), countY(y),
                    vecXP(static_cast<size_t>(countX) * countY, 0),
                    vecYP(static_cast<size_t>(countX) * countY, 0),
                    vecDP(static_cast<size_t>(countX) * countY, 0),
                    vecX (static_cast<size_t>(countX) * countY, 0),
                    vecY (static_cast<size_t>(countX) * countY, 0),
                    vecD (static_cast<size_t>(countX) * countY, 0) {}

    PTX_BEGIN_FIELDS(VectorField2DParams)
        PTX_FIELD(VectorField2DParams, size, "Size", 0, 0),
        PTX_FIELD(VectorField2DParams, position, "Position", 0, 0),
        PTX_FIELD(VectorField2DParams, rotation, "Rotation", __FLT_MIN__, __FLT_MAX__),
        PTX_FIELD(VectorField2DParams, showDensity, "Show density", 0, 1),
        PTX_FIELD(VectorField2DParams, countX, "Count x", 0, 65535),
        PTX_FIELD(VectorField2DParams, countY, "Count y", 0, 65535),
        PTX_FIELD(VectorField2DParams, vecXP, "Vec xp", -128, 127),
        PTX_FIELD(VectorField2DParams, vecYP, "Vec yp", -128, 127),
        PTX_FIELD(VectorField2DParams, vecDP, "Vec dp", -128, 127),
        PTX_FIELD(VectorField2DParams, vecX, "Vec x", -128, 127),
        PTX_FIELD(VectorField2DParams, vecY, "Vec y", -128, 127),
        PTX_FIELD(VectorField2DParams, vecD, "Vec d", -128, 127)
    PTX_END_FIELDS

    PTX_BEGIN_METHODS(VectorField2DParams)
        /* No reflected methods. */
    PTX_END_METHODS

    PTX_BEGIN_DESCRIBE(VectorField2DParams)
        PTX_CTOR(VectorField2DParams, uint16_t, uint16_t)
    PTX_END_DESCRIBE(VectorField2DParams)

};
