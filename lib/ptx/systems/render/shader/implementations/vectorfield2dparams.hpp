// vectorfield2dparams.hpp
#pragma once

#include <cstdint>
#include "../../../../core/math/vector2d.hpp"

/**
 * @file vectorfield2dparams.hpp
 * @brief Raw state for a 2D vector field grid (owns plain arrays, no std::vector).
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

    int8_t* vecXP;  ///< Previous X component buffer (size = countX*countY).
    int8_t* vecYP;  ///< Previous Y component buffer (size = countX*countY).
    int8_t* vecDP;  ///< Previous density buffer     (size = countX*countY).
    int8_t* vecX;   ///< Current  X component buffer (size = countX*countY).
    int8_t* vecY;   ///< Current  Y component buffer (size = countX*countY).
    int8_t* vecD;   ///< Current  density buffer     (size = countX*countY).

    /**
     * @brief Allocate buffers for a countX × countY grid.
     * @param x Number of columns.
     * @param y Number of rows.
     */
    explicit VectorField2DParams(uint16_t x, uint16_t y)
        : countX(x), countY(y),
          vecXP(new int8_t[x*y]), vecYP(new int8_t[x*y]), vecDP(new int8_t[x*y]),
          vecX (new int8_t[x*y]), vecY (new int8_t[x*y]), vecD (new int8_t[x*y]) {}

    /**
     * @brief Free all owned buffers.
     */
    ~VectorField2DParams() {
        delete[] vecXP; delete[] vecYP; delete[] vecDP;
        delete[] vecX;  delete[] vecY;  delete[] vecD;
    }
};
