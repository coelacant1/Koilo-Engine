// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file Scene.h
 * @brief Defines the `Scene` class for managing meshes and effects in a 3D environment.
 *
 * The `Scene` class serves as a container for 3D meshes and optional screen-space effects.
 * It provides methods to manage meshes and apply visual effects to the entire scene.
 *
 * @date 22/12/2024
 * @version 1.0
 * @author Coela Can't
 */

#pragma once

#include <vector>
#include <map>
#include <string>
#include <memory>
#include <cstdint>

#include "mesh.hpp"
#include "scenenode.hpp"
#include "../../core/geometry/ray.hpp"
#include <koilo/registry/reflect_macros.hpp>


namespace koilo {

struct RaycastHit; // fwd

/**
 * @class Scene
 * @brief Manages a collection of 3D meshes and applies optional screen-space effects.
 *
 * The `Scene` class allows for the addition, removal, and management of 3D meshes.
 * It also supports applying screen-space effects to modify the appearance of the entire scene.
 */
class Scene {
private:
    std::vector<Mesh*> meshes; ///< Collection of mesh pointers managed by the scene.
    unsigned int numMeshes = 0; ///< Current number of meshes in the scene.
    bool doesUseEffect = false; ///< Flag indicating whether the effect is enabled.

    // Scene graph nodes (owned by scene)
    std::map<std::string, SceneNode*> nodesByName_;
    std::vector<std::unique_ptr<SceneNode>> ownedNodes_;
    std::uint64_t hierarchyGen_ = 0; ///< Bumped on structural change.

    void RemoveElement(unsigned int element);

public:
    /**
     * @brief Constructs a `Scene` instance.
     */
    Scene();
    ~Scene();

    // --- Mesh management (flat list for rendering) ---
    void AddMesh(Mesh* object);
    void RemoveMesh(unsigned int i);
    void RemoveMesh(Mesh* object);
    Mesh** GetMeshes();
    unsigned int GetMeshCount();
    Mesh* GetMesh(unsigned int index);
    uint32_t GetTotalTriangleCount() const;

    // --- Scene graph node management ---
    SceneNode* CreateObject(const std::string& name);
    SceneNode* Find(const std::string& name);
    std::size_t GetNodeCount() const;

    /**
     * @brief All nodes whose parent is null (i.e. graph roots).
     *
     * Returned by value to keep the API safe across mutations; the
     * pointers themselves are stable for the lifetime of the node
     * (nodes are owned by the Scene via unique_ptr and only destroyed
     * when the Scene is destroyed).
     */
    std::vector<SceneNode*> GetRootNodes() const;

    /**
     * @brief Monotonic counter bumped whenever the node graph's
     * structure changes (CreateObject + manual BumpHierarchyGeneration
     * calls). The editor uses this to skip rebuilding the live
     * hierarchy panel when nothing has changed.
     *
     * NOTE: SceneNode::SetParent is NOT auto-tracked yet - call
     * BumpHierarchyGeneration() manually after reparenting if the
     * editor must observe it. See E3 follow-up.
     */
    std::uint64_t HierarchyGeneration() const { return hierarchyGen_; }

    /// Manual bump for callers that mutate hierarchy without going
    /// through CreateObject (e.g. after SceneNode::SetParent).
    void BumpHierarchyGeneration() { ++hierarchyGen_; }

    /// Walk all SceneNodes that have an attached mesh and return the
    /// node whose mesh is hit nearest by `ray`.  The ray is in world
    /// space; for each node it is transformed into the mesh's local
    /// space using the node's world transform (translation + rotation
    /// only at present -- non-uniform scale is not yet supported).
    /// Returns nullptr if nothing is hit.
    ///
    /// Used by the editor viewport for click-to-select.  Note: nodes
    /// hidden in hierarchy that still hold a mesh ARE pickable; the
    /// editor is responsible for filtering if needed.
    SceneNode* PickNode(const Ray& worldRay, float maxDistance = 1e30f);

    /**
     * @brief Serialize the scene's node graph to a `.kscene` file.
     *
     * v1 scope (matches `docs/kscene-format.md` rules): only emits
     * `SceneNode` declarations and their local Transform setters, plus
     * a second pass for parent links. Mesh attachments, scripts,
     * physics bodies, colliders, and lights are NOT serialized - Scene
     * does not own those resources, so the editor cannot reconstruct
     * them from this object alone.  This is enough to round-trip
     * editor-authored hierarchy + transforms; richer scenes still rely
     * on an outer `.ks` runner.
     *
     * Returns true on success.  On failure (e.g. file open error) the
     * file is left untouched.
     */
    bool SaveToKScene(const std::string& path) const;

    KL_BEGIN_FIELDS(Scene)
    KL_END_FIELDS

    KL_BEGIN_METHODS(Scene)
        KL_METHOD_AUTO(Scene, AddMesh, "Add mesh"),
        /* Remove mesh */ KL_METHOD_OVLD(Scene, RemoveMesh, void, unsigned int),
        /* Remove mesh */ KL_METHOD_OVLD(Scene, RemoveMesh, void, Mesh *),
        KL_METHOD_AUTO(Scene, GetMeshes, "Get meshes"),
        KL_METHOD_AUTO(Scene, GetMeshCount, "Get mesh count"),
        KL_METHOD_AUTO(Scene, GetMesh, "Get mesh by index"),
        KL_METHOD_AUTO(Scene, GetTotalTriangleCount, "Get total triangle count"),
        KL_METHOD_AUTO(Scene, CreateObject, "Create a named scene node"),
        KL_METHOD_AUTO(Scene, Find, "Find scene node by name"),
        KL_METHOD_AUTO(Scene, GetNodeCount, "Get number of scene nodes"),
        KL_METHOD_AUTO(Scene, HierarchyGeneration, "Get hierarchy generation counter"),
        KL_METHOD_AUTO(Scene, BumpHierarchyGeneration, "Manually bump hierarchy generation"),
        KL_METHOD_AUTO(Scene, PickNode, "Pick the nearest scene node hit by a ray"),
        KL_METHOD_AUTO(Scene, SaveToKScene, "Save scene node graph to a .kscene file"),
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(Scene)
        KL_CTOR0(Scene)
    KL_END_DESCRIBE(Scene)

};

} // namespace koilo
