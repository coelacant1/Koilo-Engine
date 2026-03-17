// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file primitivemesh.hpp
 * @brief Generates simple primitive geometry (quads, cubes, etc.) for script use.
 *
 * Follows the same pattern as MorphableMesh: construct, generate, then GetMesh().
 */

#pragma once

#include <koilo/systems/scene/mesh.hpp>
#include <koilo/assets/model/statictrianglegroup.hpp>
#include <koilo/assets/model/trianglegroup.hpp>
#include <koilo/assets/model/indexgroup.hpp>
#include <koilo/core/math/vector3d.hpp>
#include <koilo/core/math/vector2d.hpp>
#include <koilo/registry/reflect_macros.hpp>
#include <memory>
#include <vector>

namespace koilo {

class PrimitiveMesh {
public:
    PrimitiveMesh();
    ~PrimitiveMesh();

    // Create a quad (2 triangles, 4 vertices) centered at origin, without UVs.
    void CreateQuad(float width, float height);

    // Create a quad with UVs (0,0)-(1,1) for texture mapping.
    void CreateTexturedQuad(float width, float height);

    // Get the underlying Mesh (returns nullptr if not yet created).
    Mesh* GetMesh();

    // Check if the current mesh has UV data.
    bool HasUV() const;

private:
    std::unique_ptr<Mesh> mesh_;
    std::unique_ptr<StaticTriangleGroup> baseGeometry_;
    std::unique_ptr<TriangleGroup> workingGeometry_;
    std::vector<Vector3D> vertices_;
    std::vector<IndexGroup> indices_;
    std::vector<Vector2D> uvVertices_;
    std::vector<IndexGroup> uvIndices_;

    void Cleanup();

    KL_BEGIN_FIELDS(PrimitiveMesh)
    KL_END_FIELDS

    KL_BEGIN_METHODS(PrimitiveMesh)
        KL_METHOD_AUTO(PrimitiveMesh, CreateQuad, "Create quad mesh"),
        KL_METHOD_AUTO(PrimitiveMesh, CreateTexturedQuad, "Create textured quad"),
        KL_METHOD_AUTO(PrimitiveMesh, GetMesh, "Get mesh"),
        KL_METHOD_AUTO(PrimitiveMesh, HasUV, "Has UV data")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(PrimitiveMesh)
        KL_CTOR0(PrimitiveMesh)
    KL_END_DESCRIBE(PrimitiveMesh)
};

} // namespace koilo
