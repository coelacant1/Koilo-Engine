// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file script_world_manager.hpp
 * @brief Script-facing wrapper for WorldManager. Reflectable, registered as "world" global.
 *
 * @date 02/22/2026
 * @author Coela
 */

#pragma once

#include <koilo/systems/world/worldmanager.hpp>
#include <koilo/registry/reflect_macros.hpp>
#include <string>

namespace koilo {

/**
 * @class ScriptWorldManager
 * @brief Reflectable wrapper for KoiloScript access to WorldManager.
 */
class ScriptWorldManager {
private:
    WorldManager manager_;

public:
    ScriptWorldManager() = default;
    ~ScriptWorldManager() = default;

    // === Level management ===
    bool CreateLevel(const std::string& name) { return manager_.CreateLevel(name) != nullptr; }
    void RemoveLevel(const std::string& name) { manager_.RemoveLevel(name); }
    int GetLevelCount() const { return static_cast<int>(manager_.GetLevelCount()); }

    // === Active level ===
    void SetActiveLevel(const std::string& name) { manager_.SetActiveLevel(name); }
    std::string GetActiveLevelName() const { return manager_.GetActiveLevelName(); }

    // === Load / Unload ===
    bool LoadLevel(const std::string& name) { return manager_.LoadLevel(name); }
    bool UnloadLevel(const std::string& name) { return manager_.UnloadLevel(name); }
    void UnloadAllInactive() { manager_.UnloadAllInactiveLevels(); }

    // === File I/O ===
    bool SaveLevel(const std::string& name, const std::string& filePath) {
        return manager_.SaveLevelToFile(name, filePath);
    }
    bool LoadLevelFromFile(const std::string& filePath) {
        return manager_.LoadLevelFromFile(filePath) != nullptr;
    }

    // === Streaming ===
    void SetStreamingEnabled(bool enabled) { manager_.SetStreamingEnabled(enabled); }
    bool IsStreamingEnabled() const { return manager_.IsStreamingEnabled(); }
    void SetViewerPosition(float x, float y, float z) {
        manager_.SetStreamingViewerPosition(Vector3D(x, y, z));
    }
    void SetStreamingInterval(float seconds) { manager_.SetStreamingCheckInterval(seconds); }
    void CheckStreaming() { manager_.CheckStreaming(); }

    // === Update ===
    void Update() { manager_.Update(); }

    // === Direct access ===
    WorldManager& GetManager() { return manager_; }

    KL_BEGIN_FIELDS(ScriptWorldManager)
    KL_END_FIELDS

    KL_BEGIN_METHODS(ScriptWorldManager)
        KL_METHOD_AUTO(ScriptWorldManager, CreateLevel, "Create a level"),
        KL_METHOD_AUTO(ScriptWorldManager, RemoveLevel, "Remove a level"),
        KL_METHOD_AUTO(ScriptWorldManager, GetLevelCount, "Get level count"),
        KL_METHOD_AUTO(ScriptWorldManager, SetActiveLevel, "Set active level"),
        KL_METHOD_AUTO(ScriptWorldManager, GetActiveLevelName, "Get active level name"),
        KL_METHOD_AUTO(ScriptWorldManager, LoadLevel, "Load level by name"),
        KL_METHOD_AUTO(ScriptWorldManager, UnloadLevel, "Unload level by name"),
        KL_METHOD_AUTO(ScriptWorldManager, UnloadAllInactive, "Unload all inactive levels"),
        KL_METHOD_AUTO(ScriptWorldManager, SaveLevel, "Save level to file"),
        KL_METHOD_AUTO(ScriptWorldManager, LoadLevelFromFile, "Load level from file"),
        KL_METHOD_AUTO(ScriptWorldManager, SetStreamingEnabled, "Enable/disable streaming"),
        KL_METHOD_AUTO(ScriptWorldManager, IsStreamingEnabled, "Is streaming enabled"),
        KL_METHOD_AUTO(ScriptWorldManager, SetViewerPosition, "Set streaming viewer position"),
        KL_METHOD_AUTO(ScriptWorldManager, SetStreamingInterval, "Set streaming check interval"),
        KL_METHOD_AUTO(ScriptWorldManager, CheckStreaming, "Trigger streaming check"),
        KL_METHOD_AUTO(ScriptWorldManager, Update, "Update world manager")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(ScriptWorldManager)
        KL_CTOR0(ScriptWorldManager)
    KL_END_DESCRIBE(ScriptWorldManager)
};

} // namespace koilo
