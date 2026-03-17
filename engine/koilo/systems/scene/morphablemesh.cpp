// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file morphablemesh.cpp
 * @brief Implementation of MorphableMesh class.
 */

#include <koilo/systems/scene/morphablemesh.hpp>

namespace koilo {

MorphableMesh::MorphableMesh()
    : controller_(std::make_unique<BlendshapeController>(nullptr, 64)) {
}

MorphableMesh::~MorphableMesh() = default;

bool MorphableMesh::Load(const char* filepath) {
    // Load mesh from file
    if (!loader_.Load(filepath)) {
        return false;
    }
    
    // Build geometry structures
    if (!BuildGeometry()) {
        return false;
    }
    
    // Build blendshapes from morph targets
    if (!BuildBlendshapes()) {
        return false;
    }
    
    loaded_ = true;
    return true;
}

bool MorphableMesh::LoadFromLoader(const KoiloMeshLoader* loader) {
    if (!loader || loader->GetVertexCount() == 0) {
        return false;
    }
    
    // Copy loader data to internal loader
    // This is a bit inefficient but keeps the architecture clean
    // Alternative: Make loader_ a pointer and manage ownership
    loader_ = *loader;
    
    // Build geometry structures
    if (!BuildGeometry()) {
        return false;
    }
    
    // Build blendshapes from morph targets
    if (!BuildBlendshapes()) {
        return false;
    }
    
    loaded_ = true;
    return true;
}

bool MorphableMesh::IsLoaded() const {
    return loaded_;
}

bool MorphableMesh::SetMorphWeight(const char* name, float weight) {
    auto it = morphNameToId_.find(name);
    if (it == morphNameToId_.end()) {
        return false; // Morph not found
    }
    
    controller_->SetWeight(it->second, weight);
    return true;
}

float MorphableMesh::GetMorphWeight(const char* name) const {
    auto it = morphNameToId_.find(name);
    if (it == morphNameToId_.end()) {
        return 0.0f;
    }
    
    return controller_->GetWeight(it->second);
}

int MorphableMesh::GetMorphCount() const {
    return static_cast<int>(loader_.GetMorphCount());
}

int MorphableMesh::GetVertexCount() const {
    return static_cast<int>(loader_.GetVertexCount());
}

void MorphableMesh::Update() {
    if (!loaded_ || !mesh_) {
        return;
    }
    
    controller_->Update(mesh_.get());
}

void MorphableMesh::ResetMorphs() {
    if (!loaded_) {
        return;
    }
    
    controller_->ResetWeights();
}

Mesh* MorphableMesh::GetMesh() {
    return mesh_.get();
}

const char* MorphableMesh::GetError() const {
    return loader_.GetError();
}

bool MorphableMesh::BuildGeometry() {
    size_t vertexCount = loader_.GetVertexCount();
    size_t triangleCount = loader_.GetTriangleCount();
    
    if (vertexCount == 0 || triangleCount == 0) {
        return false;
    }
    
    const float* vertices = loader_.GetVertices();
    const uint32_t* triangles = loader_.GetTriangles();
    
    if (!vertices || !triangles) {
        return false;
    }
    
    // Allocate and copy vertices
    vertexData_.resize(vertexCount);
    for (size_t i = 0; i < vertexCount; ++i) {
        vertexData_[i] = Vector3D(
            vertices[i * 3 + 0],
            vertices[i * 3 + 1],
            vertices[i * 3 + 2]
        );
    }
    
    // Allocate and copy triangles
    indexData_.resize(triangleCount);
    for (size_t i = 0; i < triangleCount; ++i) {
        indexData_[i] = IndexGroup(
            static_cast<int>(triangles[i * 3 + 0]),
            static_cast<int>(triangles[i * 3 + 1]),
            static_cast<int>(triangles[i * 3 + 2])
        );
    }
    
    // Build UV data if available
    if (loader_.HasUVs()) {
        const float* uvData = loader_.GetUVs();
        const uint32_t* uvTris = loader_.GetUVTriangles();
        uint32_t uvCount = loader_.GetUVCount();
        
        if (uvData && uvTris && uvCount > 0) {
            uvVertices_.resize(uvCount);
            for (uint32_t i = 0; i < uvCount; ++i) {
                uvVertices_[i] = Vector2D(uvData[i * 2], uvData[i * 2 + 1]);
            }
            uvIndices_.resize(triangleCount);
            for (size_t i = 0; i < triangleCount; ++i) {
                uvIndices_[i] = IndexGroup(
                    static_cast<int>(uvTris[i * 3 + 0]),
                    static_cast<int>(uvTris[i * 3 + 1]),
                    static_cast<int>(uvTris[i * 3 + 2])
                );
            }
            baseGeometry_ = std::make_unique<StaticTriangleGroup>(
                vertexData_.data(), indexData_.data(), uvIndices_.data(),
                uvVertices_.data(), vertexCount, triangleCount);
        } else {
            baseGeometry_ = std::make_unique<StaticTriangleGroup>(
                vertexData_.data(), indexData_.data(), vertexCount, triangleCount);
        }
    } else {
        baseGeometry_ = std::make_unique<StaticTriangleGroup>(
            vertexData_.data(), indexData_.data(), vertexCount, triangleCount);
    }
    
    workingGeometry_ = std::make_unique<TriangleGroup>(baseGeometry_.get());
    mesh_ = std::make_unique<Mesh>(baseGeometry_.get(), workingGeometry_.get(), nullptr);
    
    return true;
}

bool MorphableMesh::BuildBlendshapes() {
    size_t morphCount = loader_.GetMorphCount();
    
    if (morphCount == 0) {
        return true; // No morphs is valid (static mesh)
    }
    
    // Create Blendshape for each morph target
    for (size_t i = 0; i < morphCount; ++i) {
        const MorphTarget* morphPtr = loader_.GetMorph(static_cast<uint32_t>(i));
        if (!morphPtr) {
            continue; // Skip invalid morph
        }
        
        const MorphTarget& morph = *morphPtr;
        size_t count = morph.indices.size();
        
        if (count == 0) {
            continue; // Skip empty morph
        }
        
        // Allocate owned arrays for indices and deltas
        auto indices = std::make_unique<int[]>(count);
        auto deltas = std::make_unique<Vector3D[]>(count);
        
        // Copy indices
        for (size_t j = 0; j < count; ++j) {
            indices[j] = static_cast<int>(morph.indices[j]);
        }
        
        // Copy delta vectors (deltaX/Y/Z -> Vector3D)
        for (size_t j = 0; j < count; ++j) {
            deltas[j] = Vector3D(
                morph.deltaX[j],
                morph.deltaY[j],
                morph.deltaZ[j]
            );
        }
        
        // Create Blendshape with const pointers (we own the data)
        const int* indexPtr = indices.get();
        const Vector3D* deltaPtr = deltas.get();
        
        auto blendshape = std::make_unique<Blendshape>(
            static_cast<int>(count),
            indexPtr,
            deltaPtr
        );
        
        // Add to controller with dictionary ID = index
        uint16_t dictId = static_cast<uint16_t>(i);
        controller_->AddBlendshape(dictId, blendshape.get());
        
        // Store name mapping
        morphNameToId_[morph.name] = dictId;
        
        // Store ownership
        blendshapes_.push_back(std::move(blendshape));
        morphIndices_.push_back(std::move(indices));
        morphVectors_.push_back(std::move(deltas));
    }
    
    return true;
}

} // namespace koilo
