// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file skeleton.hpp
 * @brief Skeletal animation system - Bone hierarchy and runtime skinning.
 *
 * Provides bone-driven vertex deformation for character animation.
 * Each Bone stores a local transform and inverse bind-pose matrix.
 * The Skeleton computes world-space bone matrices and applies
 * weighted vertex skinning to mesh geometry.
 *
 * @date 23/02/2026
 * @author Coela
 */

#pragma once

#include <koilo/core/math/matrix4x4.hpp>
#include <koilo/core/math/vector3d.hpp>
#include <koilo/core/math/quaternion.hpp>
#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include <algorithm>
#include "../../../registry/reflect_macros.hpp"

namespace koilo {

/**
 * @struct Bone
 * @brief Single bone in a skeletal hierarchy.
 *
 * Bones form a tree via parentIndex. The root bone has parentIndex = -1.
 * localTransform is the bone's current pose in parent-local space.
 * inverseBindMatrix transforms from mesh space to bone space at bind pose.
 */
struct Bone {
    std::string name;
    int parentIndex = -1;
    Matrix4x4 inverseBindMatrix;  ///< Mesh-space -> bone-space at bind pose
    Matrix4x4 localTransform;     ///< Current pose in parent-local space

    // Decomposed local transform for animation targeting
    Vector3D localPosition;
    Quaternion localRotation;
    Vector3D localScale{1.0f, 1.0f, 1.0f};

    // Rebuild localTransform from position/rotation/scale
    void UpdateLocalTransform() {
        localTransform = Matrix4x4::TRS(localPosition, localRotation, localScale);
    }

    KL_BEGIN_FIELDS(Bone)
        KL_FIELD(Bone, name, "Name", 0, 0),
        KL_FIELD(Bone, inverseBindMatrix, "Inverse bind matrix", 0, 0),
        KL_FIELD(Bone, localRotation, "Local rotation", 0, 0)
    KL_END_FIELDS

    KL_BEGIN_METHODS(Bone)
        /* No reflected methods. */
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(Bone)
        /* No reflected ctors. */
    KL_END_DESCRIBE(Bone)

};

/**
 * @struct SkinVertex
 * @brief Per-vertex skinning data - up to 4 bone influences.
 *
 * boneIndices[i] references a bone in the Skeleton.
 * weights[i] is the influence of that bone (weights should sum to 1.0).
 */
struct SkinVertex {
    uint8_t boneIndices[4] = {0, 0, 0, 0};
    float weights[4] = {0.0f, 0.0f, 0.0f, 0.0f};

    // Normalize weights so they sum to 1.0
    void Normalize() {
        float sum = weights[0] + weights[1] + weights[2] + weights[3];
        if (sum > 0.0001f) {
            float inv = 1.0f / sum;
            for (int i = 0; i < 4; ++i) weights[i] *= inv;
        }
    }

    KL_BEGIN_FIELDS(SkinVertex)
        /* No reflected fields. */
    KL_END_FIELDS

    KL_BEGIN_METHODS(SkinVertex)
        KL_METHOD_AUTO(SkinVertex, Normalize, "Normalize")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(SkinVertex)
        /* No reflected ctors. */
    KL_END_DESCRIBE(SkinVertex)

};

/**
 * @struct SkinData
 * @brief Skinning weights for all vertices in a mesh.
 *
 * skinVertices.size() must match the mesh vertex count.
 */
struct SkinData {
    std::vector<SkinVertex> skinVertices;

    std::size_t GetVertexCount() const { return skinVertices.size(); }

    void AddVertex(uint8_t b0, float w0,
                   uint8_t b1 = 0, float w1 = 0.0f,
                   uint8_t b2 = 0, float w2 = 0.0f,
                   uint8_t b3 = 0, float w3 = 0.0f) {
        SkinVertex sv;
        sv.boneIndices[0] = b0; sv.weights[0] = w0;
        sv.boneIndices[1] = b1; sv.weights[1] = w1;
        sv.boneIndices[2] = b2; sv.weights[2] = w2;
        sv.boneIndices[3] = b3; sv.weights[3] = w3;
        sv.Normalize();
        skinVertices.push_back(sv);
    }

    KL_BEGIN_FIELDS(SkinData)
        KL_FIELD(SkinData, skinVertices, "Skin vertices", 0, 0)
    KL_END_FIELDS

    KL_BEGIN_METHODS(SkinData)
        KL_METHOD_AUTO(SkinData, GetVertexCount, "Get vertex count"),
        KL_METHOD_AUTO(SkinData, AddVertex, "Add vertex")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(SkinData)
        /* No reflected ctors. */
    KL_END_DESCRIBE(SkinData)

};

/**
 * @class Skeleton
 * @brief Bone hierarchy with runtime skinning.
 *
 * Usage:
 *   1. AddBone() to build the hierarchy
 *   2. Animate bone local transforms (via AnimationChannel or script)
 *   3. ComputeWorldMatrices() to propagate hierarchy
 *   4. SkinVertices() to deform mesh geometry
 */
class Skeleton {
public:
    Skeleton() = default;

    // Add a bone. Returns its index.
    std::size_t AddBone(const std::string& name, int parentIndex = -1) {
        std::size_t idx = bones_.size();
        Bone bone;
        bone.name = name;
        bone.parentIndex = parentIndex;
        bone.inverseBindMatrix.SetIdentity();
        bone.localTransform.SetIdentity();
        bones_.push_back(bone);
        boneMap_[name] = static_cast<int>(idx);
        return idx;
    }

    // Get bone by index
    Bone* GetBone(std::size_t index) {
        return (index < bones_.size()) ? &bones_[index] : nullptr;
    }

    const Bone* GetBone(std::size_t index) const {
        return (index < bones_.size()) ? &bones_[index] : nullptr;
    }

    // Get bone by name. Returns nullptr if not found.
    Bone* GetBone(const std::string& name) {
        auto it = boneMap_.find(name);
        return (it != boneMap_.end()) ? &bones_[it->second] : nullptr;
    }

    const Bone* GetBone(const std::string& name) const {
        auto it = boneMap_.find(name);
        return (it != boneMap_.end()) ? &bones_[it->second] : nullptr;
    }

    // Get bone index by name. Returns -1 if not found.
    int GetBoneIndex(const std::string& name) const {
        auto it = boneMap_.find(name);
        return (it != boneMap_.end()) ? it->second : -1;
    }

    std::size_t GetBoneCount() const { return bones_.size(); }

    /**
     * @brief Compute world-space bone matrices.
     *
     * Must be called after modifying bone local transforms and before SkinVertices().
     * Traverses the hierarchy root->leaf, multiplying parent × local.
     */
    void ComputeWorldMatrices() {
        worldMatrices_.resize(bones_.size());
        skinMatrices_.resize(bones_.size());

        for (std::size_t i = 0; i < bones_.size(); ++i) {
            bones_[i].UpdateLocalTransform();

            if (bones_[i].parentIndex < 0) {
                worldMatrices_[i] = bones_[i].localTransform;
            } else {
                worldMatrices_[i] = worldMatrices_[bones_[i].parentIndex].Multiply(
                    bones_[i].localTransform);
            }
            // Skin matrix = worldMatrix × inverseBindMatrix
            skinMatrices_[i] = worldMatrices_[i].Multiply(bones_[i].inverseBindMatrix);
        }
    }

    /**
     * @brief Apply skinning to vertex positions.
     *
     * For each vertex: skinnedPos = Σ(weight_i × skinMatrix[bone_i] × originalPos)
     *
     * @param skinData Per-vertex bone indices and weights.
     * @param srcPositions Original bind-pose vertex positions.
     * @param dstPositions Output skinned positions (must be pre-allocated, same size).
     * @param vertexCount Number of vertices.
     */
    void SkinVertices(const SkinData& skinData,
                      const Vector3D* srcPositions,
                      Vector3D* dstPositions,
                      std::size_t vertexCount) const {
        std::size_t count = std::min(vertexCount, skinData.skinVertices.size());

        for (std::size_t v = 0; v < count; ++v) {
            const SkinVertex& sv = skinData.skinVertices[v];
            Vector3D result(0, 0, 0);

            for (int b = 0; b < 4; ++b) {
                if (sv.weights[b] < 0.0001f) continue;
                if (sv.boneIndices[b] >= skinMatrices_.size()) continue;

                Vector3D transformed = skinMatrices_[sv.boneIndices[b]]
                    .TransformVector(srcPositions[v]);
                result.X += transformed.X * sv.weights[b];
                result.Y += transformed.Y * sv.weights[b];
                result.Z += transformed.Z * sv.weights[b];
            }

            dstPositions[v] = result;
        }
    }

    // Get computed skin matrix for a bone (after ComputeWorldMatrices)
    const Matrix4x4* GetSkinMatrix(std::size_t boneIndex) const {
        return (boneIndex < skinMatrices_.size()) ? &skinMatrices_[boneIndex] : nullptr;
    }

    // Get world matrix for a bone (after ComputeWorldMatrices)
    const Matrix4x4* GetWorldMatrix(std::size_t boneIndex) const {
        return (boneIndex < worldMatrices_.size()) ? &worldMatrices_[boneIndex] : nullptr;
    }

    /**
     * @brief Set bind pose from current bone transforms.
     *
     * Call once after positioning all bones in their default (bind) pose.
     * Computes inverseBindMatrix for each bone.
     */
    void SetBindPose() {
        ComputeWorldMatrices();
        for (std::size_t i = 0; i < bones_.size(); ++i) {
            bones_[i].inverseBindMatrix = worldMatrices_[i].Inverse();
        }
    }

    // Reset all bone local transforms to identity
    void ResetPose() {
        for (auto& bone : bones_) {
            bone.localPosition = Vector3D(0, 0, 0);
            bone.localRotation = Quaternion();
            bone.localScale = Vector3D(1, 1, 1);
            bone.localTransform.SetIdentity();
        }
    }

private:
    std::vector<Bone> bones_;
    std::unordered_map<std::string, int> boneMap_;
    std::vector<Matrix4x4> worldMatrices_;   ///< World-space transforms
    std::vector<Matrix4x4> skinMatrices_;    ///< worldMatrix × inverseBindMatrix

    KL_BEGIN_FIELDS(Skeleton)
        /* No reflected fields. */
    KL_END_FIELDS

    KL_BEGIN_METHODS(Skeleton)
        KL_METHOD_AUTO(Skeleton, AddBone, "Add bone"),
        KL_METHOD_AUTO(Skeleton, GetBoneIndex, "Get bone index"),
        KL_METHOD_AUTO(Skeleton, GetBoneCount, "Get bone count"),
        KL_METHOD_AUTO(Skeleton, ComputeWorldMatrices, "Compute world matrices"),
        KL_METHOD_AUTO(Skeleton, SkinVertices, "Skin vertices"),
        KL_METHOD_AUTO(Skeleton, GetSkinMatrix, "Get skin matrix"),
        KL_METHOD_AUTO(Skeleton, GetWorldMatrix, "Get world matrix"),
        KL_METHOD_AUTO(Skeleton, SetBindPose, "Set bind pose"),
        KL_METHOD_AUTO(Skeleton, ResetPose, "Reset pose")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(Skeleton)
        KL_CTOR0(Skeleton)
    KL_END_DESCRIBE(Skeleton)

};

} // namespace koilo
