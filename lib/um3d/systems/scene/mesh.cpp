#include "mesh.hpp"

Mesh::Mesh(IStaticTriangleGroup* originalTriangles, ITriangleGroup* modifiedTriangles, IMaterial* material) : originalTriangles(originalTriangles), modifiedTriangles(modifiedTriangles) {
    this->material = material;
}

Mesh::~Mesh() {}

void Mesh::Enable() {
    enabled = true;
}

void Mesh::Disable() {
    enabled = false;
}

bool Mesh::IsEnabled() {
    return enabled;
}

bool Mesh::HasUV(){
    return originalTriangles->HasUV();
}


const Vector2D* Mesh::GetUVVertices(){
    return originalTriangles->GetUVVertices();
}

const IndexGroup* Mesh::GetUVIndexGroup(){
    return originalTriangles->GetUVIndexGroup();
}

Vector3D Mesh::GetCenterOffset() {
    Vector3D center;

    for (int i = 0; i < modifiedTriangles->GetVertexCount(); i++) {
        center = center + modifiedTriangles->GetVertices()[i];
    }

    return center.Divide(modifiedTriangles->GetVertexCount());
}

void Mesh::GetMinMaxDimensions(Vector3D& minimum, Vector3D& maximum) {
    for (int i = 0; i < modifiedTriangles->GetVertexCount(); i++) {
        minimum = Vector3D::Min(minimum, modifiedTriangles->GetVertices()[i]);
        maximum = Vector3D::Max(maximum, modifiedTriangles->GetVertices()[i]);
    }
}

Vector3D Mesh::GetSize() {
    Vector3D min, max;

    GetMinMaxDimensions(min, max);

    return max - min;
}

Transform* Mesh::GetTransform() {
    return &transform;
}

void Mesh::SetTransform(Transform& t) {
    transform = t;
}

void Mesh::ResetVertices() {
    for (int i = 0; i < modifiedTriangles->GetVertexCount(); i++) {
        modifiedTriangles->GetVertices()[i] = originalTriangles->GetVertices()[i];
    }
}

void Mesh::UpdateTransform() {
    for (int i = 0; i < modifiedTriangles->GetVertexCount(); i++) {
        Vector3D modifiedVector = modifiedTriangles->GetVertices()[i];

        modifiedVector = (modifiedVector - transform.GetScaleOffset()) * transform.GetScale() + transform.GetScaleOffset();
        modifiedVector = transform.GetRotation().RotateVector(modifiedVector - transform.GetRotationOffset()) + transform.GetRotationOffset();
        modifiedVector = modifiedVector + transform.GetPosition();

        modifiedTriangles->GetVertices()[i] = modifiedVector;
    }
}

ITriangleGroup* Mesh::GetTriangleGroup() {
    return modifiedTriangles;
}

IMaterial* Mesh::GetMaterial() {
    return material;
}

void Mesh::SetMaterial(IMaterial* material) {
    this->material = material;
}
