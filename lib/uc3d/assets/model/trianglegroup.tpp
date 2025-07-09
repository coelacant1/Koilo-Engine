#pragma once

template<int vertexCount, int triangleCount>
TriangleGroup<vertexCount, triangleCount>::TriangleGroup(IStaticTriangleGroup* staticTriangleGroup) {
    indexGroup = staticTriangleGroup->GetIndexGroup();

    for (int i = 0; i < vertexCount; i++) {
        vertices[i] = staticTriangleGroup->GetVertices()[i];
    }

    for (int i = 0; i < triangleCount; i++) {
        triangles[i].p1 = &vertices[indexGroup[i].A];
        triangles[i].p2 = &vertices[indexGroup[i].B];
        triangles[i].p3 = &vertices[indexGroup[i].C];
    }
}

template<int vertexCount, int triangleCount>
const IndexGroup* TriangleGroup<vertexCount, triangleCount>::GetIndexGroup() {
    return indexGroup;
}

template<int vertexCount, int triangleCount>
int TriangleGroup<vertexCount, triangleCount>::GetTriangleCount() {
    return triangleCount;
}

template<int vertexCount, int triangleCount>
Vector3D* TriangleGroup<vertexCount, triangleCount>::GetVertices() {
    return vertices;
}

template<int vertexCount, int triangleCount>
int TriangleGroup<vertexCount, triangleCount>::GetVertexCount() {
    return vertexCount;
}

template<int vertexCount, int triangleCount>
Triangle3D* TriangleGroup<vertexCount, triangleCount>::GetTriangles() {
    return triangles;
}
