/**
 * @file rastertriangle3d.hpp
 * @brief Triangle prepared for perspective-correct rasterization.
 * @date  18/06/2025
 */
#pragma once

#include "../../../geometry/3d/triangle.hpp"
#include "../../../render/material/material.hpp"
#include "../../../math/vector3d.hpp"

/**
 * @class RasterTriangle3D
 * @brief Stores edge equations, depth, material for the rasterizer.
 */
class RasterTriangle3D
{
public:
    Triangle3D geo;           ///< geometry triangle
    float oneOverZ1;          ///< 1/z for vertex 1
    float oneOverZ2;          ///< 1/z for vertex 2
    float oneOverZ3;          ///< 1/z for vertex 3
    Material* mat;            ///< surface material

    /** @brief Build from geometry + depth + material. */
    RasterTriangle3D(const Triangle3D& g,
                     float z1, float z2, float z3,
                     Material* m);

    /**
     * @brief Fast plane depth at pixel (x,y).
     * @param x screen x
     * @param y screen y
     * @return interpolated 1/z value
     */
    float DepthAt(float x, float y) const;

private:
    float a;   ///< plane A coeff
    float b;   ///< plane B coeff
    float c;   ///< plane C coeff
};
