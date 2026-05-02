// SPDX-License-Identifier: GPL-3.0-or-later
#include <koilo/systems/ecs/entitymanager.hpp>

namespace koilo {

koilo::EntityManager::EntityManager()
    : entityCount(0) {
}

koilo::EntityManager::~EntityManager() {
    Clear();
}

Entity koilo::EntityManager::CreateEntity() {
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

void koilo::EntityManager::DestroyEntity(Entity entity) {
    if (!IsEntityValid(entity)) {
        return;
    }

    uint32_t index = entity.GetIndex();

    // Remove all components
    for (auto& arr : componentArrays) {
        if (arr) arr->Remove(entity);
    }

    // Clear component mask
    if (index < componentMasks.size()) {
        componentMasks[index].reset();
    }

    // Mark index as free (increment generation to invalidate old handles)
    if (index < generations.size()) {
        generations[index]++;
    }
    freeIndices.push(index);
}

bool koilo::EntityManager::IsEntityValid(Entity entity) const {
    uint32_t index = entity.GetIndex();
    uint32_t generation = entity.GetGeneration();

    if (index >= generations.size()) {
        return false;
    }

    return generations[index] == generation && generation > 0;
}

size_t koilo::EntityManager::GetEntityCount() const {
    return entityCount - freeIndices.size();
}

const ComponentMask& koilo::EntityManager::GetComponentMask(Entity entity) const {
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

void koilo::EntityManager::Clear() {
    // Clear all component arrays
    for (auto& arr : componentArrays) {
        if (arr) arr->Clear();
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

} // namespace koilo
