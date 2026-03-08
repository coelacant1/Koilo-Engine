#include <ptx/systems/world/worldmanager.hpp>
#include <ptx/systems/ecs/entitymanager.hpp>
#include <algorithm>
#include <iostream>

namespace ptx {

WorldManager::WorldManager(EntityManager* entityManager)
    : entityManager(entityManager), activeLevel(nullptr),
      streamingEnabled(false), streamingViewerPosition(0, 0, 0),
      streamingCheckInterval(1.0f), timeSinceLastStreamingCheck(0.0f) {
}

WorldManager::~WorldManager() {
    // Unload all levels
    for (auto& level : levels) {
        if (level->GetState() != LevelState::Unloaded) {
            level->Unload();
        }
    }
}

// === Level Management ===

std::shared_ptr<Level> WorldManager::CreateLevel(const std::string& name) {
    // Check if level already exists
    if (levelsByName.find(name) != levelsByName.end()) {
        std::cerr << "Level '" << name << "' already exists" << std::endl;
        return nullptr;
    }

    auto level = std::make_shared<Level>(name);
    level->SetEntityManager(entityManager);
    AddLevel(level);
    return level;
}

void WorldManager::AddLevel(std::shared_ptr<Level> level) {
    if (!level) {
        return;
    }

    level->SetEntityManager(entityManager);
    levels.push_back(level);
    levelsByName[level->GetName()] = level;
}

void WorldManager::RemoveLevel(const std::string& name) {
    // Unload if loaded
    auto level = GetLevel(name);
    if (level && level->GetState() != LevelState::Unloaded) {
        level->Unload();
    }

    // Deactivate if active
    if (activeLevel && activeLevel->GetName() == name) {
        activeLevel = nullptr;
    }

    // Remove from collections
    levelsByName.erase(name);
    auto it = std::remove_if(levels.begin(), levels.end(), [&](const std::shared_ptr<Level>& l) {
        return l->GetName() == name;
    });
    levels.erase(it, levels.end());
}

std::shared_ptr<Level> WorldManager::GetLevel(const std::string& name) const {
    auto it = levelsByName.find(name);
    if (it != levelsByName.end()) {
        return it->second;
    }
    return nullptr;
}

// === Active Level ===

void WorldManager::SetActiveLevel(const std::string& name) {
    auto level = GetLevel(name);
    if (!level) {
        std::cerr << "Cannot set active level: '" << name << "' not found" << std::endl;
        return;
    }

    // Deactivate current level
    if (activeLevel) {
        activeLevel->Deactivate();
    }

    // Activate new level
    activeLevel = level;

    // Load if not loaded
    if (activeLevel->GetState() == LevelState::Unloaded) {
        activeLevel->Load();
    }

    activeLevel->Activate();
}

std::string WorldManager::GetActiveLevelName() const {
    if (activeLevel) {
        return activeLevel->GetName();
    }
    return "";
}

// === Level Loading ===

bool WorldManager::LoadLevel(const std::string& name) {
    auto level = GetLevel(name);
    if (!level) {
        std::cerr << "Cannot load level: '" << name << "' not found" << std::endl;
        return false;
    }

    if (level->GetState() != LevelState::Unloaded) {
        return true;  // Already loaded
    }

    level->Load();

    // Trigger callbacks
    for (auto& callback : onLevelLoadCallbacks) {
        if (callback) {
            callback(level);
        }
    }

    return true;
}

bool WorldManager::UnloadLevel(const std::string& name) {
    auto level = GetLevel(name);
    if (!level) {
        std::cerr << "Cannot unload level: '" << name << "' not found" << std::endl;
        return false;
    }

    // Cannot unload active level
    if (activeLevel && activeLevel->GetName() == name) {
        std::cerr << "Cannot unload active level: '" << name << "'" << std::endl;
        return false;
    }

    if (level->GetState() == LevelState::Unloaded) {
        return true;  // Already unloaded
    }

    level->Unload();

    // Trigger callbacks
    for (auto& callback : onLevelUnloadCallbacks) {
        if (callback) {
            callback(level);
        }
    }

    return true;
}

std::shared_ptr<Level> WorldManager::LoadLevelFromFile(const std::string& filePath) {
    // TODO: Implement file loading (JSON, binary, etc.)
    // For now, create a placeholder level
    auto level = std::make_shared<Level>("LoadedLevel");
    level->SetFilePath(filePath);
    level->SetEntityManager(entityManager);
    AddLevel(level);
    level->Load();
    return level;
}

bool WorldManager::SaveLevelToFile(const std::string& name, const std::string& filePath) {
    auto level = GetLevel(name);
    if (!level) {
        std::cerr << "Cannot save level: '" << name << "' not found" << std::endl;
        return false;
    }

    // TODO: Implement file saving (JSON, binary, etc.)
    level->SetFilePath(filePath);
    return true;
}

void WorldManager::UnloadAllInactiveLevels() {
    for (auto& level : levels) {
        if (level != activeLevel && level->GetState() != LevelState::Unloaded) {
            level->Unload();

            // Trigger callbacks
            for (auto& callback : onLevelUnloadCallbacks) {
                if (callback) {
                    callback(level);
                }
            }
        }
    }
}

// === Streaming ===

void WorldManager::CheckStreaming() {
    if (!streamingEnabled) {
        return;
    }

    for (auto& level : levels) {
        if (!level->IsStreamable()) {
            continue;
        }

        bool inRange = level->IsInStreamingRange(streamingViewerPosition);
        LevelState state = level->GetState();

        // Load if in range and unloaded
        if (inRange && state == LevelState::Unloaded) {
            level->Load();

            // Trigger callbacks
            for (auto& callback : onLevelLoadCallbacks) {
                if (callback) {
                    callback(level);
                }
            }
        }
        // Unload if out of range and loaded (but not active)
        else if (!inRange && (state == LevelState::Loaded || state == LevelState::Active)) {
            if (level != activeLevel) {
                level->Unload();

                // Trigger callbacks
                for (auto& callback : onLevelUnloadCallbacks) {
                    if (callback) {
                        callback(level);
                    }
                }
            }
        }
    }
}

// === Callbacks ===

void WorldManager::AddOnLevelLoadCallback(LevelLoadCallback callback) {
    onLevelLoadCallbacks.push_back(callback);
}

void WorldManager::AddOnLevelUnloadCallback(LevelUnloadCallback callback) {
    onLevelUnloadCallbacks.push_back(callback);
}

// === Update ===

void WorldManager::Update(float deltaTime) {
    // Streaming checks
    if (streamingEnabled) {
        timeSinceLastStreamingCheck += deltaTime;
        if (timeSinceLastStreamingCheck >= streamingCheckInterval) {
            CheckStreaming();
            timeSinceLastStreamingCheck = 0.0f;
        }
    }
}

} // namespace ptx
