/**
 * @file TriangleGroup.h
 * @brief Defines the TriangleGroup class for dynamic manipulation of triangles in 3D space.
 *
 * The TriangleGroup class represents a group of triangles and provides methods for
 * accessing and managing their vertices, indices, and optional UV mappings.
 *
 * @date 22/12/2024
 * @version 1.0
 * @author Coela Can't
 */

#pragma once

#include <vector>
#include "../../core/geometry/3d/triangle.hpp"
#include "itrianglegroup.hpp"
#include "indexgroup.hpp"
#include "istatictrianglegroup.hpp"
#include "../../registry/reflect_macros.hpp"

/**
 * @class TriangleGroup
 * @brief Represents a dynamic (runtime-sized) group of 3D triangles.
 *
 * This runtime variant replaces an earlier compile‑time templated version. All
 * triangles own vertex-pointer wiring into the internally owned vertex array.
 *
 * Invariants:
 *  - `vertices` owns exactly `GetVertexCount()` elements.
 *  - `triangles` owns exactly `GetTriangleCount()` elements.
 *  - Each Triangle3D's `a`, `b`, `c` pointers either point into `vertices` or are nullptr during construction only.
 *  - `indexGroup` is non-owning; lifetime must exceed this group or remain valid externally.
 */
class TriangleGroup : public ITriangleGroup {
private:
    std::vector<Triangle3D> triangles;   ///< Owning storage of triangles.
    std::vector<Vector3D>  vertices;    ///< Owning storage of vertex positions.
    const IndexGroup* indexGroup = nullptr; ///< Non-owning pointer to source index group (indices into `vertices`).

public:
    /**
     * @brief Construct from a static triangle group.
     * @details Copies vertex data from the supplied static group, stores a pointer to
     *          its IndexGroup (non-owning), allocates Triangle3D objects and rewires
     *          their vertex pointers according to the indices. The static group must
     *          outlive this instance if the index group is reused elsewhere.
     * @param staticTriangleGroup Source static group (must not be null).
     */
    explicit TriangleGroup(IStaticTriangleGroup* staticTriangleGroup);

    ~TriangleGroup() override = default;

    /** @brief Access the underlying (non-owning) index group. */
    const IndexGroup* GetIndexGroup() override;

    /** @brief Number of triangles stored. */
    int GetTriangleCount() override;

    /** @brief Mutable pointer to contiguous vertex storage (size == GetVertexCount()). */
    Vector3D* GetVertices() override;

    /** @brief Number of vertices stored. */
    int GetVertexCount() override;

    /** @brief Mutable pointer to first Triangle3D (size == GetTriangleCount()). */
    Triangle3D* GetTriangles() override;

    PTX_BEGIN_FIELDS(TriangleGroup)
        /* No reflected fields. */
    PTX_END_FIELDS

    PTX_BEGIN_METHODS(TriangleGroup)
        PTX_METHOD_AUTO(TriangleGroup, GetIndexGroup, "Get index group"),
        PTX_METHOD_AUTO(TriangleGroup, GetTriangleCount, "Get triangle count"),
        PTX_METHOD_AUTO(TriangleGroup, GetVertices, "Get vertices"),
        PTX_METHOD_AUTO(TriangleGroup, GetVertexCount, "Get vertex count"),
        PTX_METHOD_AUTO(TriangleGroup, GetTriangles, "Get triangles")
    PTX_END_METHODS

    PTX_BEGIN_DESCRIBE(TriangleGroup)
        PTX_CTOR(TriangleGroup, IStaticTriangleGroup *)
    PTX_END_DESCRIBE(TriangleGroup)

};

