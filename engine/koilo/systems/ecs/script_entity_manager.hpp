// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file script_entity_manager.hpp
 * @brief Script-facing ECS wrapper - non-template, reflectable.
 *
 * Wraps EntityManager with concrete methods that can be registered as a
 * script global. All template-based component access is handled internally;
 * scripts interact through named component accessors.
 */

#pragma once

#include <koilo/systems/ecs/entity.hpp>
#include <koilo/core/math/vector3d.hpp>
#include <koilo/core/math/quaternion.hpp>
#include <koilo/registry/reflect_macros.hpp>
#include <string>

namespace koilo {

// Forward declarations (avoid pulling in template-heavy headers)
class EntityManager;
class SceneNode;

/**
 * @class ScriptEntityManager
 * @brief Non-template ECS facade for KoiloScript.
 *
 * Owns an EntityManager internally and exposes concrete methods for
 * entity lifecycle, built-in component access, and SceneNode bridging.
 * No polymorphic or template keywords - passes the reflection filter.
 */
class ScriptEntityManager {
public:
    ScriptEntityManager();
    ~ScriptEntityManager();

    // -- Entity lifecycle --------------------------------------------
    Entity Create();
    Entity CreateNamed(const std::string& name);
    void Destroy(Entity entity);
    bool IsValid(Entity entity) const;
    int GetCount() const;

    // -- Script-friendly API (double handles preserve full 64-bit Entity ID) -
    double ScriptCreate();
    double ScriptCreateNamed(const std::string& name);
    void ScriptDestroy(double id);
    bool ScriptIsValid(double id) const;
    void ScriptSetTag(double id, const std::string& tag);
    std::string ScriptGetTag(double id) const;
    double ScriptFindByTag(const std::string& tag) const;
    void ScriptSetPosition(double id, const Vector3D& pos);
    Vector3D ScriptGetPosition(double id) const;
    void ScriptSetRotation(double id, const Quaternion& rot);
    Quaternion ScriptGetRotation(double id) const;
    void ScriptSetScale(double id, const Vector3D& scale);
    Vector3D ScriptGetScale(double id) const;
    void ScriptSetVelocity(double id, const Vector3D& linear);
    Vector3D ScriptGetVelocity(double id) const;
    void ScriptSetAngularVelocity(double id, const Vector3D& angular);
    Vector3D ScriptGetAngularVelocity(double id) const;

    // -- Tag component -----------------------------------------------
    void SetTag(Entity entity, const std::string& tag);
    std::string GetTag(Entity entity) const;
    Entity FindByTag(const std::string& tag) const;

    // -- Transform component -----------------------------------------
    void SetPosition(Entity entity, const Vector3D& pos);
    Vector3D GetPosition(Entity entity) const;
    void SetRotation(Entity entity, const Quaternion& rot);
    Quaternion GetRotation(Entity entity) const;
    void SetScale(Entity entity, const Vector3D& scale);
    Vector3D GetScale(Entity entity) const;

    // -- Velocity component ------------------------------------------
    void SetVelocity(Entity entity, const Vector3D& linear);
    Vector3D GetVelocity(Entity entity) const;
    void SetAngularVelocity(Entity entity, const Vector3D& angular);
    Vector3D GetAngularVelocity(Entity entity) const;

    // -- SceneNode bridge --------------------------------------------
    void AttachNode(Entity entity, SceneNode* node);
    SceneNode* GetNode(Entity entity) const;

    // -- Sync transforms (call once per frame) -----------------------
    void SyncTransforms();

    // -- Iteration (C++ only) ----------------------------------------
    /// Get all tracked entities for inspection/iteration.
    std::vector<Entity> GetAllEntities() const;

    // -- Access underlying manager (C++ only) ------------------------
    EntityManager& GetManager();
    const EntityManager& GetManager() const;

    // -- Reflection --------------------------------------------------
    KL_BEGIN_FIELDS(ScriptEntityManager)
    KL_END_FIELDS

    KL_BEGIN_METHODS(ScriptEntityManager)
        koilo::make::MakeMethod<ScriptEntityManager, &ScriptEntityManager::ScriptCreate>("Create", "Create entity"),
        koilo::make::MakeMethod<ScriptEntityManager, &ScriptEntityManager::ScriptCreateNamed>("CreateNamed", "Create named entity"),
        koilo::make::MakeMethod<ScriptEntityManager, &ScriptEntityManager::ScriptDestroy>("Destroy", "Destroy entity"),
        koilo::make::MakeMethod<ScriptEntityManager, &ScriptEntityManager::ScriptIsValid>("IsValid", "Check entity validity"),
        KL_METHOD_AUTO(ScriptEntityManager, GetCount, "Get entity count"),
        koilo::make::MakeMethod<ScriptEntityManager, &ScriptEntityManager::ScriptSetTag>("SetTag", "Set entity tag"),
        koilo::make::MakeMethod<ScriptEntityManager, &ScriptEntityManager::ScriptGetTag>("GetTag", "Get entity tag"),
        koilo::make::MakeMethod<ScriptEntityManager, &ScriptEntityManager::ScriptFindByTag>("FindByTag", "Find entity by tag"),
        koilo::make::MakeMethod<ScriptEntityManager, &ScriptEntityManager::ScriptSetPosition>("SetPosition", "Set entity position"),
        koilo::make::MakeMethod<ScriptEntityManager, &ScriptEntityManager::ScriptGetPosition>("GetPosition", "Get entity position"),
        koilo::make::MakeMethod<ScriptEntityManager, &ScriptEntityManager::ScriptSetRotation>("SetRotation", "Set entity rotation"),
        koilo::make::MakeMethod<ScriptEntityManager, &ScriptEntityManager::ScriptGetRotation>("GetRotation", "Get entity rotation"),
        koilo::make::MakeMethod<ScriptEntityManager, &ScriptEntityManager::ScriptSetScale>("SetScale", "Set entity scale"),
        koilo::make::MakeMethod<ScriptEntityManager, &ScriptEntityManager::ScriptGetScale>("GetScale", "Get entity scale"),
        koilo::make::MakeMethod<ScriptEntityManager, &ScriptEntityManager::ScriptSetVelocity>("SetVelocity", "Set linear velocity"),
        koilo::make::MakeMethod<ScriptEntityManager, &ScriptEntityManager::ScriptGetVelocity>("GetVelocity", "Get linear velocity"),
        koilo::make::MakeMethod<ScriptEntityManager, &ScriptEntityManager::ScriptSetAngularVelocity>("SetAngularVelocity", "Set angular velocity"),
        koilo::make::MakeMethod<ScriptEntityManager, &ScriptEntityManager::ScriptGetAngularVelocity>("GetAngularVelocity", "Get angular velocity"),
        KL_METHOD_AUTO(ScriptEntityManager, SyncTransforms, "Sync entity transforms to scene nodes")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(ScriptEntityManager)
        KL_CTOR0(ScriptEntityManager)
    KL_END_DESCRIBE(ScriptEntityManager)

private:
    EntityManager* manager_;
    std::unordered_map<uint32_t, SceneNode*> entityToNode_;
    std::unordered_map<uint32_t, Entity> indexToEntity_;  // track full Entity per index
};

} // namespace koilo
