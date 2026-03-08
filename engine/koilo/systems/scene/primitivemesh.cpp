// SPDX-License-Identifier: GPL-3.0-or-later
#include <koilo/systems/scene/primitivemesh.hpp>

namespace koilo {

PrimitiveMesh::PrimitiveMesh() = default;

PrimitiveMesh::~PrimitiveMesh() {
    Cleanup();
}

void PrimitiveMesh::Cleanup() {
    delete mesh_;
    delete workingGeometry_;
    delete baseGeometry_;
    delete[] vertices_;
    delete[] indices_;
    delete[] uvVertices_;
    delete[] uvIndices_;
    mesh_ = nullptr;
    workingGeometry_ = nullptr;
    baseGeometry_ = nullptr;
    vertices_ = nullptr;
    indices_ = nullptr;
    uvVertices_ = nullptr;
    uvIndices_ = nullptr;
}

void PrimitiveMesh::CreateQuad(float width, float height) {
    Cleanup();

    float hw = width * 0.5f;
    float hh = height * 0.5f;

    // 4 vertices: centered at origin, facing +Z
    vertices_ = new Vector3D[4];
    vertices_[0] = Vector3D(-hw, -hh, 0.0f);  // bottom-left
    vertices_[1] = Vector3D( hw, -hh, 0.0f);  // bottom-right
    vertices_[2] = Vector3D( hw,  hh, 0.0f);  // top-right
    vertices_[3] = Vector3D(-hw,  hh, 0.0f);  // top-left

    // 2 triangles
    indices_ = new IndexGroup[2];
    indices_[0] = IndexGroup(0, 1, 2);  // bottom-right tri
    indices_[1] = IndexGroup(0, 2, 3);  // top-left tri

    baseGeometry_ = new StaticTriangleGroup(vertices_, indices_, 4, 2);
    workingGeometry_ = new TriangleGroup(baseGeometry_);
    mesh_ = new Mesh(baseGeometry_, workingGeometry_, nullptr);
}

void PrimitiveMesh::CreateTexturedQuad(float width, float height) {
    Cleanup();

    float hw = width * 0.5f;
    float hh = height * 0.5f;

    vertices_ = new Vector3D[4];
    vertices_[0] = Vector3D(-hw, -hh, 0.0f);  // bottom-left
    vertices_[1] = Vector3D( hw, -hh, 0.0f);  // bottom-right
    vertices_[2] = Vector3D( hw,  hh, 0.0f);  // top-right
    vertices_[3] = Vector3D(-hw,  hh, 0.0f);  // top-left

    indices_ = new IndexGroup[2];
    indices_[0] = IndexGroup(0, 1, 2);
    indices_[1] = IndexGroup(0, 2, 3);

    // UV coordinates: (0,1)=bottom-left, (1,0)=top-right
    uvVertices_ = new Vector2D[4];
    uvVertices_[0] = Vector2D(0.0f, 1.0f);  // bottom-left
    uvVertices_[1] = Vector2D(1.0f, 1.0f);  // bottom-right
    uvVertices_[2] = Vector2D(1.0f, 0.0f);  // top-right
    uvVertices_[3] = Vector2D(0.0f, 0.0f);  // top-left

    // UV indices match vertex indices
    uvIndices_ = new IndexGroup[2];
    uvIndices_[0] = IndexGroup(0, 1, 2);
    uvIndices_[1] = IndexGroup(0, 2, 3);

    baseGeometry_ = new StaticTriangleGroup(vertices_, indices_, uvIndices_, uvVertices_, 4, 2);
    workingGeometry_ = new TriangleGroup(baseGeometry_);
    mesh_ = new Mesh(baseGeometry_, workingGeometry_, nullptr);
}

bool PrimitiveMesh::HasUV() const {
    return uvVertices_ != nullptr;
}

Mesh* PrimitiveMesh::GetMesh() {
    return mesh_;
}

} // namespace koilo
