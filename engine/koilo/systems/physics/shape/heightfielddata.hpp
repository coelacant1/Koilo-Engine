// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file heightfielddata.hpp
 * @brief Shared static heightfield geometry.
 *
 * Regular grid of `(widthCells+1) x (depthCells+1)` height samples spaced
 * `cellSize` apart in X and Z. The grid is centered at the local origin so
 * vertex (i, j) lives at:
 *   x = (i - widthCells * 0.5) * cellSize
 *   z = (j - depthCells * 0.5) * cellSize
 *   y = heights[j * (widthCells + 1) + i]
 *
 * Held by `std::shared_ptr<const HeightfieldData>` so multiple
 * HeightfieldColliders can share one sample array (e.g. instanced terrain
 * tiles). Build() validates dimensions and computes the local AABB; queries
 * are constant-time per cell so no acceleration structure is needed.
 *
 * Each cell (i, j) is split into two triangles whose CCW winding gives a
 * +Y face normal on flat terrain:
 *   t0 = (v00, v11, v10)
 *   t1 = (v00, v01, v11)
 * where v00, v10, v11, v01 are the (i,j), (i+1,j), (i+1,j+1), (i,j+1)
 * corner vertices respectively.
 */

#pragma once

#include <koilo/core/geometry/3d/aabb.hpp>
#include <koilo/core/math/vector3d.hpp>

#include <algorithm>
#include <cstdint>
#include <vector>

namespace koilo {

struct HeightfieldData {
    std::uint32_t      widthCells = 0;   ///< Number of cells in X (vertices = widthCells+1).
    std::uint32_t      depthCells = 0;   ///< Number of cells in Z (vertices = depthCells+1).
    float              cellSize   = 1.0f;///< Uniform grid spacing in X and Z.
    std::vector<float> heights;          ///< Row-major over Z then X; size = (W+1)*(D+1).
    AABB               localAabb;        ///< Computed in Build().
    bool               valid      = false;

    /** Recomputes localAabb and validates dimensions / sample count. */
    void Build() {
        valid = false;
        if (widthCells == 0 || depthCells == 0 || cellSize <= 0.0f) {
            localAabb = AABB();
            return;
        }
        const std::size_t expected = static_cast<std::size_t>(widthCells + 1u)
                                   * static_cast<std::size_t>(depthCells + 1u);
        if (heights.size() != expected) {
            localAabb = AABB();
            return;
        }
        const float halfW = widthCells * 0.5f * cellSize;
        const float halfD = depthCells * 0.5f * cellSize;
        float minY = heights[0], maxY = heights[0];
        for (float h : heights) {
            minY = std::min(minY, h);
            maxY = std::max(maxY, h);
        }
        localAabb = AABB(Vector3D(-halfW, minY, -halfD),
                         Vector3D( halfW, maxY,  halfD));
        valid = true;
    }

    bool Empty() const { return !valid; }

    /** Local-space X coordinate of vertex column `i`. */
    inline float VertexX(std::uint32_t i) const {
        return (static_cast<float>(i) - widthCells * 0.5f) * cellSize;
    }
    /** Local-space Z coordinate of vertex row `j`. */
    inline float VertexZ(std::uint32_t j) const {
        return (static_cast<float>(j) - depthCells * 0.5f) * cellSize;
    }
    /** Local-space height sample at vertex (i, j). */
    inline float HeightAt(std::uint32_t i, std::uint32_t j) const {
        return heights[static_cast<std::size_t>(j) * (widthCells + 1u) + i];
    }
    /** Local-space vertex position at corner (i, j). */
    inline Vector3D Vertex(std::uint32_t i, std::uint32_t j) const {
        return Vector3D(VertexX(i), HeightAt(i, j), VertexZ(j));
    }
};

} // namespace koilo
