// SPDX-License-Identifier: GPL-3.0-or-later
#include <koilo/systems/scene/deform/blendshape.hpp>


namespace koilo {

koilo::Blendshape::Blendshape(int count, int* indexes, Vector3D* vertices) {
    this->count = count;
    this->indexes = indexes;
    this->vertices = vertices;
}

koilo::Blendshape::Blendshape(int count, const int* indexes, const Vector3D* vertices) {
    this->count = count;
    this->indexes = indexes;
    this->vertices = vertices;
}

void koilo::Blendshape::BlendObject3D(ITriangleGroup* obj) {
    for (int i = 0; i < count; i++) {
        obj->GetVertices()[indexes[i]] = obj->GetVertices()[indexes[i]] + vertices[i] * Weight;
    }
}

} // namespace koilo
