// SPDX-License-Identifier: GPL-3.0-or-later
#include <koilo/assets/model/statictrianglegroup.hpp>


namespace koilo {

koilo::StaticTriangleGroup::StaticTriangleGroup(Vector3D* vertices, const IndexGroup* indexGroup, uint32_t vertexCount, uint32_t triangleCount)
    : triangles(triangleCount), vertices(vertices), indexGroup(indexGroup), uvIndexGroup(nullptr), uvVertices(nullptr), hasUV(false), vertexCount(vertexCount), triangleCount(triangleCount) {

    for (uint32_t i = 0; i < triangleCount; i++) {
        triangles[i].p1 = &vertices[indexGroup[i].A];
        triangles[i].p2 = &vertices[indexGroup[i].B];
        triangles[i].p3 = &vertices[indexGroup[i].C];
    }
}

koilo::StaticTriangleGroup::StaticTriangleGroup(Vector3D* vertices, const IndexGroup* indexGroup, const IndexGroup* uvIndexGroup, const Vector2D* uvVertices, uint32_t vertexCount, uint32_t triangleCount)
    : triangles(triangleCount), vertices(vertices), indexGroup(indexGroup), uvIndexGroup(uvIndexGroup), uvVertices(uvVertices), hasUV(true), vertexCount(vertexCount), triangleCount(triangleCount) {

    for (uint32_t i = 0; i < triangleCount; i++) {
        triangles[i].p1 = &vertices[indexGroup[i].A];
        triangles[i].p2 = &vertices[indexGroup[i].B];
        triangles[i].p3 = &vertices[indexGroup[i].C];
    }
}

bool koilo::StaticTriangleGroup::HasUV(){
    return hasUV;
}

const IndexGroup* koilo::StaticTriangleGroup::GetIndexGroup() {
    return indexGroup;
}

uint32_t koilo::StaticTriangleGroup::GetTriangleCount() {
    return triangleCount;
}

Vector3D* koilo::StaticTriangleGroup::GetVertices() {
    return vertices;
}

uint32_t koilo::StaticTriangleGroup::GetVertexCount() {
    return vertexCount;
}

Triangle3D* koilo::StaticTriangleGroup::GetTriangles() {
    return triangles.data();
}

const Vector2D* koilo::StaticTriangleGroup::GetUVVertices() {
    return uvVertices;
}

const IndexGroup* koilo::StaticTriangleGroup::GetUVIndexGroup() {
    return uvIndexGroup;
}

} // namespace koilo
