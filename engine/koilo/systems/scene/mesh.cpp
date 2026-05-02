// SPDX-License-Identifier: GPL-3.0-or-later
#include <koilo/systems/scene/mesh.hpp>
#include <koilo/systems/scene/deform/blendshapecontroller.hpp>
#include <koilo/systems/scene/animation/skeleton.hpp>
#include <koilo/systems/scene/mesh_bvh.hpp>


namespace koilo {

koilo::Mesh::Mesh(IStaticTriangleGroup* originalTriangles, ITriangleGroup* modifiedTriangles, IMaterial* material) : originalTriangles(originalTriangles), modifiedTriangles(modifiedTriangles) {
    this->material = material;
}

koilo::Mesh::~Mesh() {
    delete raycastAccel_;
    raycastAccel_ = nullptr;
}

MeshBVH* koilo::Mesh::GetOrBuildRaycastAccel() {
    if (!modifiedTriangles) return nullptr;

    // Make sure transform has been applied so vertices reflect world space.
    UpdateTransform();

    if (raycastAccel_ && raycastAccelVersion_ == gpuVersion_) {
        return raycastAccel_;
    }

    if (!raycastAccel_) raycastAccel_ = new MeshBVH();

    raycastAccel_->Build(modifiedTriangles->GetVertices(),
                         modifiedTriangles->GetIndexGroup(),
                         modifiedTriangles->GetTriangleCount());
    raycastAccelVersion_ = gpuVersion_;
    return raycastAccel_;
}

void koilo::Mesh::Enable() {
    enabled = true;
}

void koilo::Mesh::Disable() {
    enabled = false;
}

bool koilo::Mesh::IsEnabled() {
    return enabled;
}

bool koilo::Mesh::HasUV(){
    if (!originalTriangles) return false;
    return originalTriangles->HasUV();
}


const Vector2D* koilo::Mesh::GetUVVertices(){
    if (!originalTriangles) return nullptr;
    return originalTriangles->GetUVVertices();
}

const IndexGroup* koilo::Mesh::GetUVIndexGroup(){
    if (!originalTriangles) return nullptr;
    return originalTriangles->GetUVIndexGroup();
}

Vector3D koilo::Mesh::GetCenterOffset() {
    Vector3D center;
    if (!modifiedTriangles || modifiedTriangles->GetVertexCount() == 0)
        return center;

    for (uint32_t i = 0; i < modifiedTriangles->GetVertexCount(); i++) {
        center = center + modifiedTriangles->GetVertices()[i];
    }

    return center.Divide(modifiedTriangles->GetVertexCount());
}

void koilo::Mesh::GetMinMaxDimensions(Vector3D& minimum, Vector3D& maximum) {
    if (!modifiedTriangles) return;
    for (uint32_t i = 0; i < modifiedTriangles->GetVertexCount(); i++) {
        minimum = Vector3D::Min(minimum, modifiedTriangles->GetVertices()[i]);
        maximum = Vector3D::Max(maximum, modifiedTriangles->GetVertices()[i]);
    }
}

Vector3D koilo::Mesh::GetSize() {
    Vector3D min, max;

    GetMinMaxDimensions(min, max);

    return max - min;
}

Transform* koilo::Mesh::GetTransform() {
    return &transform;
}

void koilo::Mesh::SetTransform(Transform& t) {
    transform = t;
    transformDirty_ = true;
}

void koilo::Mesh::ResetVertices() {
    if (!modifiedTriangles || !originalTriangles) return;
    for (uint32_t i = 0; i < modifiedTriangles->GetVertexCount(); i++) {
        modifiedTriangles->GetVertices()[i] = originalTriangles->GetVertices()[i];
    }
    transformDirty_ = true;
}

void koilo::Mesh::UpdateTransform() {
    if (!transformDirty_ || !modifiedTriangles) return;

    for (uint32_t i = 0; i < modifiedTriangles->GetVertexCount(); i++) {
        Vector3D original = modifiedTriangles->GetVertices()[i];
        Vector3D modifiedVector = original;

        modifiedVector = (modifiedVector - transform.GetScaleOffset()) * transform.GetScale() + transform.GetScaleOffset();
        modifiedVector = transform.GetRotation().RotateVector(modifiedVector - transform.GetRotationOffset()) + transform.GetRotationOffset();
        modifiedVector = modifiedVector + transform.GetPosition();

        modifiedTriangles->GetVertices()[i] = modifiedVector;
    }

    transformDirty_ = false;
    ++gpuVersion_;
}

void koilo::Mesh::MarkTransformDirty() {
    transformDirty_ = true;
}

ITriangleGroup* koilo::Mesh::GetTriangleGroup() {
    return modifiedTriangles;
}

IMaterial* koilo::Mesh::GetMaterial() {
    return material;
}

void koilo::Mesh::SetMaterial(IMaterial* material) {
    this->material = material;
}

void koilo::Mesh::SetBlendshapeController(BlendshapeController* controller) {
    blendshapeController = controller;
}

BlendshapeController* koilo::Mesh::GetBlendshapeController() {
    return blendshapeController;
}

void koilo::Mesh::UpdateBlendshapes() {
    if (blendshapeController) {
        blendshapeController->Update(this);
    }
}

void koilo::Mesh::SetSkeleton(Skeleton* skeleton) {
    skeleton_ = skeleton;
}

Skeleton* koilo::Mesh::GetSkeleton() {
    return skeleton_;
}

void koilo::Mesh::SetSkinData(SkinData* skinData) {
    skinData_ = skinData;
}

SkinData* koilo::Mesh::GetSkinData() {
    return skinData_;
}

void koilo::Mesh::UpdateSkinning() {
    if (!skeleton_ || !skinData_) return;
    if (!originalTriangles || !modifiedTriangles) return;

    uint32_t vertexCount = modifiedTriangles->GetVertexCount();
    if (vertexCount == 0) return;

    // Reset to bind pose
    ResetVertices();

    // Apply blendshapes first (if any)
    if (blendshapeController) {
        blendshapeController->Update(this);
    }

    // Compute bone matrices for this frame
    skeleton_->ComputeWorldMatrices();

    // Apply skinning: read from current (possibly blendshape-modified) vertices,
    // write skinned positions back in place
    const Vector3D* srcVerts = originalTriangles->GetVertices();
    Vector3D* dstVerts = modifiedTriangles->GetVertices();
    skeleton_->SkinVertices(*skinData_, srcVerts, dstVerts, vertexCount);
    ++gpuVersion_;
}

} // namespace koilo
