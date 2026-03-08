// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file scenenode.hpp
 * @brief Scene graph node with parent-child transform hierarchy.
 *
 * SceneNode provides the foundation for the scene graph: each node has a
 * local transform, an optional parent, and zero or more children.  World
 * transforms are lazily recomputed when dirty.  Nodes may optionally hold
 * a Mesh pointer for rendering.
 */

#pragma once

#include <koilo/core/math/transform.hpp>
#include <koilo/core/math/vector3d.hpp>
#include <koilo/registry/reflect_macros.hpp>
#include <string>
#include <vector>

namespace koilo {

class Mesh;  // forward

/**
 * @class SceneNode
 * @brief A node in the scene graph with parent-child transform inheritance.
 */
class SceneNode {
public:
    SceneNode();
    explicit SceneNode(const std::string& name);
    ~SceneNode();

    // --- Name ---
    const std::string& GetName() const;
    void SetName(const std::string& name);

    // --- Transform (local) ---
    Transform& GetLocalTransform();
    void SetPosition(Vector3D pos);
    void SetRotation(Vector3D eulerXYZ);
    void SetScale(Vector3D scale);
    Vector3D GetPosition() const;

    // --- Hierarchy ---
    void SetParent(SceneNode* parent);
    SceneNode* GetParent() const;
    const std::vector<SceneNode*>& GetChildren() const;
    std::size_t GetChildCount() const;
    SceneNode* FindChild(const std::string& name) const;

    // --- World transform (cached, recomputed when dirty) ---
    const Transform& GetWorldTransform();
    void MarkDirty();

    // --- Optional mesh attachment ---
    void SetMesh(Mesh* mesh);
    Mesh* GetMesh() const;

    // --- Script context binding ---
    void SetScriptPath(const std::string& path);
    const std::string& GetScriptPath() const;

private:
    std::string name_;
    Transform localTransform_;
    Transform worldTransform_;
    bool worldDirty_ = true;

    SceneNode* parent_ = nullptr;
    std::vector<SceneNode*> children_;

    Mesh* mesh_ = nullptr;
    std::string scriptPath_;

    void AddChild(SceneNode* child);
    void RemoveChild(SceneNode* child);
    void RecomputeWorldTransform();

    // --- Reflection ---
    KL_BEGIN_FIELDS(SceneNode)
    KL_END_FIELDS

    KL_BEGIN_METHODS(SceneNode)
        KL_METHOD_AUTO(SceneNode, SetPosition, "Set local position"),
        KL_METHOD_AUTO(SceneNode, SetRotation, "Set local rotation (euler XYZ)"),
        KL_METHOD_AUTO(SceneNode, SetScale, "Set local scale"),
        KL_METHOD_AUTO(SceneNode, GetPosition, "Get local position"),
        KL_METHOD_AUTO(SceneNode, GetChildCount, "Get number of children"),
        KL_METHOD_AUTO(SceneNode, SetParent, "Set parent node"),
        KL_METHOD_AUTO(SceneNode, GetParent, "Get parent node"),
        KL_METHOD_AUTO(SceneNode, FindChild, "Find child by name"),
        KL_METHOD_AUTO(SceneNode, SetMesh, "Attach mesh to node"),
        KL_METHOD_AUTO(SceneNode, GetMesh, "Get attached mesh"),
        KL_METHOD_AUTO(SceneNode, SetScriptPath, "Set per-object script path"),
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(SceneNode)
        KL_CTOR0(SceneNode)
    KL_END_DESCRIBE(SceneNode)
};

} // namespace koilo
