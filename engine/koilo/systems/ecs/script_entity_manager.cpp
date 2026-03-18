// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file script_entity_manager.cpp
 * @brief ScriptEntityManager implementation - bridges ECS to KoiloScript.
 */

#include <koilo/systems/ecs/script_entity_manager.hpp>
#include <koilo/systems/ecs/entitymanager.hpp>
#include <koilo/systems/ecs/components/tagcomponent.hpp>
#include <koilo/systems/ecs/components/transformcomponent.hpp>
#include <koilo/systems/ecs/components/velocitycomponent.hpp>
#include <koilo/systems/scene/scenenode.hpp>

namespace koilo {

// --- Lifecycle --------------------------------------------------------------

ScriptEntityManager::ScriptEntityManager()
    : manager_(new EntityManager()) {}

ScriptEntityManager::~ScriptEntityManager() {
    delete manager_;
}

// --- Entity operations ------------------------------------------------------

Entity ScriptEntityManager::Create() {
    Entity e = manager_->CreateEntity();
    // Every entity gets a transform by default
    manager_->AddComponent<TransformComponent>(e, TransformComponent());
    return e;
}

Entity ScriptEntityManager::CreateNamed(const std::string& name) {
    Entity e = Create();
    manager_->AddComponent<TagComponent>(e, TagComponent(name));
    return e;
}

void ScriptEntityManager::Destroy(Entity entity) {
    if (!manager_->IsEntityValid(entity)) return;

    uint32_t idx = entity.GetIndex();
    entityToNode_.erase(idx);
    indexToEntity_.erase(idx);

    manager_->DestroyEntity(entity);
}

bool ScriptEntityManager::IsValid(Entity entity) const {
    return manager_->IsEntityValid(entity);
}

int ScriptEntityManager::GetCount() const {
    return static_cast<int>(manager_->GetEntityCount());
}

// --- Tag component ----------------------------------------------------------

void ScriptEntityManager::SetTag(Entity entity, const std::string& tag) {
    if (!manager_->IsEntityValid(entity)) return;
    if (manager_->HasComponent<TagComponent>(entity)) {
        manager_->GetComponent<TagComponent>(entity)->tag = tag;
    } else {
        manager_->AddComponent<TagComponent>(entity, TagComponent(tag));
    }
}

std::string ScriptEntityManager::GetTag(Entity entity) const {
    if (!manager_->IsEntityValid(entity)) return "";
    auto* tc = manager_->GetComponent<TagComponent>(entity);
    return tc ? tc->tag : "";
}

Entity ScriptEntityManager::FindByTag(const std::string& tag) const {
    auto entities = manager_->GetEntitiesWithComponent<TagComponent>();
    for (auto& e : entities) {
        auto* tc = manager_->GetComponent<TagComponent>(e);
        if (tc && tc->tag == tag) return e;
    }
    return Entity(); // null
}

// --- Transform component ---------------------------------------------------

void ScriptEntityManager::SetPosition(Entity entity, const Vector3D& pos) {
    if (!manager_->IsEntityValid(entity)) return;
    auto* tc = manager_->GetComponent<TransformComponent>(entity);
    if (tc) tc->SetPosition(pos);
}

Vector3D ScriptEntityManager::GetPosition(Entity entity) const {
    if (!manager_->IsEntityValid(entity)) return Vector3D();
    auto* tc = manager_->GetComponent<TransformComponent>(entity);
    return tc ? tc->GetPosition() : Vector3D();
}

void ScriptEntityManager::SetRotation(Entity entity, const Quaternion& rot) {
    if (!manager_->IsEntityValid(entity)) return;
    auto* tc = manager_->GetComponent<TransformComponent>(entity);
    if (tc) tc->SetRotation(rot);
}

Quaternion ScriptEntityManager::GetRotation(Entity entity) const {
    if (!manager_->IsEntityValid(entity)) return Quaternion();
    auto* tc = manager_->GetComponent<TransformComponent>(entity);
    return tc ? tc->GetRotation() : Quaternion();
}

void ScriptEntityManager::SetScale(Entity entity, const Vector3D& scale) {
    if (!manager_->IsEntityValid(entity)) return;
    auto* tc = manager_->GetComponent<TransformComponent>(entity);
    if (tc) tc->SetScale(scale);
}

Vector3D ScriptEntityManager::GetScale(Entity entity) const {
    if (!manager_->IsEntityValid(entity)) return Vector3D(1.0f, 1.0f, 1.0f);
    auto* tc = manager_->GetComponent<TransformComponent>(entity);
    return tc ? tc->GetScale() : Vector3D(1.0f, 1.0f, 1.0f);
}

// --- Velocity component ----------------------------------------------------

void ScriptEntityManager::SetVelocity(Entity entity, const Vector3D& linear) {
    if (!manager_->IsEntityValid(entity)) return;
    if (manager_->HasComponent<VelocityComponent>(entity)) {
        manager_->GetComponent<VelocityComponent>(entity)->linear = linear;
    } else {
        manager_->AddComponent<VelocityComponent>(entity, VelocityComponent(linear));
    }
}

Vector3D ScriptEntityManager::GetVelocity(Entity entity) const {
    if (!manager_->IsEntityValid(entity)) return Vector3D();
    auto* vc = manager_->GetComponent<VelocityComponent>(entity);
    return vc ? vc->linear : Vector3D();
}

void ScriptEntityManager::SetAngularVelocity(Entity entity, const Vector3D& angular) {
    if (!manager_->IsEntityValid(entity)) return;
    if (manager_->HasComponent<VelocityComponent>(entity)) {
        manager_->GetComponent<VelocityComponent>(entity)->angular = angular;
    } else {
        manager_->AddComponent<VelocityComponent>(entity, VelocityComponent(Vector3D(), angular));
    }
}

Vector3D ScriptEntityManager::GetAngularVelocity(Entity entity) const {
    if (!manager_->IsEntityValid(entity)) return Vector3D();
    auto* vc = manager_->GetComponent<VelocityComponent>(entity);
    return vc ? vc->angular : Vector3D();
}

// --- SceneNode bridge ------------------------------------------------------

void ScriptEntityManager::AttachNode(Entity entity, SceneNode* node) {
    if (!manager_->IsEntityValid(entity) || !node) return;
    uint32_t idx = entity.GetIndex();
    entityToNode_[idx] = node;
    indexToEntity_[idx] = entity;
}

SceneNode* ScriptEntityManager::GetNode(Entity entity) const {
    auto it = entityToNode_.find(entity.GetIndex());
    return (it != entityToNode_.end()) ? it->second : nullptr;
}

void ScriptEntityManager::SyncTransforms() {
    for (auto& [idx, node] : entityToNode_) {
        auto eit = indexToEntity_.find(idx);
        if (eit == indexToEntity_.end()) continue;
        Entity entity = eit->second;
        if (!manager_->IsEntityValid(entity)) continue;

        auto* tc = manager_->GetComponent<TransformComponent>(entity);
        if (tc) {
            node->SetPosition(tc->GetPosition());
        }
    }
}

// --- Manager access --------------------------------------------------------

EntityManager& ScriptEntityManager::GetManager() { return *manager_; }
const EntityManager& ScriptEntityManager::GetManager() const { return *manager_; }

// --- Script-friendly API (double handles) ----------------------------------

double ScriptEntityManager::ScriptCreate() {
    return static_cast<double>(Create().GetID());
}

double ScriptEntityManager::ScriptCreateNamed(const std::string& name) {
    return static_cast<double>(CreateNamed(name).GetID());
}

void ScriptEntityManager::ScriptDestroy(double id) {
    Destroy(Entity(static_cast<EntityID>(id)));
}

bool ScriptEntityManager::ScriptIsValid(double id) const {
    return IsValid(Entity(static_cast<EntityID>(id)));
}

void ScriptEntityManager::ScriptSetTag(double id, const std::string& tag) {
    SetTag(Entity(static_cast<EntityID>(id)), tag);
}

std::string ScriptEntityManager::ScriptGetTag(double id) const {
    return GetTag(Entity(static_cast<EntityID>(id)));
}

double ScriptEntityManager::ScriptFindByTag(const std::string& tag) const {
    Entity e = FindByTag(tag);
    return e.IsNull() ? -1.0 : static_cast<double>(e.GetID());
}

void ScriptEntityManager::ScriptSetPosition(double id, const Vector3D& pos) {
    SetPosition(Entity(static_cast<EntityID>(id)), pos);
}

Vector3D ScriptEntityManager::ScriptGetPosition(double id) const {
    return GetPosition(Entity(static_cast<EntityID>(id)));
}

void ScriptEntityManager::ScriptSetRotation(double id, const Quaternion& rot) {
    SetRotation(Entity(static_cast<EntityID>(id)), rot);
}

Quaternion ScriptEntityManager::ScriptGetRotation(double id) const {
    return GetRotation(Entity(static_cast<EntityID>(id)));
}

void ScriptEntityManager::ScriptSetScale(double id, const Vector3D& scale) {
    SetScale(Entity(static_cast<EntityID>(id)), scale);
}

Vector3D ScriptEntityManager::ScriptGetScale(double id) const {
    return GetScale(Entity(static_cast<EntityID>(id)));
}

void ScriptEntityManager::ScriptSetVelocity(double id, const Vector3D& linear) {
    SetVelocity(Entity(static_cast<EntityID>(id)), linear);
}

Vector3D ScriptEntityManager::ScriptGetVelocity(double id) const {
    return GetVelocity(Entity(static_cast<EntityID>(id)));
}

void ScriptEntityManager::ScriptSetAngularVelocity(double id, const Vector3D& angular) {
    SetAngularVelocity(Entity(static_cast<EntityID>(id)), angular);
}

Vector3D ScriptEntityManager::ScriptGetAngularVelocity(double id) const {
    return GetAngularVelocity(Entity(static_cast<EntityID>(id)));
}

std::vector<Entity> ScriptEntityManager::GetAllEntities() const {
    std::vector<Entity> result;
    result.reserve(indexToEntity_.size());
    for (auto& [idx, ent] : indexToEntity_) {
        if (manager_->IsEntityValid(ent))
            result.push_back(ent);
    }
    return result;
}

} // namespace koilo
