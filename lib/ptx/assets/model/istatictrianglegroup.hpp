/**
 * @file IStaticTriangleGroup.h
 * @brief Defines the IStaticTriangleGroup interface for managing a collection of static 3D triangles.
 *
 * This interface provides methods to access and manipulate static triangle data, including vertices,
 * UV coordinates, and indices. It is useful for rendering, geometric calculations, and other 3D applications.
 *
 * @date 22/12/2024
 * @version 1.0
 * @author Coela Can't
 */

#pragma once

#include "../../core/math/vector2d.hpp"
#include "../../core/math/vector3d.hpp"
#include "../../core/geometry/3d/triangle.hpp"
#include "../../core/geometry/3d/triangle.hpp"
#include "indexgroup.hpp"
#include "../../registry/reflect_macros.hpp"

/**
 * @class IStaticTriangleGroup
 * @brief Interface for managing a static group of 3D triangles and associated data.
 */
class IStaticTriangleGroup {
public:
    /**
     * @brief Checks if the triangle group has UV coordinates.
     * @return True if UV coordinates are present, false otherwise.
     */
    virtual const bool HasUV() = 0;

    /**
     * @brief Retrieves the index group for the triangle group.
     * @return A pointer to the IndexGroup array.
     */
    virtual const IndexGroup* GetIndexGroup() = 0;

    /**
     * @brief Retrieves the total number of triangles in the group.
     * @return The number of triangles.
     */
    virtual const int GetTriangleCount() = 0;

    /**
     * @brief Retrieves the array of vertices in the triangle group.
     * @return A pointer to the array of Vector3D vertices.
     */
    virtual const Vector3D* GetVertices() = 0;

    /**
     * @brief Retrieves the total number of vertices in the group.
     * @return The number of vertices.
     */
    virtual const int GetVertexCount() = 0;

    /**
     * @brief Retrieves the array of Triangle3D objects representing the triangles.
     * @return A pointer to the array of Triangle3D objects.
     */
    virtual Triangle3D* GetTriangles() = 0;

    /**
     * @brief Retrieves the array of UV vertices in the triangle group.
     * @return A pointer to the array of Vector2D UV vertices.
     */
    virtual const Vector2D* GetUVVertices() = 0;

    /**
     * @brief Retrieves the index group for the UV vertices.
     * @return A pointer to the IndexGroup array for UV vertices.
     */
    virtual const IndexGroup* GetUVIndexGroup() = 0;

    PTX_BEGIN_FIELDS(IStaticTriangleGroup)
        /* TODO: PTX_FIELD(IStaticTriangleGroup, member, "Doc", min, max) */
    PTX_END_FIELDS
    
    PTX_BEGIN_METHODS(IStaticTriangleGroup)
        PTX_METHOD_AUTO(IStaticTriangleGroup, HasUV, "Has uv"),
        PTX_METHOD_AUTO(IStaticTriangleGroup, GetIndexGroup, "Get index group"),
        PTX_METHOD_AUTO(IStaticTriangleGroup, GetTriangleCount, "Get triangle count"),
        PTX_METHOD_AUTO(IStaticTriangleGroup, GetVertices, "Get vertices"),
        PTX_METHOD_AUTO(IStaticTriangleGroup, GetVertexCount, "Get vertex count"),
        PTX_METHOD_AUTO(IStaticTriangleGroup, GetTriangles, "Get triangles"),
        PTX_METHOD_AUTO(IStaticTriangleGroup, GetUVVertices, "Get uvvertices"),
        PTX_METHOD_AUTO(IStaticTriangleGroup, GetUVIndexGroup, "Get uvindex group")
    PTX_END_METHODS
    
    PTX_BEGIN_DESCRIBE(IStaticTriangleGroup)
        /* TODO: PTX_CTOR0(IStaticTriangleGroup) or PTX_CTOR(IStaticTriangleGroup, ...) */
    PTX_END_DESCRIBE(IStaticTriangleGroup)
    
};
