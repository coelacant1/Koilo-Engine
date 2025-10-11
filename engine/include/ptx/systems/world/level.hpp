/**
 * @file level.hpp
 * @brief Represents a game level/scene with entities and resources.
 *
 * @date 11/10/2025
 * @author Coela
 */

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include "../../registry/reflect_macros.hpp"
#include "../ecs/entity.hpp"

namespace ptx {

// Forward declarations
class EntityManager;
class Scene;  // Rendering scene (from systems/scene/scene.hpp)

/**
 * @enum LevelState
 * @brief Current state of a level.
 */
enum class LevelState {
    Unloaded,   ///< Not loaded in memory
    Loading,    ///< Currently being loaded
    Loaded,     ///< Fully loaded and ready
    Active,     ///< Currently active (updating)
    Unloading   ///< Currently being unloaded
};

/**
 * @class Level
 * @brief Represents a game level/scene.
 *
 * A level contains entities, resources, and metadata. Levels can be
 * loaded/unloaded dynamically and serialized to/from disk.
 */
class Level {
private:
    std::string name;                    ///< Level name
    std::string filePath;                ///< Path to level file
    LevelState state;                    ///< Current state
    std::vector<Entity> entities;        ///< Entities in this level
    std::unordered_map<std::string, std::string> metadata;  ///< Level metadata
    EntityManager* entityManager;        ///< Entity manager (not owned)
    Scene* renderScene;                  ///< Optional rendering scene (not owned)

    // Streaming bounds
    bool isStreamable;                   ///< Can this level be streamed?
    Vector3D streamingOrigin;            ///< Center point for streaming
    float streamingRadius;               ///< Radius for streaming activation

public:
    /**
     * @brief Constructor.
     */
    Level(const std::string& name = "Untitled Level");

    /**
     * @brief Destructor.
     */
    ~Level();

    // === Level Properties ===

    /**
     * @brief Gets the level name.
     */
    std::string GetName() const { return name; }

    /**
     * @brief Sets the level name.
     */
    void SetName(const std::string& newName) { name = newName; }

    /**
     * @brief Gets the level file path.
     */
    std::string GetFilePath() const { return filePath; }

    /**
     * @brief Sets the level file path.
     */
    void SetFilePath(const std::string& path) { filePath = path; }

    /**
     * @brief Gets the current level state.
     */
    LevelState GetState() const { return state; }

    /**
     * @brief Sets the entity manager.
     */
    void SetEntityManager(EntityManager* manager) { entityManager = manager; }

    // === Scene Integration ===

    /**
     * @brief Sets the rendering scene associated with this level.
     *
     * The Scene object (from systems/scene/) handles rendering/meshes,
     * while Level handles gameplay/entities. This links them together.
     */
    void SetRenderScene(Scene* scene) { renderScene = scene; }

    /**
     * @brief Gets the rendering scene associated with this level.
     */
    Scene* GetRenderScene() const { return renderScene; }

    /**
     * @brief Checks if this level has an associated render scene.
     */
    bool HasRenderScene() const { return renderScene != nullptr; }

    // === Entity Management ===

    /**
     * @brief Adds an entity to this level.
     */
    void AddEntity(Entity entity);

    /**
     * @brief Removes an entity from this level.
     */
    void RemoveEntity(Entity entity);

    /**
     * @brief Gets all entities in this level.
     */
    const std::vector<Entity>& GetEntities() const { return entities; }

    /**
     * @brief Clears all entities from this level.
     */
    void ClearEntities();

    /**
     * @brief Gets the number of entities in this level.
     */
    size_t GetEntityCount() const { return entities.size(); }

    // === Metadata ===

    /**
     * @brief Sets a metadata value.
     */
    void SetMetadata(const std::string& key, const std::string& value);

    /**
     * @brief Gets a metadata value.
     */
    std::string GetMetadata(const std::string& key) const;

    /**
     * @brief Checks if metadata key exists.
     */
    bool HasMetadata(const std::string& key) const;

    // === Streaming ===

    /**
     * @brief Sets whether this level can be streamed.
     */
    void SetStreamable(bool streamable) { isStreamable = streamable; }

    /**
     * @brief Checks if this level is streamable.
     */
    bool IsStreamable() const { return isStreamable; }

    /**
     * @brief Sets the streaming origin and radius.
     */
    void SetStreamingBounds(const Vector3D& origin, float radius);

    /**
     * @brief Gets the streaming origin.
     */
    Vector3D GetStreamingOrigin() const { return streamingOrigin; }

    /**
     * @brief Gets the streaming radius.
     */
    float GetStreamingRadius() const { return streamingRadius; }

    /**
     * @brief Checks if a position is within streaming range.
     */
    bool IsInStreamingRange(const Vector3D& position) const;

    // === Lifecycle ===

    /**
     * @brief Loads the level (internal state transition).
     */
    void Load();

    /**
     * @brief Unloads the level (internal state transition).
     */
    void Unload();

    /**
     * @brief Activates the level for updating.
     */
    void Activate();

    /**
     * @brief Deactivates the level.
     */
    void Deactivate();

    PTX_BEGIN_FIELDS(Level)
        PTX_FIELD(Level, name, "Name", 0, 0),
        PTX_FIELD(Level, filePath, "File path", 0, 0),
        PTX_FIELD(Level, isStreamable, "Streamable", 0, 1),
        PTX_FIELD(Level, streamingRadius, "Streaming radius", 0, 0)
    PTX_END_FIELDS

    PTX_BEGIN_METHODS(Level)
        PTX_METHOD_AUTO(Level, GetName, "Get name"),
        PTX_METHOD_AUTO(Level, SetName, "Set name"),
        PTX_METHOD_AUTO(Level, GetEntityCount, "Get entity count"),
        PTX_METHOD_AUTO(Level, IsStreamable, "Is streamable"),
        PTX_METHOD_AUTO(Level, Load, "Load"),
        PTX_METHOD_AUTO(Level, Unload, "Unload")
    PTX_END_METHODS

    PTX_BEGIN_DESCRIBE(Level)
        PTX_CTOR(Level, std::string)
    PTX_END_DESCRIBE(Level)
};

} // namespace ptx
