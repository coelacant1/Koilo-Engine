// SPDX-License-Identifier: GPL-3.0-or-later
#include <koilo/systems/world/level.hpp>
#include <koilo/ecs/entitymanager.hpp>
#include <algorithm>

namespace koilo {

koilo::Level::Level(const std::string& name)
    : name(name), state(LevelState::Unloaded), entityManager(nullptr),
      renderScene(nullptr), isStreamable(false), streamingOrigin(0, 0, 0),
      streamingRadius(1000.0f) {
}

koilo::Level::~Level() {
    if (state != LevelState::Unloaded) {
        Unload();
    }
}

// === Entity Management ===

void koilo::Level::AddEntity(Entity entity) {
    // Check if entity already exists
    auto it = std::find_if(entities.begin(), entities.end(), [&](const Entity& e) {
        return e.GetID() == entity.GetID();
    });

    if (it == entities.end()) {
        entities.push_back(entity);
    }
}

void koilo::Level::RemoveEntity(Entity entity) {
    auto it = std::remove_if(entities.begin(), entities.end(), [&](const Entity& e) {
        return e.GetID() == entity.GetID();
    });

    entities.erase(it, entities.end());
}

void koilo::Level::ClearEntities() {
    // Destroy all entities through entity manager if available
    if (entityManager) {
        for (const Entity& entity : entities) {
            entityManager->DestroyEntity(entity);
        }
    }
    entities.clear();
}

// === Metadata ===

void koilo::Level::SetMetadata(const std::string& key, const std::string& value) {
    metadata[key] = value;
}

std::string koilo::Level::GetMetadata(const std::string& key) const {
    auto it = metadata.find(key);
    if (it != metadata.end()) {
        return it->second;
    }
    return "";
}

bool koilo::Level::HasMetadata(const std::string& key) const {
    return metadata.find(key) != metadata.end();
}

// === Streaming ===

void koilo::Level::SetStreamingBounds(const Vector3D& origin, float radius) {
    streamingOrigin = origin;
    streamingRadius = radius;
}

bool koilo::Level::IsInStreamingRange(const Vector3D& position) const {
    if (!isStreamable) {
        return false;
    }

    Vector3D delta = position - streamingOrigin;
    float distanceSquared = delta.DotProduct(delta);
    float radiusSquared = streamingRadius * streamingRadius;

    return distanceSquared <= radiusSquared;
}

// === Lifecycle ===

void koilo::Level::Load() {
    if (state != LevelState::Unloaded) {
        return;
    }

    state = LevelState::Loading;
    // Loading logic would go here (resource loading, entity creation, etc.)
    state = LevelState::Loaded;
}

void koilo::Level::Unload() {
    if (state == LevelState::Unloaded) {
        return;
    }

    state = LevelState::Unloading;
    ClearEntities();
    state = LevelState::Unloaded;
}

void koilo::Level::Activate() {
    if (state == LevelState::Loaded) {
        state = LevelState::Active;
    }
}

void koilo::Level::Deactivate() {
    if (state == LevelState::Active) {
        state = LevelState::Loaded;
    }
}

} // namespace koilo
