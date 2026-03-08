#include <ptx/ecs/entitymanager.hpp>

namespace ptx {

EntityManager::EntityManager()
    : entityCount(0) {
}

EntityManager::~EntityManager() {
    Clear();
}

Entity EntityManager::CreateEntity() {
    uint32_t index;
    uint32_t generation;

    if (!freeIndices.empty()) {
        // Reuse free index
        index = freeIndices.front();
        freeIndices.pop();

        // Increment generation
        if (index >= generations.size()) {
            generations.resize(index + 1, 0);
        }
        generations[index]++;
        generation = generations[index];
    } else {
        // Allocate new index
        index = entityCount++;

        if (index >= generations.size()) {
            generations.resize(index + 1, 0);
        }
        generations[index] = 1;
        generation = 1;
    }

    // Ensure component mask is sized
    if (index >= componentMasks.size()) {
        componentMasks.resize(index + 1);
    }
    componentMasks[index].reset();

    return Entity(Entity::MakeID(index, generation));
}

void EntityManager::DestroyEntity(Entity entity) {
    if (!IsEntityValid(entity)) {
        return;
    }

    uint32_t index = entity.GetIndex();

    // Remove all components
    for (auto& pair : componentArrays) {
        pair.second->Remove(entity);
    }

    // Clear component mask
    if (index < componentMasks.size()) {
        componentMasks[index].reset();
    }

    // Mark index as free
    freeIndices.push(index);
}

bool EntityManager::IsEntityValid(Entity entity) const {
    uint32_t index = entity.GetIndex();
    uint32_t generation = entity.GetGeneration();

    if (index >= generations.size()) {
        return false;
    }

    return generations[index] == generation && generation > 0;
}

size_t EntityManager::GetEntityCount() const {
    return entityCount - freeIndices.size();
}

const ComponentMask& EntityManager::GetComponentMask(Entity entity) const {
    static ComponentMask emptyMask;

    if (!IsEntityValid(entity)) {
        return emptyMask;
    }

    uint32_t index = entity.GetIndex();
    if (index >= componentMasks.size()) {
        return emptyMask;
    }

    return componentMasks[index];
}

void EntityManager::Clear() {
    // Clear all component arrays
    for (auto& pair : componentArrays) {
        pair.second->Clear();
    }
    componentArrays.clear();

    // Reset entity tracking
    generations.clear();
    componentMasks.clear();
    while (!freeIndices.empty()) {
        freeIndices.pop();
    }
    entityCount = 0;
}

} // namespace ptx
