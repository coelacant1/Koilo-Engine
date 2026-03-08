#include <ptx/assets/model/trianglegroup.hpp>
#include <algorithm>

TriangleGroup::TriangleGroup(IStaticTriangleGroup* staticTriangleGroup) {
    if (!staticTriangleGroup) {
        return;
    }
    indexGroup = staticTriangleGroup->GetIndexGroup();
    const int vCount = staticTriangleGroup->GetVertexCount();
    const int tCount = staticTriangleGroup->GetTriangleCount();

    if (vCount > 0) {
        vertices.resize(static_cast<size_t>(vCount));
        const Vector3D* srcVerts = staticTriangleGroup->GetVertices();
        std::copy(srcVerts, srcVerts + vCount, vertices.begin());
    }

    if (tCount > 0 && indexGroup) {
        triangles.resize(static_cast<size_t>(tCount));
        for (int i = 0; i < tCount; ++i) {
            triangles[static_cast<size_t>(i)].p1 = &vertices[indexGroup[i].A];
            triangles[static_cast<size_t>(i)].p2 = &vertices[indexGroup[i].B];
            triangles[static_cast<size_t>(i)].p3 = &vertices[indexGroup[i].C];
        }
    }
}

const IndexGroup* TriangleGroup::GetIndexGroup() { return indexGroup; }
int TriangleGroup::GetTriangleCount() { return static_cast<int>(triangles.size()); }
Vector3D* TriangleGroup::GetVertices() { return vertices.empty() ? nullptr : vertices.data(); }
int TriangleGroup::GetVertexCount() { return static_cast<int>(vertices.size()); }
Triangle3D* TriangleGroup::GetTriangles() { return triangles.empty() ? nullptr : triangles.data(); }
