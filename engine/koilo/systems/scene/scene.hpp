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

#include "mesh.hpp"
#include "scenenode.hpp"
#include <koilo/registry/reflect_macros.hpp>


namespace koilo {

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
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(Scene)
        KL_CTOR0(Scene)
    KL_END_DESCRIBE(Scene)

};

} // namespace koilo
