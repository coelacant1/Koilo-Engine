// SPDX-License-Identifier: GPL-3.0-or-later
#include <koilo/systems/scene/primitivemesh.hpp>

namespace koilo {

PrimitiveMesh::PrimitiveMesh() = default;

PrimitiveMesh::~PrimitiveMesh() {
    Cleanup();
}

void PrimitiveMesh::Cleanup() {
    mesh_.reset();
    workingGeometry_.reset();
    baseGeometry_.reset();
    vertices_.clear();
    indices_.clear();
    uvVertices_.clear();
    uvIndices_.clear();
}

void PrimitiveMesh::CreateQuad(float width, float height) {
    Cleanup();

    float hw = width * 0.5f;
    float hh = height * 0.5f;

    // 4 vertices: centered at origin, facing +Z
    vertices_.resize(4);
    vertices_[0] = Vector3D(-hw, -hh, 0.0f);  // bottom-left
    vertices_[1] = Vector3D( hw, -hh, 0.0f);  // bottom-right
    vertices_[2] = Vector3D( hw,  hh, 0.0f);  // top-right
    vertices_[3] = Vector3D(-hw,  hh, 0.0f);  // top-left

    // 2 triangles
    indices_.resize(2);
    indices_[0] = IndexGroup(0, 1, 2);  // bottom-right tri
    indices_[1] = IndexGroup(0, 2, 3);  // top-left tri

    baseGeometry_ = std::make_unique<StaticTriangleGroup>(vertices_.data(), indices_.data(), 4, 2);
    workingGeometry_ = std::make_unique<TriangleGroup>(baseGeometry_.get());
    mesh_ = std::make_unique<Mesh>(baseGeometry_.get(), workingGeometry_.get(), nullptr);
}

void PrimitiveMesh::CreateTexturedQuad(float width, float height) {
    Cleanup();

    float hw = width * 0.5f;
    float hh = height * 0.5f;

    vertices_.resize(4);
    vertices_[0] = Vector3D(-hw, -hh, 0.0f);  // bottom-left
    vertices_[1] = Vector3D( hw, -hh, 0.0f);  // bottom-right
    vertices_[2] = Vector3D( hw,  hh, 0.0f);  // top-right
    vertices_[3] = Vector3D(-hw,  hh, 0.0f);  // top-left

    indices_.resize(2);
    indices_[0] = IndexGroup(0, 1, 2);
    indices_[1] = IndexGroup(0, 2, 3);

    // UV coordinates: (0,1)=bottom-left, (1,0)=top-right
    uvVertices_.resize(4);
    uvVertices_[0] = Vector2D(0.0f, 1.0f);  // bottom-left
    uvVertices_[1] = Vector2D(1.0f, 1.0f);  // bottom-right
    uvVertices_[2] = Vector2D(1.0f, 0.0f);  // top-right
    uvVertices_[3] = Vector2D(0.0f, 0.0f);  // top-left

    // UV indices match vertex indices
    uvIndices_.resize(2);
    uvIndices_[0] = IndexGroup(0, 1, 2);
    uvIndices_[1] = IndexGroup(0, 2, 3);

    baseGeometry_ = std::make_unique<StaticTriangleGroup>(
        vertices_.data(), indices_.data(), uvIndices_.data(),
        uvVertices_.data(), 4, 2);
    workingGeometry_ = std::make_unique<TriangleGroup>(baseGeometry_.get());
    mesh_ = std::make_unique<Mesh>(baseGeometry_.get(), workingGeometry_.get(), nullptr);
}

bool PrimitiveMesh::HasUV() const {
    return !uvVertices_.empty();
}

Mesh* PrimitiveMesh::GetMesh() {
    return mesh_.get();
}

} // namespace koilo
