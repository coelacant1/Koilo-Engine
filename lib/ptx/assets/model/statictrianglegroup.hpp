/**
 * @file StaticTriangleGroup.h
 * @brief Defines the StaticTriangleGroup class for managing 3D triangle geometry.
 *
 * The StaticTriangleGroup class represents a fixed collection of 3D triangles, 
 * vertices, and associated UV data. It provides methods to access and manipulate 
 * this data for rendering or simulation purposes.
 *
 * @date 22/12/2024
 * @version 1.0
 * @tparam vertexCount The number of vertices in the group.
 * @tparam triangleCount The number of triangles in the group.
 * @author Coela Can't
 */

#pragma once

#include "../../core/geometry/3d/triangle.hpp"
#include "indexgroup.hpp"
#include "istatictrianglegroup.hpp"
#include "../../registry/reflect_macros.hpp"

/**
 * @class StaticTriangleGroup
 * @brief Represents a static collection of 3D triangles and associated vertex/UV data.
 * @tparam vertexCount Number of vertices in the group.
 * @tparam triangleCount Number of triangles in the group.
 */
template<int vertexCount, int triangleCount>
class StaticTriangleGroup : public IStaticTriangleGroup {
private:
    Triangle3D triangles[triangleCount]; ///< Array of 3D triangles in the group.
    Vector3D* vertices; ///< Array of vertex positions.
    const IndexGroup* indexGroup; ///< Index group defining triangle vertex indices.
    const IndexGroup* uvIndexGroup; ///< Index group for UV coordinates (if available).
    const Vector2D* uvVertices; ///< Array of UV coordinates for texture mapping.
    const bool hasUV; ///< Indicates whether the group contains UV data.

public:
    /**
     * @brief Constructor for a group without UV data.
     * @param vertices Array of vertex positions.
     * @param indexGroup Index group defining triangle vertex indices.
     */
    StaticTriangleGroup(Vector3D* vertices, const IndexGroup* indexGroup);

    /**
     * @brief Constructor for a group with UV data.
     * @param vertices Array of vertex positions.
     * @param indexGroup Index group defining triangle vertex indices.
     * @param uvIndexGroup Index group for UV coordinates.
     * @param uvVertices Array of UV coordinates for texture mapping.
     */
    StaticTriangleGroup(Vector3D* vertices, const IndexGroup* indexGroup, const IndexGroup* uvIndexGroup, const Vector2D* uvVertices);

    /**
     * @brief Checks if the group has UV data.
     * @return True if UV data is present, otherwise false.
     */
    const bool HasUV() override;

    /**
     * @brief Retrieves the triangle index group.
     * @return Pointer to the IndexGroup defining triangle vertex indices.
     */
    const IndexGroup* GetIndexGroup() override;

    /**
     * @brief Gets the total number of triangles in the group.
     * @return The number of triangles.
     */
    const int GetTriangleCount() override;

    /**
     * @brief Retrieves the array of vertex positions.
     * @return Pointer to the array of vertices.
     */
    Vector3D* GetVertices() override;

    /**
     * @brief Gets the total number of vertices in the group.
     * @return The number of vertices.
     */
    const int GetVertexCount() override;

    /**
     * @brief Retrieves the array of triangles in the group.
     * @return Pointer to the array of triangles.
     */
    Triangle3D* GetTriangles() override;

    /**
     * @brief Retrieves the array of UV coordinates.
     * @return Pointer to the array of UV coordinates, or nullptr if not available.
     */
    const Vector2D* GetUVVertices() override;

    /**
     * @brief Retrieves the UV index group.
     * @return Pointer to the UV IndexGroup, or nullptr if not available.
     */
    const IndexGroup* GetUVIndexGroup() override;

    /* NOTE: StaticTriangleGroup is a template; verify macros accept template types. */
    PTX_BEGIN_FIELDS(StaticTriangleGroup)
        /* TODO: PTX_FIELD(StaticTriangleGroup, member, "Doc", min, max) */
    PTX_END_FIELDS
    
    PTX_BEGIN_METHODS(StaticTriangleGroup)
        PTX_METHOD_AUTO(StaticTriangleGroup, HasUV, "Has uv"),
        PTX_METHOD_AUTO(StaticTriangleGroup, GetIndexGroup, "Get index group"),
        PTX_METHOD_AUTO(StaticTriangleGroup, GetTriangleCount, "Get triangle count"),
        PTX_METHOD_AUTO(StaticTriangleGroup, GetVertices, "Get vertices"),
        PTX_METHOD_AUTO(StaticTriangleGroup, GetVertexCount, "Get vertex count"),
        PTX_METHOD_AUTO(StaticTriangleGroup, GetTriangles, "Get triangles"),
        PTX_METHOD_AUTO(StaticTriangleGroup, GetUVVertices, "Get uvvertices"),
        PTX_METHOD_AUTO(StaticTriangleGroup, GetUVIndexGroup, "Get uvindex group")
    PTX_END_METHODS
    
    PTX_BEGIN_DESCRIBE(StaticTriangleGroup)
        PTX_CTOR(StaticTriangleGroup, Vector3D *, const IndexGroup *),
        PTX_CTOR(StaticTriangleGroup, Vector3D *, const IndexGroup *, const IndexGroup *, const Vector2D *)
    PTX_END_DESCRIBE(StaticTriangleGroup)
    
};

#include "statictrianglegroup.tpp"
