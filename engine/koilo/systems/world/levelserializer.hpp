// SPDX-License-Identifier: GPL-3.0-or-later
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
#include <functional>
#include "level.hpp"
#include <koilo/registry/reflect_macros.hpp>

namespace koilo {

// Forward declarations
class EntityManager;

/**
 * @enum SerializationFormat
 * @brief Format for level serialization.
 */
enum class SerializationFormat {
    JSON,       ///< JSON text format (human-readable)
    Binary      ///< Binary format (compact, fast)
};

/**
 * @struct SerializedEntity
 * @brief Serialized representation of an entity.
 */
struct SerializedEntity {
    EntityID id;                                         ///< Entity ID
    std::vector<std::string> componentTypes;             ///< Component type names
    std::vector<std::string> componentData;              ///< Serialized component data (JSON)

    KL_BEGIN_FIELDS(SerializedEntity)
        KL_FIELD(SerializedEntity, id, "Id", 0, 0)
    KL_END_FIELDS

    KL_BEGIN_METHODS(SerializedEntity)
        /* No reflected methods. */
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(SerializedEntity)
        /* No reflected ctors. */
    KL_END_DESCRIBE(SerializedEntity)

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

    SerializedLevel() : isStreamable(false), streamingRadius(0.0f) {}

    KL_BEGIN_FIELDS(SerializedLevel)
        KL_FIELD(SerializedLevel, name, "Name", 0, 0)
    KL_END_FIELDS

    KL_BEGIN_METHODS(SerializedLevel)
        /* No reflected methods. */
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(SerializedLevel)
        /* No reflected ctors. */
    KL_END_DESCRIBE(SerializedLevel)

};

/**
 * @struct ComponentSerializerEntry
 * @brief Type-erased component serialization entry.
 */
struct ComponentSerializerEntry {
    std::string typeName;
    std::function<bool(EntityManager*, Entity)> hasComponent;
    std::function<const void*(EntityManager*, Entity)> getComponent;
    std::function<void(EntityManager*, Entity, const std::string&)> addFromJSON;
    const ClassDesc* classDesc;

    KL_BEGIN_FIELDS(ComponentSerializerEntry)
        KL_FIELD(ComponentSerializerEntry, typeName, "Type name", 0, 0),
        KL_FIELD(ComponentSerializerEntry, classDesc, "Class desc", 0, 0)
    KL_END_FIELDS

    KL_BEGIN_METHODS(ComponentSerializerEntry)
        /* ComponentSerializerEntry is engine-internal */
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(ComponentSerializerEntry)
        /* No reflected ctors. */
    KL_END_DESCRIBE(ComponentSerializerEntry)

};

/**
 * @class LevelSerializer
 * @brief Handles serialization and deserialization of levels.
 *
 * Uses the reflection system and a component registry to serialize
 * entity component data without compile-time type knowledge.
 */
class LevelSerializer {
private:
    EntityManager* entityManager;
    SerializationFormat format;

public:
    LevelSerializer(EntityManager* entityManager = nullptr,
                    SerializationFormat format = SerializationFormat::JSON);
    ~LevelSerializer();

    // === Configuration ===
    void SetEntityManager(EntityManager* manager) { entityManager = manager; }
    EntityManager* GetEntityManager() const { return entityManager; }
    void SetFormat(SerializationFormat newFormat) { format = newFormat; }
    SerializationFormat GetFormat() const { return format; }

    // === Component Registry ===

    /**
     * @brief Initializes the component type registry. Call once at startup.
     */
    static void InitializeComponentRegistry();

    /**
     * @brief Gets the registered component serializers.
     */
    static std::vector<ComponentSerializerEntry>& GetComponentRegistry();

    // === Serialization ===
    bool SerializeLevelToFile(std::shared_ptr<Level> level, const std::string& filePath);
    std::shared_ptr<Level> DeserializeLevelFromFile(const std::string& filePath);
    std::string SerializeLevelToString(std::shared_ptr<Level> level);
    std::shared_ptr<Level> DeserializeLevelFromString(const std::string& data);

    // === Entity Serialization ===
    SerializedEntity SerializeEntity(Entity entity);
    Entity DeserializeEntity(const SerializedEntity& serializedEntity);

    KL_BEGIN_FIELDS(LevelSerializer)
    KL_END_FIELDS

    KL_BEGIN_METHODS(LevelSerializer)
        KL_METHOD_AUTO(LevelSerializer, SerializeLevelToFile, "Serialize level to file"),
        KL_METHOD_AUTO(LevelSerializer, DeserializeLevelFromFile, "Deserialize level from file")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(LevelSerializer)
        KL_CTOR0(LevelSerializer)
    KL_END_DESCRIBE(LevelSerializer)

private:
    bool SerializeToJSON(const SerializedLevel& level, const std::string& filePath);
    bool SerializeToBinary(const SerializedLevel& level, const std::string& filePath);
    std::string SerializeToJSONString(const SerializedLevel& level);

    SerializedLevel DeserializeFromJSON(const std::string& filePath);
    SerializedLevel DeserializeFromBinary(const std::string& filePath);
    SerializedLevel DeserializeFromJSONString(const std::string& json);

    SerializedLevel BuildSerializedLevel(std::shared_ptr<Level> level);
    std::shared_ptr<Level> RebuildLevel(const SerializedLevel& serialized, const std::string& filePath = "");
};

} // namespace koilo
