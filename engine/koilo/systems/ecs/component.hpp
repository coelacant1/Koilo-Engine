// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file component.hpp
 * @brief Component type identification for ECS.
 *
 * @date 11/10/2025
 * @author Coela
 */

#pragma once

#include <cstdint>

namespace koilo {

/**
 * @typedef ComponentTypeID
 * @brief Unique type identifier for component types.
 */
using ComponentTypeID = uint32_t;

/**
 * @class ComponentTypeIDGenerator
 * @brief Generates unique IDs for component types.
 */
class ComponentTypeIDGenerator {
private:
    static ComponentTypeID counter;

public:
    /**
     * @brief Gets unique ID for a component type.
     */
    template<typename T>
    static ComponentTypeID GetID() {
        static ComponentTypeID id = counter++;
        return id;
    }

    /**
     * @brief Gets the total number of registered component types.
     */
    static ComponentTypeID GetCount() {
        return counter;
    }
};

/**
 * @brief Helper function to get component type ID.
 */
template<typename T>
inline ComponentTypeID GetComponentTypeID() {
    return ComponentTypeIDGenerator::GetID<T>();
}

/**
 * @brief Concept for component types (must be struct/class, not pointer).
 * NOTE: Requires C++20. Commented out for C++17 compatibility.
 */
// template<typename T>
// concept ComponentType = std::is_class_v<T> && !std::is_pointer_v<T>;

} // namespace koilo
