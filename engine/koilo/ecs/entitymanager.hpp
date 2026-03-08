/**
 * @file entitymanager.hpp
 * @brief Entity and component manager for ECS.
 *
 * @date 11/10/2025
 * @author Coela
 */

#pragma once

#include <vector>
#include <unordered_map>
#include <memory>
#include <bitset>
#include <queue>
#include <typeindex>
#include "entity.hpp"
#include "component.hpp"
#include "../../registry/reflect_macros.hpp"

namespace ptx {

/**
 * @typedef ComponentMask
 * @brief Bitset representing which components an entity has (max 64 component types).
 */
using ComponentMask = std::bitset<64>;

/**
 * @class IComponentArray
 * @brief Interface for type-erased component storage.
 */
class IComponentArray {
public:
    virtual ~IComponentArray() = default;

    /**
     * @brief Removes component for an entity.
     */
    virtual void Remove(Entity entity) = 0;

    /**
     * @brief Clears all components.
     */
    virtual void Clear() = 0;

    /**
     * @brief Gets the number of components.
     */
    virtual size_t Size() const = 0;
};

/**
 * @class ComponentArray
 * @brief Dense array storage for components of type T.
 */
template<typename T>
class ComponentArray : public IComponentArray {
private:
    std::vector<T> components;                           ///< Dense array of components
    std::unordered_map<Entity, size_t> entityToIndex;    ///< Entity -> index mapping
    std::unordered_map<size_t, Entity> indexToEntity;    ///< Index -> entity mapping

public:
    /**
     * @brief Adds a component to an entity.
     */
    T& Add(Entity entity, const T& component) {
        if (entityToIndex.find(entity) != entityToIndex.end()) {
            // Already exists, replace
            size_t index = entityToIndex[entity];
            components[index] = component;
            return components[index];
        }

        size_t newIndex = components.size();
        components.push_back(component);
        entityToIndex[entity] = newIndex;
        indexToEntity[newIndex] = entity;

        return components[newIndex];
    }

    /**
     * @brief Removes a component from an entity.
     */
    virtual void Remove(Entity entity) override {
        if (entityToIndex.find(entity) == entityToIndex.end()) {
            return;  // Not found
        }

        size_t indexToRemove = entityToIndex[entity];
        size_t lastIndex = components.size() - 1;

        // Swap with last element
        if (indexToRemove != lastIndex) {
            components[indexToRemove] = components[lastIndex];

            // Update mappings
            Entity lastEntity = indexToEntity[lastIndex];
            entityToIndex[lastEntity] = indexToRemove;
            indexToEntity[indexToRemove] = lastEntity;
        }

        // Remove last element
        components.pop_back();
        entityToIndex.erase(entity);
        indexToEntity.erase(lastIndex);
    }

    /**
     * @brief Gets a component for an entity.
     */
    T* Get(Entity entity) {
        auto it = entityToIndex.find(entity);
        if (it == entityToIndex.end()) {
            return nullptr;
        }
        return &components[it->second];
    }

    /**
     * @brief Gets a component for an entity (const).
     */
    const T* Get(Entity entity) const {
        auto it = entityToIndex.find(entity);
        if (it == entityToIndex.end()) {
            return nullptr;
        }
        return &components[it->second];
    }

    /**
     * @brief Checks if entity has this component.
     */
    bool Has(Entity entity) const {
        return entityToIndex.find(entity) != entityToIndex.end();
    }

    /**
     * @brief Clears all components.
     */
    virtual void Clear() override {
        components.clear();
        entityToIndex.clear();
        indexToEntity.clear();
    }

    /**
     * @brief Gets the number of components.
     */
    virtual size_t Size() const override {
        return components.size();
    }

    /**
     * @brief Gets direct access to dense component array (for iteration).
     */
    std::vector<T>& GetComponents() {
        return components;
    }

    /**
     * @brief Gets direct access to dense component array (const).
     */
    const std::vector<T>& GetComponents() const {
        return components;
    }

    /**
     * @brief Gets the entity for a component index.
     */
    Entity GetEntity(size_t index) const {
        auto it = indexToEntity.find(index);
        if (it != indexToEntity.end()) {
            return it->second;
        }
        return Entity();
    }
};

/**
 * @class EntityManager
 * @brief Manages entities and their components in the ECS.
 */
class EntityManager {
private:
    std::vector<uint32_t> generations;                   ///< Generation for each entity index
    std::queue<uint32_t> freeIndices;                    ///< Recycled entity indices
    uint32_t entityCount;                                ///< Total entities created (for new indices)

    std::vector<ComponentMask> componentMasks;           ///< Component masks for each entity

    std::unordered_map<ComponentTypeID, std::shared_ptr<IComponentArray>> componentArrays;  ///< Component storage

public:
    /**
     * @brief Constructor.
     */
    EntityManager();

    /**
     * @brief Destructor.
     */
    ~EntityManager();

    // === Entity Management ===

    /**
     * @brief Creates a new entity.
     */
    Entity CreateEntity();

    /**
     * @brief Destroys an entity and removes all its components.
     */
    void DestroyEntity(Entity entity);

    /**
     * @brief Checks if an entity is valid (generation matches).
     */
    bool IsEntityValid(Entity entity) const;

    /**
     * @brief Gets the number of alive entities.
     */
    size_t GetEntityCount() const;

    // === Component Management ===

    /**
     * @brief Adds a component to an entity.
     */
    template<typename T>
    T& AddComponent(Entity entity, const T& component);

    /**
     * @brief Removes a component from an entity.
     */
    template<typename T>
    void RemoveComponent(Entity entity);

    /**
     * @brief Gets a component from an entity.
     */
    template<typename T>
    T* GetComponent(Entity entity);

    /**
     * @brief Gets a component from an entity (const).
     */
    template<typename T>
    const T* GetComponent(Entity entity) const;

    /**
     * @brief Checks if an entity has a component.
     */
    template<typename T>
    bool HasComponent(Entity entity) const;

    /**
     * @brief Gets the component mask for an entity.
     */
    const ComponentMask& GetComponentMask(Entity entity) const;

    /**
     * @brief Gets all entities with a specific component.
     */
    template<typename T>
    std::vector<Entity> GetEntitiesWithComponent() const;

    /**
     * @brief Gets all entities with multiple components (variadic).
     */
    template<typename... Components>
    std::vector<Entity> GetEntitiesWithComponents() const;

    /**
     * @brief Iterates over all components of type T.
     */
    template<typename T>
    void ForEachComponent(std::function<void(Entity, T&)> callback);

    /**
     * @brief Clears all entities and components.
     */
    void Clear();

private:
    /**
     * @brief Gets or creates a component array for type T.
     */
    template<typename T>
    std::shared_ptr<ComponentArray<T>> GetComponentArray();

    PTX_BEGIN_FIELDS(EntityManager)
    PTX_END_FIELDS

    PTX_BEGIN_METHODS(EntityManager)
        PTX_METHOD_AUTO(EntityManager, CreateEntity, "Create entity"),
        PTX_METHOD_AUTO(EntityManager, DestroyEntity, "Destroy entity"),
        PTX_METHOD_AUTO(EntityManager, IsEntityValid, "Is entity valid"),
        PTX_METHOD_AUTO(EntityManager, GetEntityCount, "Get entity count"),
        PTX_METHOD_AUTO(EntityManager, Clear, "Clear")
    PTX_END_METHODS

    PTX_BEGIN_DESCRIBE(EntityManager)
        PTX_CTOR0(EntityManager)
    PTX_END_DESCRIBE(EntityManager)
};

// === Template Implementations ===

template<typename T>
T& EntityManager::AddComponent(Entity entity, const T& component) {
    if (!IsEntityValid(entity)) {
        throw std::runtime_error("Entity is not valid");
    }

    auto componentArray = GetComponentArray<T>();
    T& addedComponent = componentArray->Add(entity, component);

    // Update component mask
    uint32_t index = entity.GetIndex();
    if (index >= componentMasks.size()) {
        componentMasks.resize(index + 1);
    }
    componentMasks[index].set(GetComponentTypeID<T>());

    return addedComponent;
}

template<typename T>
void EntityManager::RemoveComponent(Entity entity) {
    if (!IsEntityValid(entity)) {
        return;
    }

    auto componentArray = GetComponentArray<T>();
    componentArray->Remove(entity);

    // Update component mask
    uint32_t index = entity.GetIndex();
    if (index < componentMasks.size()) {
        componentMasks[index].reset(GetComponentTypeID<T>());
    }
}

template<typename T>
T* EntityManager::GetComponent(Entity entity) {
    if (!IsEntityValid(entity)) {
        return nullptr;
    }

    auto componentArray = GetComponentArray<T>();
    return componentArray->Get(entity);
}

template<typename T>
const T* EntityManager::GetComponent(Entity entity) const {
    if (!IsEntityValid(entity)) {
        return nullptr;
    }

    ComponentTypeID typeID = GetComponentTypeID<T>();
    auto it = componentArrays.find(typeID);
    if (it == componentArrays.end()) {
        return nullptr;
    }

    auto componentArray = std::static_pointer_cast<ComponentArray<T>>(it->second);
    return componentArray->Get(entity);
}

template<typename T>
bool EntityManager::HasComponent(Entity entity) const {
    if (!IsEntityValid(entity)) {
        return false;
    }

    uint32_t index = entity.GetIndex();
    if (index >= componentMasks.size()) {
        return false;
    }

    return componentMasks[index].test(GetComponentTypeID<T>());
}

template<typename T>
std::vector<Entity> EntityManager::GetEntitiesWithComponent() const {
    std::vector<Entity> entities;

    ComponentTypeID typeID = GetComponentTypeID<T>();
    auto it = componentArrays.find(typeID);
    if (it == componentArrays.end()) {
        return entities;
    }

    auto componentArray = std::static_pointer_cast<ComponentArray<T>>(it->second);
    size_t count = componentArray->Size();

    for (size_t i = 0; i < count; ++i) {
        Entity entity = componentArray->GetEntity(i);
        if (IsEntityValid(entity)) {
            entities.push_back(entity);
        }
    }

    return entities;
}

template<typename... Components>
std::vector<Entity> EntityManager::GetEntitiesWithComponents() const {
    std::vector<Entity> entities;

    // Create component mask for required components
    ComponentMask requiredMask;
    (requiredMask.set(GetComponentTypeID<Components>()), ...);

    // Check all entities
    for (uint32_t i = 0; i < componentMasks.size(); ++i) {
        if (i < generations.size() && generations[i] > 0) {
            // Check if entity has all required components
            if ((componentMasks[i] & requiredMask) == requiredMask) {
                Entity entity(Entity::MakeID(i, generations[i]));
                entities.push_back(entity);
            }
        }
    }

    return entities;
}

template<typename T>
void EntityManager::ForEachComponent(std::function<void(Entity, T&)> callback) {
    auto componentArray = GetComponentArray<T>();
    auto& components = componentArray->GetComponents();

    for (size_t i = 0; i < components.size(); ++i) {
        Entity entity = componentArray->GetEntity(i);
        if (IsEntityValid(entity)) {
            callback(entity, components[i]);
        }
    }
}

template<typename T>
std::shared_ptr<ComponentArray<T>> EntityManager::GetComponentArray() {
    ComponentTypeID typeID = GetComponentTypeID<T>();

    auto it = componentArrays.find(typeID);
    if (it != componentArrays.end()) {
        return std::static_pointer_cast<ComponentArray<T>>(it->second);
    }

    // Create new component array
    auto newArray = std::make_shared<ComponentArray<T>>();
    componentArrays[typeID] = newArray;
    return newArray;
}

} // namespace ptx
