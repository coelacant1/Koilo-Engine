// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file reflectionserializer.hpp
 * @brief Reflection-based JSON serialization for any Described class.
 *
 * Uses ClassDesc field metadata to serialize/deserialize objects without
 * requiring type-specific code. Handles nested reflected types recursively.
 *
 * @date 02/22/2026
 * @author Coela
 */

#pragma once

#include <string>
#include <koilo/registry/registry.hpp>
#include <koilo/registry/reflect_macros.hpp>

namespace koilo {

/**
 * @class ReflectionSerializer
 * @brief Serializes/deserializes any reflected object to/from JSON.
 *
 * Supported field types: float, double, int, bool, uint32_t, uint64_t,
 * std::string, and any type with a ClassDesc in the global registry
 * (Vector3D, Quaternion, Transform, etc.).
 */
class ReflectionSerializer {
public:
    ReflectionSerializer() = default;
    ~ReflectionSerializer() = default;

    /**
     * @brief Serializes an object to a JSON string.
     * @param obj Pointer to the object
     * @param desc ClassDesc for the object type
     * @return JSON string representation
     */
    static std::string SerializeToJSON(const void* obj, const ClassDesc* desc);

    /**
     * @brief Deserializes a JSON string into an object.
     * @param obj Pointer to the object to populate
     * @param desc ClassDesc for the object type
     * @param json JSON string to parse
     * @return true if successful
     */
    static bool DeserializeFromJSON(void* obj, const ClassDesc* desc, const std::string& json);

    /**
     * @brief Finds a ClassDesc by type_info in the global registry.
     */
    static const ClassDesc* FindClassByType(const std::type_info* type);

    KL_BEGIN_FIELDS(ReflectionSerializer)
    KL_END_FIELDS

    KL_BEGIN_METHODS(ReflectionSerializer)
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(ReflectionSerializer)
        KL_CTOR0(ReflectionSerializer)
    KL_END_DESCRIBE(ReflectionSerializer)

private:
    static std::string SerializeField(const void* fieldPtr, const std::type_info* type);
    static bool DeserializeField(void* fieldPtr, const std::type_info* type, const std::string& value);
    static std::string EscapeJSON(const std::string& str);
    static std::string UnescapeJSON(const std::string& str);
    static std::string Trim(const std::string& str);
    static size_t FindMatchingBrace(const std::string& json, size_t openPos);
};

} // namespace koilo
