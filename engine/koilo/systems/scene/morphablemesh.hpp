// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file morphablemesh.hpp
 * @brief Mesh with runtime-loaded morph targets (blendshapes) from KoiloMesh files.
 *
 * This class bridges KoiloMeshLoader and BlendshapeController, enabling scripts to load
 * meshes with morph targets from files and animate them by setting morph weights.
 *
 * @author Coela
 * @date 2025
 */

#pragma once

#include <string>
#include <map>
#include <memory>
#include <cstdint>

#include <koilo/systems/scene/mesh.hpp>
#include <koilo/systems/scene/deform/blendshapecontroller.hpp>
#include <koilo/systems/scene/deform/blendshape.hpp>
#include <koilo/assets/koilomesh_loader.hpp>
#include <koilo/assets/model/trianglegroup.hpp>
#include <koilo/assets/model/statictrianglegroup.hpp>
#include <koilo/registry/reflect_macros.hpp>

namespace koilo {

/**
 * @class MorphableMesh
 * @brief Mesh that loads morph targets from .kmesh files at runtime.
 *
 * This class provides a high-level interface for loading meshes with morph targets
 * and controlling them via simple name-based methods. It manages:
 * - KoiloMeshLoader for loading .kmesh files
 * - TriangleGroup/StaticTriangleGroup for mesh geometry
 * - Blendshape objects for each morph target
 * - BlendshapeController for weight management and blending
 *
 * Example usage from script:
 * @code
 * object face : MorphableMesh
 * face.Load("models/nukude_flat.kmesh")
 * face.SetMorphWeight("Anger", 1.0)
 * face.SetMorphWeight("Blink", 0.5)
 * face.Update()
 * @endcode
 */
class MorphableMesh {
private:
    KoiloMeshLoader loader_;
    std::unique_ptr<BlendshapeController> controller_;
    
    // Mesh geometry
    std::unique_ptr<StaticTriangleGroup> baseGeometry_;
    std::unique_ptr<TriangleGroup> workingGeometry_;
    std::unique_ptr<Mesh> mesh_;
    
    // Morph target data (owned by this class)
    std::vector<std::unique_ptr<Blendshape>> blendshapes_;
    std::vector<std::unique_ptr<int[]>> morphIndices_;      // Owned index arrays
    std::vector<std::unique_ptr<Vector3D[]>> morphVectors_; // Owned vertex delta arrays
    
    // Geometry arrays (owned, borrowed by StaticTriangleGroup)
    std::vector<Vector3D> vertexData_;
    std::vector<IndexGroup> indexData_;
    
    // UV data (owned by this class)
    std::vector<Vector2D> uvVertices_;
    std::vector<IndexGroup> uvIndices_;
    
    // Name to blendshape mapping
    std::map<std::string, uint16_t> morphNameToId_;
    
    bool loaded_ = false;

public:
    /**
     * @brief Constructs an empty MorphableMesh.
     */
    MorphableMesh();
    
    /**
     * @brief Destructor - cleans up allocated resources.
     */
    ~MorphableMesh();
    
    /**
     * @brief Loads a .kmesh file and initializes geometry + morph targets.
     *
     * @param filepath Path to the .kmesh file (e.g., "models/nukude_flat.kmesh")
     * @return True if loaded successfully, false otherwise
     */
    bool Load(const char* filepath);
    
    /**
     * @brief Loads from an existing KoiloMeshLoader (e.g., from script engine).
     *
     * @param loader Pointer to already-loaded KoiloMeshLoader
     * @return True if loaded successfully, false otherwise
     */
    bool LoadFromLoader(const KoiloMeshLoader* loader);
    
    /**
     * @brief Checks if a mesh is loaded.
     *
     * @return True if a mesh has been loaded successfully
     */
    bool IsLoaded() const;
    
    /**
     * @brief Sets the weight for a morph target by name.
     *
     * @param name Name of the morph target (e.g., "Anger", "Blink")
     * @param weight Weight value (typically 0.0 to 1.0)
     * @return True if morph found and weight set, false if morph not found
     */
    bool SetMorphWeight(const char* name, float weight);
    
    /**
     * @brief Gets the current weight for a morph target by name.
     *
     * @param name Name of the morph target
     * @return Current weight (0.0 if morph not found)
     */
    float GetMorphWeight(const char* name) const;
    
    /**
     * @brief Gets the number of morph targets loaded.
     *
     * @return Number of morph targets
     */
    int GetMorphCount() const;
    
    /**
     * @brief Gets the number of vertices in the mesh.
     *
     * @return Number of vertices
     */
    int GetVertexCount() const;
    
    /**
     * @brief Updates the mesh geometry by applying all weighted morph targets.
     *
     * This should be called after setting morph weights to recalculate vertices.
     */
    void Update();
    
    /**
     * @brief Resets all morph weights to 0.
     */
    void ResetMorphs();
    
    /**
     * @brief Gets the underlying Mesh object for rendering.
     *
     * @return Pointer to the Mesh (nullptr if not loaded)
     */
    Mesh* GetMesh();
    
    /**
     * @brief Gets the last error message from the loader.
     *
     * @return Error string (empty if no error)
     */
    const char* GetError() const;

private:
    /**
     * @brief Converts KoiloMeshLoader data to internal geometry structures.
     */
    bool BuildGeometry();
    
    /**
     * @brief Creates Blendshape objects from loaded morph targets.
     */
    bool BuildBlendshapes();

    KL_BEGIN_FIELDS(MorphableMesh)
        /* No reflected fields - data managed internally */
    KL_END_FIELDS

    KL_BEGIN_METHODS(MorphableMesh)
        KL_METHOD_AUTO(MorphableMesh, Load, "Load mesh from file"),
        KL_METHOD_AUTO(MorphableMesh, IsLoaded, "Check if loaded"),
        KL_METHOD_AUTO(MorphableMesh, SetMorphWeight, "Set morph weight by name"),
        KL_METHOD_AUTO(MorphableMesh, GetMorphWeight, "Get morph weight by name"),
        KL_METHOD_AUTO(MorphableMesh, GetMorphCount, "Get number of morphs"),
        KL_METHOD_AUTO(MorphableMesh, GetVertexCount, "Get vertex count"),
        KL_METHOD_AUTO(MorphableMesh, Update, "Update geometry"),
        KL_METHOD_AUTO(MorphableMesh, ResetMorphs, "Reset all morphs"),
        KL_METHOD_AUTO(MorphableMesh, GetMesh, "Get Mesh object"),
        KL_METHOD_AUTO(MorphableMesh, GetError, "Get error message")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(MorphableMesh)
        KL_CTOR0(MorphableMesh)
    KL_END_DESCRIBE(MorphableMesh)
};

} // namespace koilo
