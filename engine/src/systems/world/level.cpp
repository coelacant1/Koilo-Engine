#include <ptx/systems/world/level.hpp>
#include <ptx/systems/ecs/entitymanager.hpp>
#include <algorithm>

namespace ptx {

Level::Level(const std::string& name)
    : name(name), state(LevelState::Unloaded), entityManager(nullptr),
      renderScene(nullptr), isStreamable(false), streamingOrigin(0, 0, 0),
      streamingRadius(1000.0f) {
}

Level::~Level() {
    if (state != LevelState::Unloaded) {
        Unload();
    }
}

// === Entity Management ===

void Level::AddEntity(Entity entity) {
    // Check if entity already exists
    auto it = std::find_if(entities.begin(), entities.end(), [&](const Entity& e) {
        return e.GetID() == entity.GetID();
    });

    if (it == entities.end()) {
        entities.push_back(entity);
    }
}

void Level::RemoveEntity(Entity entity) {
    auto it = std::remove_if(entities.begin(), entities.end(), [&](const Entity& e) {
        return e.GetID() == entity.GetID();
    });

    entities.erase(it, entities.end());
}

void Level::ClearEntities() {
    // Destroy all entities through entity manager if available
    if (entityManager) {
        for (const Entity& entity : entities) {
            entityManager->DestroyEntity(entity);
        }
    }
    entities.clear();
}

// === Metadata ===

void Level::SetMetadata(const std::string& key, const std::string& value) {
    metadata[key] = value;
}

std::string Level::GetMetadata(const std::string& key) const {
    auto it = metadata.find(key);
    if (it != metadata.end()) {
        return it->second;
    }
    return "";
}

bool Level::HasMetadata(const std::string& key) const {
    return metadata.find(key) != metadata.end();
}

// === Streaming ===

void Level::SetStreamingBounds(const Vector3D& origin, float radius) {
    streamingOrigin = origin;
    streamingRadius = radius;
}

bool Level::IsInStreamingRange(const Vector3D& position) const {
    if (!isStreamable) {
        return false;
    }

    float distanceSquared = (position - streamingOrigin).lengthSquared();
    float radiusSquared = streamingRadius * streamingRadius;

    return distanceSquared <= radiusSquared;
}

// === Lifecycle ===

void Level::Load() {
    if (state != LevelState::Unloaded) {
        return;
    }

    state = LevelState::Loading;
    // Loading logic would go here (resource loading, entity creation, etc.)
    state = LevelState::Loaded;
}

void Level::Unload() {
    if (state == LevelState::Unloaded) {
        return;
    }

    state = LevelState::Unloading;
    ClearEntities();
    state = LevelState::Unloaded;
}

void Level::Activate() {
    if (state == LevelState::Loaded) {
        state = LevelState::Active;
    }
}

void Level::Deactivate() {
    if (state == LevelState::Active) {
        state = LevelState::Loaded;
    }
}

} // namespace ptx
