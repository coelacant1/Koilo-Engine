// SPDX-License-Identifier: GPL-3.0-or-later
#include <koilo/systems/world/reflectionserializer.hpp>
#include <koilo/registry/global_registry.hpp>
#include <koilo/core/math/vector3d.hpp>
#include <koilo/core/math/quaternion.hpp>
#include <koilo/core/math/transform.hpp>
#include <sstream>

namespace koilo {

// --- Type checks using type_info ---

static bool IsFloat(const std::type_info* t) { return *t == typeid(float); }
static bool IsDouble(const std::type_info* t) { return *t == typeid(double); }
static bool IsInt(const std::type_info* t) { return *t == typeid(int); }
static bool IsUInt32(const std::type_info* t) { return *t == typeid(uint32_t); }
static bool IsUInt64(const std::type_info* t) { return *t == typeid(uint64_t); }
static bool IsBool(const std::type_info* t) { return *t == typeid(bool); }
static bool IsString(const std::type_info* t) { return *t == typeid(std::string); }

// Known nested types and their ClassDesc names
struct KnownType {
    const std::type_info* typeInfo;
    const char* className;
};

static const KnownType kKnownTypes[] = {
    { &typeid(Vector3D),   "Vector3D" },
    { &typeid(Quaternion), "Quaternion" },
    { &typeid(Transform),  "Transform" },
};

// --- Serialize ---

std::string ReflectionSerializer::SerializeToJSON(const void* obj, const ClassDesc* desc) {
    if (!obj || !desc) return "{}";

    std::ostringstream out;
    out << "{";

    bool first = true;
    for (size_t i = 0; i < desc->fields.count; ++i) {
        const FieldDecl& field = desc->fields.data[i];
        const void* fieldPtr = field.access.get_cptr(obj);

        if (!first) out << ", ";
        first = false;

        out << "\"" << field.name << "\": ";
        out << SerializeField(fieldPtr, field.type);
    }

    out << "}";
    return out.str();
}

std::string ReflectionSerializer::SerializeField(const void* fieldPtr, const std::type_info* type) {
    if (!fieldPtr || !type) return "null";

    if (IsFloat(type)) {
        std::ostringstream s;
        s << *static_cast<const float*>(fieldPtr);
        return s.str();
    }
    if (IsDouble(type)) {
        std::ostringstream s;
        s << *static_cast<const double*>(fieldPtr);
        return s.str();
    }
    if (IsInt(type)) {
        return std::to_string(*static_cast<const int*>(fieldPtr));
    }
    if (IsUInt32(type)) {
        return std::to_string(*static_cast<const uint32_t*>(fieldPtr));
    }
    if (IsUInt64(type)) {
        return std::to_string(*static_cast<const uint64_t*>(fieldPtr));
    }
    if (IsBool(type)) {
        return *static_cast<const bool*>(fieldPtr) ? "true" : "false";
    }
    if (IsString(type)) {
        return "\"" + EscapeJSON(*static_cast<const std::string*>(fieldPtr)) + "\"";
    }

    // Check if it's a reflected type (Vector3D, Quaternion, Transform, etc.)
    const ClassDesc* nestedDesc = FindClassByType(type);
    if (nestedDesc) {
        return SerializeToJSON(fieldPtr, nestedDesc);
    }

    return "null";
}

// --- Deserialize ---

bool ReflectionSerializer::DeserializeFromJSON(void* obj, const ClassDesc* desc, const std::string& json) {
    if (!obj || !desc) return false;

    std::string trimmed = Trim(json);
    if (trimmed.size() < 2 || trimmed.front() != '{' || trimmed.back() != '}') return false;

    // Strip outer braces
    std::string inner = trimmed.substr(1, trimmed.size() - 2);

    // Parse key-value pairs
    size_t pos = 0;
    while (pos < inner.size()) {
        // Skip whitespace
        while (pos < inner.size() && (inner[pos] == ' ' || inner[pos] == '\n' || inner[pos] == '\r' || inner[pos] == '\t'))
            ++pos;
        if (pos >= inner.size()) break;

        // Expect opening quote for key
        if (inner[pos] != '"') { ++pos; continue; }
        ++pos;
        size_t keyStart = pos;
        while (pos < inner.size() && inner[pos] != '"') ++pos;
        std::string key = inner.substr(keyStart, pos - keyStart);
        ++pos; // skip closing quote

        // Skip colon and whitespace
        while (pos < inner.size() && (inner[pos] == ' ' || inner[pos] == ':' || inner[pos] == '\t'))
            ++pos;

        // Extract value
        std::string value;
        if (pos < inner.size() && inner[pos] == '{') {
            size_t end = FindMatchingBrace(inner, pos);
            value = inner.substr(pos, end - pos + 1);
            pos = end + 1;
        } else if (pos < inner.size() && inner[pos] == '"') {
            ++pos;
            size_t valStart = pos;
            while (pos < inner.size() && inner[pos] != '"') {
                if (inner[pos] == '\\') ++pos; // skip escaped char
                ++pos;
            }
            value = "\"" + inner.substr(valStart, pos - valStart) + "\"";
            ++pos; // skip closing quote
        } else {
            size_t valStart = pos;
            while (pos < inner.size() && inner[pos] != ',' && inner[pos] != '}')
                ++pos;
            value = Trim(inner.substr(valStart, pos - valStart));
        }

        // Find the field in the descriptor
        for (size_t i = 0; i < desc->fields.count; ++i) {
            const FieldDecl& field = desc->fields.data[i];
            if (key == field.name) {
                void* fieldPtr = field.access.get_ptr(obj);
                DeserializeField(fieldPtr, field.type, value);
                break;
            }
        }

        // Skip comma
        while (pos < inner.size() && (inner[pos] == ',' || inner[pos] == ' ' || inner[pos] == '\n' || inner[pos] == '\r' || inner[pos] == '\t'))
            ++pos;
    }

    return true;
}

bool ReflectionSerializer::DeserializeField(void* fieldPtr, const std::type_info* type, const std::string& value) {
    if (!fieldPtr || !type || value.empty()) return false;

    std::string trimmed = Trim(value);

    if (IsFloat(type)) {
        *static_cast<float*>(fieldPtr) = std::stof(trimmed);
        return true;
    }
    if (IsDouble(type)) {
        *static_cast<double*>(fieldPtr) = std::stod(trimmed);
        return true;
    }
    if (IsInt(type)) {
        *static_cast<int*>(fieldPtr) = std::stoi(trimmed);
        return true;
    }
    if (IsUInt32(type)) {
        *static_cast<uint32_t*>(fieldPtr) = static_cast<uint32_t>(std::stoul(trimmed));
        return true;
    }
    if (IsUInt64(type)) {
        *static_cast<uint64_t*>(fieldPtr) = std::stoull(trimmed);
        return true;
    }
    if (IsBool(type)) {
        *static_cast<bool*>(fieldPtr) = (trimmed == "true" || trimmed == "1");
        return true;
    }
    if (IsString(type)) {
        // Strip quotes
        if (trimmed.size() >= 2 && trimmed.front() == '"' && trimmed.back() == '"') {
            *static_cast<std::string*>(fieldPtr) = UnescapeJSON(trimmed.substr(1, trimmed.size() - 2));
        } else {
            *static_cast<std::string*>(fieldPtr) = trimmed;
        }
        return true;
    }

    // Nested reflected object
    const ClassDesc* nestedDesc = FindClassByType(type);
    if (nestedDesc && trimmed.front() == '{') {
        return DeserializeFromJSON(fieldPtr, nestedDesc, trimmed);
    }

    return false;
}

// --- Utilities ---

const ClassDesc* ReflectionSerializer::FindClassByType(const std::type_info* type) {
    if (!type) return nullptr;

    auto& nameMap = ClassRegistryMap();
    for (const auto& known : kKnownTypes) {
        if (*type == *known.typeInfo) {
            auto it = nameMap.find(known.className);
            if (it != nameMap.end()) return it->second;
        }
    }

    return nullptr;
}

std::string ReflectionSerializer::EscapeJSON(const std::string& str) {
    std::string result;
    result.reserve(str.size());
    for (char c : str) {
        switch (c) {
            case '"':  result += "\\\""; break;
            case '\\': result += "\\\\"; break;
            case '\n': result += "\\n"; break;
            case '\r': result += "\\r"; break;
            case '\t': result += "\\t"; break;
            default:   result += c; break;
        }
    }
    return result;
}

std::string ReflectionSerializer::UnescapeJSON(const std::string& str) {
    std::string result;
    result.reserve(str.size());
    for (size_t i = 0; i < str.size(); ++i) {
        if (str[i] == '\\' && i + 1 < str.size()) {
            switch (str[i + 1]) {
                case '"':  result += '"'; ++i; break;
                case '\\': result += '\\'; ++i; break;
                case 'n':  result += '\n'; ++i; break;
                case 'r':  result += '\r'; ++i; break;
                case 't':  result += '\t'; ++i; break;
                default:   result += str[i]; break;
            }
        } else {
            result += str[i];
        }
    }
    return result;
}

std::string ReflectionSerializer::Trim(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) return "";
    size_t end = str.find_last_not_of(" \t\n\r");
    return str.substr(start, end - start + 1);
}

size_t ReflectionSerializer::FindMatchingBrace(const std::string& json, size_t openPos) {
    int depth = 0;
    bool inString = false;
    for (size_t i = openPos; i < json.size(); ++i) {
        if (json[i] == '"' && (i == 0 || json[i - 1] != '\\')) {
            inString = !inString;
        }
        if (!inString) {
            if (json[i] == '{') ++depth;
            else if (json[i] == '}') {
                --depth;
                if (depth == 0) return i;
            }
        }
    }
    return json.size() - 1;
}

} // namespace koilo
