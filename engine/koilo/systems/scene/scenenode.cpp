// SPDX-License-Identifier: GPL-3.0-or-later
#include <koilo/systems/scene/scenenode.hpp>
#include <algorithm>

namespace koilo {

SceneNode::SceneNode() = default;

SceneNode::SceneNode(const std::string& name)
    : name_(name) {}

SceneNode::~SceneNode() {
    if (parent_) {
        parent_->RemoveChild(this);
    }
    for (auto* child : children_) {
        child->parent_ = nullptr;
    }
}

const std::string& SceneNode::GetName() const { return name_; }
void SceneNode::SetName(const std::string& name) { name_ = name; }
Transform& SceneNode::GetLocalTransform() { return localTransform_; }
SceneNode* SceneNode::GetParent() const { return parent_; }
const std::vector<SceneNode*>& SceneNode::GetChildren() const { return children_; }
std::size_t SceneNode::GetChildCount() const { return children_.size(); }
void SceneNode::SetMesh(Mesh* mesh) { mesh_ = mesh; }
Mesh* SceneNode::GetMesh() const { return mesh_; }
void SceneNode::SetScriptPath(const std::string& path) { scriptPath_ = path; }
const std::string& SceneNode::GetScriptPath() const { return scriptPath_; }

void SceneNode::SetPosition(Vector3D pos) {
    localTransform_.SetPosition(pos);
    MarkDirty();
}

void SceneNode::SetRotation(Vector3D eulerXYZ) {
    localTransform_.SetRotation(eulerXYZ);
    MarkDirty();
}

void SceneNode::SetScale(Vector3D scale) {
    localTransform_.SetScale(scale);
    MarkDirty();
}

Vector3D SceneNode::GetPosition() const {
    return localTransform_.GetPosition();
}

void SceneNode::SetParent(SceneNode* parent) {
    if (parent_ == parent) return;
    if (parent_) {
        parent_->RemoveChild(this);
    }
    parent_ = parent;
    if (parent_) {
        parent_->AddChild(this);
    }
    MarkDirty();
}

SceneNode* SceneNode::FindChild(const std::string& name) const {
    for (auto* child : children_) {
        if (child->name_ == name) return child;
    }
    return nullptr;
}

void SceneNode::MarkDirty() {
    if (worldDirty_) return;  // already dirty, children already notified
    worldDirty_ = true;
    for (auto* child : children_) {
        child->MarkDirty();
    }
}

const Transform& SceneNode::GetWorldTransform() {
    if (worldDirty_) {
        RecomputeWorldTransform();
    }
    return worldTransform_;
}

void SceneNode::RecomputeWorldTransform() {
    if (parent_) {
        // Combine parent world + local
        const Transform& parentWorld = parent_->GetWorldTransform();
        Vector3D parentPos = parentWorld.GetPosition();
        Vector3D localPos = localTransform_.GetPosition();
        worldTransform_.SetPosition(Vector3D(
            parentPos.X + localPos.X,
            parentPos.Y + localPos.Y,
            parentPos.Z + localPos.Z
        ));
        // DEFERRED: Full parent-child rotation/scale composition (multiply quaternions, scale product)
        worldTransform_.SetScale(localTransform_.GetScale());
        worldTransform_.SetRotation(localTransform_.GetRotation());
    } else {
        // Root node: world = local
        worldTransform_.SetPosition(localTransform_.GetPosition());
        worldTransform_.SetScale(localTransform_.GetScale());
        worldTransform_.SetRotation(localTransform_.GetRotation());
    }
    worldDirty_ = false;
}

void SceneNode::AddChild(SceneNode* child) {
    children_.push_back(child);
}

void SceneNode::RemoveChild(SceneNode* child) {
    children_.erase(
        std::remove(children_.begin(), children_.end(), child),
        children_.end()
    );
}

} // namespace koilo
