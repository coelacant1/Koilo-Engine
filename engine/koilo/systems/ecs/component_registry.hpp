// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file component_registry.hpp
 * @brief Runtime component type registry for modular ECS.
 *
 * Modules register component types at runtime. IDs are assigned
 * sequentially starting after the last compile-time ID.
 *
 * @date 03/30/2026
 * @author Coela
 */
#pragma once

#include <vector>
#include <string>
#include <unordered_map>
#include <memory>
#include "component.hpp"
#include "component_type.hpp"

namespace koilo {

/**
 * @class ComponentRegistry
 * @brief Central registry for runtime component type metadata.
 *
 * Compile-time component IDs (from ComponentTypeIDGenerator) are below
 * the base; runtime IDs start at the current compile-time count.
 */
class ComponentRegistry {
public:
    ComponentRegistry();

    /**
     * @brief Registers a runtime component type.
     * @param type  Owned pointer to the type descriptor.
     * @return The assigned ComponentTypeID.
     */
    ComponentTypeID Register(std::unique_ptr<IComponentType> type);

    /**
     * @brief Convenience: register a compile-time type T by name.
     * @return The assigned runtime ComponentTypeID.
     */
    template<typename T>
    ComponentTypeID Register(const char* name) {
        return Register(std::make_unique<ComponentType<T>>(name));
    }

    /**
     * @brief Finds a component type by name.
     * @return Pointer to the descriptor, or nullptr.
     */
    const IComponentType* FindByName(const std::string& name) const;

    /**
     * @brief Finds the ComponentTypeID for a named component.
     * @return The ID, or UINT32_MAX if not found.
     */
    ComponentTypeID FindIDByName(const std::string& name) const;

    /**
     * @brief Gets a type descriptor by its runtime ID.
     * @return Pointer to the descriptor, or nullptr.
     */
    const IComponentType* GetType(ComponentTypeID id) const;

    /**
     * @brief Lists all registered runtime component type names.
     */
    std::vector<std::string> List() const;

    /**
     * @brief Returns the number of runtime-registered types.
     */
    size_t Count() const;

private:
    ComponentTypeID nextId_;

    struct Entry {
        ComponentTypeID                 id;
        std::unique_ptr<IComponentType> type;
    };

    std::vector<Entry> entries_;
    std::unordered_map<std::string, size_t> nameIndex_;
};

} // namespace koilo
