/**
 * @file levelserializer.hpp
 * @brief Serialization/deserialization for levels (gameplay entity containers).
 *
 * Note: This serializes Level objects (gameplay/entity data), not Scene objects (rendering/mesh data).
 *
 * @date 11/10/2025
 * @author Coela
 */

#pragma once

#include <string>
#include <memory>
#include <vector>
#include "level.hpp"
#include "../../registry/reflect_macros.hpp"

namespace ptx {

// Forward declarations
class EntityManager;

/**
 * @enum SerializationFormat
 * @brief Format for level serialization.
 */
enum class SerializationFormat {
    JSON,       ///< JSON text format (human-readable)
    Binary,     ///< Binary format (compact, fast)
    XML         ///< XML format (verbose, compatible)
};

/**
 * @struct SerializedEntity
 * @brief Serialized representation of an entity.
 */
struct SerializedEntity {
    EntityID id;                                         ///< Entity ID
    std::vector<std::string> componentTypes;             ///< Component type names
    std::vector<std::string> componentData;              ///< Serialized component data
};

/**
 * @struct SerializedLevel
 * @brief Serialized representation of a level.
 */
struct SerializedLevel {
    std::string name;                                    ///< Level name
    std::unordered_map<std::string, std::string> metadata;  ///< Level metadata
    std::vector<SerializedEntity> entities;              ///< Serialized entities
    bool isStreamable;                                   ///< Streaming flag
    Vector3D streamingOrigin;                            ///< Streaming origin
    float streamingRadius;                               ///< Streaming radius
};

/**
 * @class LevelSerializer
 * @brief Handles serialization and deserialization of levels (gameplay entity containers).
 *
 * The LevelSerializer can save levels to disk in various formats
 * and load them back, recreating all entities and components.
 *
 * Note: This is for Level objects (gameplay/entities), not Scene objects (rendering/meshes).
 */
class LevelSerializer {
private:
    EntityManager* entityManager;        ///< Entity manager (not owned)
    SerializationFormat format;          ///< Current format

public:
    /**
     * @brief Constructor.
     */
    LevelSerializer(EntityManager* entityManager = nullptr,
                    SerializationFormat format = SerializationFormat::JSON);

    /**
     * @brief Destructor.
     */
    ~LevelSerializer();

    // === Configuration ===

    /**
     * @brief Sets the entity manager.
     */
    void SetEntityManager(EntityManager* manager) { entityManager = manager; }

    /**
     * @brief Gets the entity manager.
     */
    EntityManager* GetEntityManager() const { return entityManager; }

    /**
     * @brief Sets the serialization format.
     */
    void SetFormat(SerializationFormat newFormat) { format = newFormat; }

    /**
     * @brief Gets the serialization format.
     */
    SerializationFormat GetFormat() const { return format; }

    // === Serialization ===

    /**
     * @brief Serializes a level to a file.
     */
    bool SerializeLevelToFile(std::shared_ptr<Level> level, const std::string& filePath);

    /**
     * @brief Deserializes a level from a file.
     */
    std::shared_ptr<Level> DeserializeLevelFromFile(const std::string& filePath);

    /**
     * @brief Serializes a level to a string.
     */
    std::string SerializeLevelToString(std::shared_ptr<Level> level);

    /**
     * @brief Deserializes a level from a string.
     */
    std::shared_ptr<Level> DeserializeLevelFromString(const std::string& data);

    // === Entity Serialization ===

    /**
     * @brief Serializes an entity.
     */
    SerializedEntity SerializeEntity(Entity entity);

    /**
     * @brief Deserializes an entity.
     */
    Entity DeserializeEntity(const SerializedEntity& serializedEntity);

    // === Component Serialization ===

    /**
     * @brief Serializes a component to a string.
     */
    template<typename T>
    std::string SerializeComponent(const T& component);

    /**
     * @brief Deserializes a component from a string.
     */
    template<typename T>
    T DeserializeComponent(const std::string& data);

    PTX_BEGIN_FIELDS(LevelSerializer)
    PTX_END_FIELDS

    PTX_BEGIN_METHODS(LevelSerializer)
        PTX_METHOD_AUTO(LevelSerializer, SerializeLevelToFile, "Serialize level to file"),
        PTX_METHOD_AUTO(LevelSerializer, DeserializeLevelFromFile, "Deserialize level from file")
    PTX_END_METHODS

    PTX_BEGIN_DESCRIBE(LevelSerializer)
        PTX_CTOR(LevelSerializer, EntityManager*, SerializationFormat)
    PTX_END_DESCRIBE(LevelSerializer)

private:
    // Format-specific helpers
    bool SerializeToJSON(const SerializedLevel& level, const std::string& filePath);
    bool SerializeToBinary(const SerializedLevel& level, const std::string& filePath);
    bool SerializeToXML(const SerializedLevel& level, const std::string& filePath);

    SerializedLevel DeserializeFromJSON(const std::string& filePath);
    SerializedLevel DeserializeFromBinary(const std::string& filePath);
    SerializedLevel DeserializeFromXML(const std::string& filePath);
};

// === Template Implementations ===

template<typename T>
std::string LevelSerializer::SerializeComponent(const T& component) {
    // TODO: Use reflection system to serialize component fields
    // For now, return empty string
    return "";
}

template<typename T>
T LevelSerializer::DeserializeComponent(const std::string& data) {
    // TODO: Use reflection system to deserialize component fields
    // For now, return default-constructed component
    return T();
}

} // namespace ptx
