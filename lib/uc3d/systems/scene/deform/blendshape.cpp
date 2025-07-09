#include "blendshape.hpp"

Blendshape::Blendshape(int count, int* indexes, Vector3D* vertices) {
    this->count = count;
    this->indexes = indexes;
    this->vertices = vertices;
}

Blendshape::Blendshape(int count, const int* indexes, const Vector3D* vertices) {
    this->count = count;
    this->indexes = indexes;
    this->vertices = vertices;
}

void Blendshape::BlendObject3D(ITriangleGroup* obj) {
    for (int i = 0; i < count; i++) {
        obj->GetVertices()[indexes[i]] = obj->GetVertices()[indexes[i]] + vertices[i] * Weight; // Add value of morph vertex to original vertex
    }
}
