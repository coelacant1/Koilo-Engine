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

#include "mesh.hpp"
#include "../../registry/reflect_macros.hpp"

/**
 * @class Scene
 * @brief Manages a collection of 3D meshes and applies optional screen-space effects.
 *
 * The `Scene` class allows for the addition, removal, and management of 3D meshes.
 * It also supports applying screen-space effects to modify the appearance of the entire scene.
 */
class Scene {
private:
    
    const int maxMeshes; ///< Maximum number of meshes allowed in the scene.
    Mesh** meshes; ///< Array of pointers to the `Mesh` instances in the scene.
    unsigned int numMeshes = 0; ///< Current number of meshes in the scene.
    bool doesUseEffect = false; ///< Flag indicating whether the effect is enabled.

    /**
     * @brief Removes an object from the scene by its index.
     *
     * This method adjusts the internal array to maintain order.
     *
     * @param element Index of the object to remove.
     */
    void RemoveElement(unsigned int element);

public:
    /**
     * @brief Constructs a `Scene` instance.
     * 
     * @param maxMeshes Maximum number of meshes the scene can hold.
     */
    Scene(unsigned int maxMeshes);

    /**
     * @brief Destructor for `Scene`, freeing allocated resources.
     */
    ~Scene();

    /**
     * @brief Adds a 3D object to the scene.
     * 
     * @param object Pointer to the `Mesh` to add.
     */
    void AddMesh(Mesh* object);

    /**
     * @brief Removes a 3D object from the scene by its index.
     * 
     * @param i Index of the object to remove.
     */
    void RemoveMesh(unsigned int i);

    /**
     * @brief Removes a specific 3D object from the scene.
     * 
     * @param object Pointer to the `Mesh` to remove.
     */
    void RemoveMesh(Mesh* object);

    /**
     * @brief Retrieves all meshes in the scene.
     * 
     * @return Pointer to the array of `Mesh` pointers.
     */
    Mesh** GetMeshes();

    /**
     * @brief Retrieves the current number of meshes in the scene.
     * 
     * @return Number of meshes in the scene.
     */
    uint8_t GetMeshCount();

    /**
     * @brief Retrieves the current number of triangles in the scene
     * 
     * @return Number of triangles in the scene.
     */
    uint32_t GetTotalTriangleCount() const;

    PTX_BEGIN_FIELDS(Scene)
        /* No reflected fields. */
    PTX_END_FIELDS

    PTX_BEGIN_METHODS(Scene)
        PTX_METHOD_AUTO(Scene, AddMesh, "Add mesh"),
        /* Remove mesh */ PTX_METHOD_OVLD(Scene, RemoveMesh, void, unsigned int),
        /* Remove mesh */ PTX_METHOD_OVLD(Scene, RemoveMesh, void, Mesh *),
        PTX_METHOD_AUTO(Scene, GetMeshes, "Get meshes"),
        PTX_METHOD_AUTO(Scene, GetMeshCount, "Get mesh count"),
        PTX_METHOD_AUTO(Scene, GetTotalTriangleCount, "Get total triangle count")
    PTX_END_METHODS

    PTX_BEGIN_DESCRIBE(Scene)
        PTX_CTOR(Scene, unsigned int)
    PTX_END_DESCRIBE(Scene)

};
