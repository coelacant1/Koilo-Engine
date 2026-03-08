/**
 * @file worldmanager.hpp
 * @brief Manages multiple levels, streaming, and world state.
 *
 * @date 11/10/2025
 * @author Coela
 */

#pragma once

#include <memory>
#include <vector>
#include <unordered_map>
#include <string>
#include <functional>
#include "level.hpp"
#include "../../core/math/vector3d.hpp"
#include "../../registry/reflect_macros.hpp"

namespace ptx {

// Forward declarations
class EntityManager;

/**
 * @typedef LevelLoadCallback
 * @brief Callback when a level finishes loading.
 */
using LevelLoadCallback = std::function<void(std::shared_ptr<Level>)>;

/**
 * @typedef LevelUnloadCallback
 * @brief Callback when a level finishes unloading.
 */
using LevelUnloadCallback = std::function<void(std::shared_ptr<Level>)>;

/**
 * @class WorldManager
 * @brief Manages game world with multiple levels and streaming.
 *
 * The WorldManager handles:
 * - Loading/unloading levels
 * - Level streaming based on position
 * - Active level management
 * - World state persistence
 */
class WorldManager {
private:
    EntityManager* entityManager;        ///< Entity manager (not owned)

    std::vector<std::shared_ptr<Level>> levels;              ///< All levels
    std::shared_ptr<Level> activeLevel;                      ///< Currently active level
    std::unordered_map<std::string, std::shared_ptr<Level>> levelsByName;

    // Streaming
    bool streamingEnabled;               ///< Is streaming active?
    Vector3D streamingViewerPosition;    ///< Position used for streaming decisions
    float streamingCheckInterval;        ///< Seconds between streaming checks
    float timeSinceLastStreamingCheck;

    // Callbacks
    std::vector<LevelLoadCallback> onLevelLoadCallbacks;
    std::vector<LevelUnloadCallback> onLevelUnloadCallbacks;

public:
    /**
     * @brief Constructor.
     */
    WorldManager(EntityManager* entityManager = nullptr);

    /**
     * @brief Destructor.
     */
    ~WorldManager();

    // === Entity Manager ===

    /**
     * @brief Sets the entity manager.
     */
    void SetEntityManager(EntityManager* manager) { entityManager = manager; }

    /**
     * @brief Gets the entity manager.
     */
    EntityManager* GetEntityManager() const { return entityManager; }

    // === Level Management ===

    /**
     * @brief Creates a new level.
     */
    std::shared_ptr<Level> CreateLevel(const std::string& name);

    /**
     * @brief Adds an existing level.
     */
    void AddLevel(std::shared_ptr<Level> level);

    /**
     * @brief Removes a level by name.
     */
    void RemoveLevel(const std::string& name);

    /**
     * @brief Gets a level by name.
     */
    std::shared_ptr<Level> GetLevel(const std::string& name) const;

    /**
     * @brief Gets all levels.
     */
    const std::vector<std::shared_ptr<Level>>& GetAllLevels() const { return levels; }

    /**
     * @brief Gets the number of levels.
     */
    size_t GetLevelCount() const { return levels.size(); }

    // === Active Level ===

    /**
     * @brief Sets the active level.
     */
    void SetActiveLevel(const std::string& name);

    /**
     * @brief Gets the active level.
     */
    std::shared_ptr<Level> GetActiveLevel() const { return activeLevel; }

    /**
     * @brief Gets the active level name.
     */
    std::string GetActiveLevelName() const;

    // === Level Loading ===

    /**
     * @brief Loads a level by name.
     */
    bool LoadLevel(const std::string& name);

    /**
     * @brief Unloads a level by name.
     */
    bool UnloadLevel(const std::string& name);

    /**
     * @brief Loads a level from a file.
     */
    std::shared_ptr<Level> LoadLevelFromFile(const std::string& filePath);

    /**
     * @brief Saves a level to a file.
     */
    bool SaveLevelToFile(const std::string& name, const std::string& filePath);

    /**
     * @brief Unloads all non-active levels.
     */
    void UnloadAllInactiveLevels();

    // === Streaming ===

    /**
     * @brief Enables/disables level streaming.
     */
    void SetStreamingEnabled(bool enabled) { streamingEnabled = enabled; }

    /**
     * @brief Checks if streaming is enabled.
     */
    bool IsStreamingEnabled() const { return streamingEnabled; }

    /**
     * @brief Sets the streaming viewer position.
     */
    void SetStreamingViewerPosition(const Vector3D& position) {
        streamingViewerPosition = position;
    }

    /**
     * @brief Gets the streaming viewer position.
     */
    Vector3D GetStreamingViewerPosition() const { return streamingViewerPosition; }

    /**
     * @brief Sets how often streaming checks occur (seconds).
     */
    void SetStreamingCheckInterval(float interval) {
        streamingCheckInterval = interval;
    }

    /**
     * @brief Manually triggers streaming check.
     */
    void CheckStreaming();

    // === Callbacks ===

    /**
     * @brief Adds a callback when a level loads.
     */
    void AddOnLevelLoadCallback(LevelLoadCallback callback);

    /**
     * @brief Adds a callback when a level unloads.
     */
    void AddOnLevelUnloadCallback(LevelUnloadCallback callback);

    // === Update ===

    /**
     * @brief Updates the world manager (streaming checks, etc.).
     */
    void Update(float deltaTime);

    PTX_BEGIN_FIELDS(WorldManager)
        PTX_FIELD(WorldManager, streamingEnabled, "Streaming enabled", 0, 1),
        PTX_FIELD(WorldManager, streamingCheckInterval, "Streaming check interval", 0, 0)
    PTX_END_FIELDS

    PTX_BEGIN_METHODS(WorldManager)
        PTX_METHOD_AUTO(WorldManager, SetStreamingEnabled, "Set streaming enabled"),
        PTX_METHOD_AUTO(WorldManager, IsStreamingEnabled, "Is streaming enabled"),
        PTX_METHOD_AUTO(WorldManager, GetLevelCount, "Get level count"),
        PTX_METHOD_AUTO(WorldManager, GetActiveLevelName, "Get active level name"),
        PTX_METHOD_AUTO(WorldManager, LoadLevel, "Load level"),
        PTX_METHOD_AUTO(WorldManager, UnloadLevel, "Unload level"),
        PTX_METHOD_AUTO(WorldManager, Update, "Update")
    PTX_END_METHODS

    PTX_BEGIN_DESCRIBE(WorldManager)
        PTX_CTOR(WorldManager, EntityManager*)
    PTX_END_DESCRIBE(WorldManager)
};

} // namespace ptx
