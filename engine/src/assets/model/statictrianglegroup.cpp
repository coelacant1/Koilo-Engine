#include <ptx/assets/model/statictrianglegroup.hpp>

StaticTriangleGroup::StaticTriangleGroup(Vector3D* vertices, const IndexGroup* indexGroup, int vertexCount, int triangleCount)
    : triangles(nullptr), vertices(vertices), indexGroup(indexGroup), uvIndexGroup(nullptr), uvVertices(nullptr), hasUV(false), vertexCount(vertexCount), triangleCount(triangleCount) {

    for (int i = 0; i < triangleCount; i++) {
        triangles[i].p1 = &vertices[indexGroup[i].A];
        triangles[i].p2 = &vertices[indexGroup[i].B];
        triangles[i].p3 = &vertices[indexGroup[i].C];
    }
}

StaticTriangleGroup::StaticTriangleGroup(Vector3D* vertices, const IndexGroup* indexGroup, const IndexGroup* uvIndexGroup, const Vector2D* uvVertices, int vertexCount, int triangleCount)
    : triangles(nullptr), vertices(vertices), indexGroup(indexGroup), uvIndexGroup(uvIndexGroup), uvVertices(uvVertices), hasUV(true), vertexCount(vertexCount), triangleCount(triangleCount) {

    for (int i = 0; i < triangleCount; i++) {
        triangles[i].p1 = &vertices[indexGroup[i].A];
        triangles[i].p2 = &vertices[indexGroup[i].B];
        triangles[i].p3 = &vertices[indexGroup[i].C];
    }
}

const bool StaticTriangleGroup::HasUV(){
    return hasUV;
}

const IndexGroup* StaticTriangleGroup::GetIndexGroup() {
    return indexGroup;
}

const int StaticTriangleGroup::GetTriangleCount() {
    return triangleCount;
}

Vector3D* StaticTriangleGroup::GetVertices() {
    return vertices;
}

const int StaticTriangleGroup::GetVertexCount() {
    return vertexCount;
}

Triangle3D* StaticTriangleGroup::GetTriangles() {
    return triangles;
}

const Vector2D* StaticTriangleGroup::GetUVVertices() {
    return uvVertices;
}

const IndexGroup* StaticTriangleGroup::GetUVIndexGroup() {
    return uvIndexGroup;
}
