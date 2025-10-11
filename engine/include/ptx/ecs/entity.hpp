/**
 * @file entity.hpp
 * @brief Entity handle for ECS system.
 *
 * @date 11/10/2025
 * @author Coela
 */

#pragma once

#include <cstdint>
#include "../../registry/reflect_macros.hpp"

namespace ptx {

/**
 * @typedef EntityID
 * @brief Unique identifier for an entity (64-bit).
 *
 * Layout:
 * - Lower 32 bits: Entity index
 * - Upper 32 bits: Generation (for detecting stale handles)
 */
using EntityID = uint64_t;

/**
 * @class Entity
 * @brief Lightweight handle to an entity in the ECS.
 */
class Entity {
private:
    EntityID id;

public:
    /**
     * @brief Default constructor (null entity).
     */
    Entity() : id(0) {}

    /**
     * @brief Constructor with ID.
     */
    explicit Entity(EntityID id) : id(id) {}

    /**
     * @brief Gets the entity ID.
     */
    EntityID GetID() const { return id; }

    /**
     * @brief Gets the entity index (lower 32 bits).
     */
    uint32_t GetIndex() const { return static_cast<uint32_t>(id & 0xFFFFFFFF); }

    /**
     * @brief Gets the entity generation (upper 32 bits).
     */
    uint32_t GetGeneration() const { return static_cast<uint32_t>((id >> 32) & 0xFFFFFFFF); }

    /**
     * @brief Checks if entity is null.
     */
    bool IsNull() const { return id == 0; }

    /**
     * @brief Checks if entity is valid (not null).
     */
    bool IsValid() const { return id != 0; }

    /**
     * @brief Equality operator.
     */
    bool operator==(const Entity& other) const { return id == other.id; }

    /**
     * @brief Inequality operator.
     */
    bool operator!=(const Entity& other) const { return id != other.id; }

    /**
     * @brief Less-than operator (for sorting/maps).
     */
    bool operator<(const Entity& other) const { return id < other.id; }

    /**
     * @brief Bool conversion operator.
     */
    explicit operator bool() const { return IsValid(); }

    /**
     * @brief Creates an entity ID from index and generation.
     */
    static EntityID MakeID(uint32_t index, uint32_t generation) {
        return (static_cast<uint64_t>(generation) << 32) | static_cast<uint64_t>(index);
    }

    PTX_BEGIN_FIELDS(Entity)
        PTX_FIELD(Entity, id, "ID", 0, 0)
    PTX_END_FIELDS

    PTX_BEGIN_METHODS(Entity)
        PTX_METHOD_AUTO(Entity, GetID, "Get ID"),
        PTX_METHOD_AUTO(Entity, GetIndex, "Get index"),
        PTX_METHOD_AUTO(Entity, GetGeneration, "Get generation"),
        PTX_METHOD_AUTO(Entity, IsNull, "Is null"),
        PTX_METHOD_AUTO(Entity, IsValid, "Is valid")
    PTX_END_METHODS

    PTX_BEGIN_DESCRIBE(Entity)
        PTX_CTOR0(Entity),
        PTX_CTOR(Entity, uint64_t)
    PTX_END_DESCRIBE(Entity)
};

/**
 * @brief Null entity constant.
 */
constexpr EntityID NULL_ENTITY = 0;

} // namespace ptx

// Hash function for Entity (for unordered_map/set)
namespace std {
    template<>
    struct hash<ptx::Entity> {
        size_t operator()(const ptx::Entity& entity) const {
            return hash<ptx::EntityID>()(entity.GetID());
        }
    };
}
