// SPDX-License-Identifier: GPL-3.0-or-later
#include <koilo/systems/world/levelserializer.hpp>
#include <koilo/systems/world/reflectionserializer.hpp>
#include <koilo/ecs/entitymanager.hpp>
#include <koilo/ecs/components/tagcomponent.hpp>
#include <koilo/ecs/components/transformcomponent.hpp>
#include <koilo/ecs/components/velocitycomponent.hpp>
#include <koilo/core/math/vector3d.hpp>
#include <koilo/core/math/quaternion.hpp>
#include <koilo/core/math/transform.hpp>
#include <koilo/scripting/reflection_bridge.hpp>
#include <fstream>
#include <sstream>
#include <iostream>

namespace koilo {

// === Component Registry (type-erased, template code stays in .cpp) ===

std::vector<ComponentSerializerEntry>& LevelSerializer::GetComponentRegistry() {
    static std::vector<ComponentSerializerEntry> registry;
    return registry;
}

namespace {
template<typename T>
void RegisterComponent(const std::string& name) {
    T::Describe();
    const ClassDesc* desc = scripting::ReflectionBridge::FindClass(name.c_str());
    if (!desc) return;

    ComponentSerializerEntry entry;
    entry.typeName = name;
    entry.classDesc = desc;
    entry.hasComponent = [](EntityManager* em, Entity e) -> bool {
        return em->HasComponent<T>(e);
    };
    entry.getComponent = [](EntityManager* em, Entity e) -> const void* {
        return em->GetComponent<T>(e);
    };
    entry.addFromJSON = [desc](EntityManager* em, Entity e, const std::string& json) {
        T comp;
        ReflectionSerializer::DeserializeFromJSON(&comp, desc, json);
        em->AddComponent<T>(e, comp);
    };
    LevelSerializer::GetComponentRegistry().push_back(entry);
}
} // anonymous namespace

void LevelSerializer::InitializeComponentRegistry() {
    auto& reg = GetComponentRegistry();
    if (!reg.empty()) return; // already initialized
    // Ensure nested reflected types are described for serialization
    Vector3D::Describe();
    Quaternion::Describe();
    Transform::Describe();
    RegisterComponent<TagComponent>("TagComponent");
    RegisterComponent<TransformComponent>("TransformComponent");
    RegisterComponent<VelocityComponent>("VelocityComponent");
}

// === Constructor / Destructor ===

LevelSerializer::LevelSerializer(EntityManager* entityManager, SerializationFormat format)
    : entityManager(entityManager), format(format) {
    InitializeComponentRegistry();
}

LevelSerializer::~LevelSerializer() {}

// === High-Level API ===

SerializedLevel LevelSerializer::BuildSerializedLevel(std::shared_ptr<Level> level) {
    SerializedLevel sl;
    sl.name = level->GetName();
    sl.isStreamable = level->IsStreamable();
    sl.streamingOrigin = level->GetStreamingOrigin();
    sl.streamingRadius = level->GetStreamingRadius();
    for (const Entity& entity : level->GetEntities()) {
        sl.entities.push_back(SerializeEntity(entity));
    }
    return sl;
}

std::shared_ptr<Level> LevelSerializer::RebuildLevel(const SerializedLevel& sl, const std::string& filePath) {
    auto level = std::make_shared<Level>(sl.name);
    level->SetStreamable(sl.isStreamable);
    level->SetStreamingBounds(sl.streamingOrigin, sl.streamingRadius);
    if (!filePath.empty()) level->SetFilePath(filePath);
    level->SetEntityManager(entityManager);
    if (entityManager) {
        for (const SerializedEntity& se : sl.entities) {
            Entity entity = DeserializeEntity(se);
            level->AddEntity(entity);
        }
    }
    return level;
}

bool LevelSerializer::SerializeLevelToFile(std::shared_ptr<Level> level, const std::string& filePath) {
    if (!level) return false;
    SerializedLevel sl = BuildSerializedLevel(level);
    switch (format) {
        case SerializationFormat::JSON:   return SerializeToJSON(sl, filePath);
        case SerializationFormat::Binary: return SerializeToBinary(sl, filePath);
        default: return false;
    }
}

std::shared_ptr<Level> LevelSerializer::DeserializeLevelFromFile(const std::string& filePath) {
    SerializedLevel sl;
    switch (format) {
        case SerializationFormat::JSON:   sl = DeserializeFromJSON(filePath); break;
        case SerializationFormat::Binary: sl = DeserializeFromBinary(filePath); break;
        default: return nullptr;
    }
    return RebuildLevel(sl, filePath);
}

std::string LevelSerializer::SerializeLevelToString(std::shared_ptr<Level> level) {
    if (!level) return "";
    SerializedLevel sl = BuildSerializedLevel(level);
    return SerializeToJSONString(sl);
}

std::shared_ptr<Level> LevelSerializer::DeserializeLevelFromString(const std::string& data) {
    SerializedLevel sl = DeserializeFromJSONString(data);
    return RebuildLevel(sl);
}

// === Entity Serialization ===

SerializedEntity LevelSerializer::SerializeEntity(Entity entity) {
    SerializedEntity result;
    result.id = entity.GetID();
    if (!entityManager) return result;

    for (auto& entry : GetComponentRegistry()) {
        if (entry.hasComponent(entityManager, entity)) {
            const void* comp = entry.getComponent(entityManager, entity);
            result.componentTypes.push_back(entry.typeName);
            result.componentData.push_back(
                ReflectionSerializer::SerializeToJSON(comp, entry.classDesc));
        }
    }
    return result;
}

Entity LevelSerializer::DeserializeEntity(const SerializedEntity& se) {
    if (!entityManager) return Entity(0);
    Entity entity = entityManager->CreateEntity();

    for (size_t i = 0; i < se.componentTypes.size(); ++i) {
        for (auto& entry : GetComponentRegistry()) {
            if (entry.typeName == se.componentTypes[i]) {
                entry.addFromJSON(entityManager, entity, se.componentData[i]);
                break;
            }
        }
    }
    return entity;
}

// === JSON Serialization ===

std::string LevelSerializer::SerializeToJSONString(const SerializedLevel& level) {
    std::ostringstream out;
    out << "{\n";
    out << "  \"name\": \"" << level.name << "\",\n";
    out << "  \"isStreamable\": " << (level.isStreamable ? "true" : "false") << ",\n";
    out << "  \"streamingOrigin\": [" << level.streamingOrigin.X << ", "
        << level.streamingOrigin.Y << ", " << level.streamingOrigin.Z << "],\n";
    out << "  \"streamingRadius\": " << level.streamingRadius << ",\n";
    out << "  \"metadata\": {";
    {
        bool first = true;
        for (auto& kv : level.metadata) {
            if (!first) out << ",";
            first = false;
            out << "\n    \"" << kv.first << "\": \"" << kv.second << "\"";
        }
        if (!level.metadata.empty()) out << "\n  ";
    }
    out << "},\n";
    out << "  \"entities\": [\n";
    for (size_t i = 0; i < level.entities.size(); ++i) {
        const SerializedEntity& e = level.entities[i];
        out << "    {\n";
        out << "      \"id\": " << e.id << ",\n";
        out << "      \"components\": [\n";
        for (size_t j = 0; j < e.componentTypes.size(); ++j) {
            out << "        {\"type\": \"" << e.componentTypes[j] << "\", \"data\": "
                << e.componentData[j] << "}";
            if (j < e.componentTypes.size() - 1) out << ",";
            out << "\n";
        }
        out << "      ]\n";
        out << "    }";
        if (i < level.entities.size() - 1) out << ",";
        out << "\n";
    }
    out << "  ]\n";
    out << "}\n";
    return out.str();
}

bool LevelSerializer::SerializeToJSON(const SerializedLevel& level, const std::string& filePath) {
    std::ofstream file(filePath);
    if (!file.is_open()) return false;
    file << SerializeToJSONString(level);
    file.close();
    return true;
}

// --- JSON Deserialization (simple parser for the format we generate) ---

static std::string TrimWS(const std::string& s) {
    size_t a = s.find_first_not_of(" \t\n\r");
    if (a == std::string::npos) return "";
    return s.substr(a, s.find_last_not_of(" \t\n\r") - a + 1);
}

static std::string ExtractStringValue(const std::string& line, const std::string& key) {
    size_t pos = line.find("\"" + key + "\"");
    if (pos == std::string::npos) return "";
    pos = line.find(':', pos);
    if (pos == std::string::npos) return "";
    ++pos;
    // Skip whitespace after colon
    while (pos < line.size() && (line[pos] == ' ' || line[pos] == '\t')) ++pos;

    if (pos < line.size() && line[pos] == '"') {
        // Quoted string value - find closing quote
        ++pos;
        size_t start = pos;
        while (pos < line.size() && line[pos] != '"') {
            if (line[pos] == '\\') ++pos;
            ++pos;
        }
        return line.substr(start, pos - start);
    }

    // Non-string value - read until comma, brace, or bracket
    size_t start = pos;
    while (pos < line.size() && line[pos] != ',' && line[pos] != '}' && line[pos] != ']')
        ++pos;
    return TrimWS(line.substr(start, pos - start));
}

SerializedLevel LevelSerializer::DeserializeFromJSONString(const std::string& json) {
    SerializedLevel level;
    std::istringstream stream(json);
    std::string line;

    enum { TOP, IN_ENTITIES, IN_ENTITY, IN_COMPONENTS, IN_COMPONENT } state = TOP;
    SerializedEntity currentEntity;
    int braceDepth = 0;
    std::string componentAccum;

    while (std::getline(stream, line)) {
        std::string trimmed = TrimWS(line);
        if (trimmed.empty()) continue;

        switch (state) {
        case TOP:
            if (trimmed.find("\"name\"") != std::string::npos) {
                level.name = ExtractStringValue(trimmed, "name");
            } else if (trimmed.find("\"isStreamable\"") != std::string::npos) {
                level.isStreamable = (trimmed.find("true") != std::string::npos);
            } else if (trimmed.find("\"streamingRadius\"") != std::string::npos) {
                std::string val = ExtractStringValue(trimmed, "streamingRadius");
                if (!val.empty()) level.streamingRadius = std::stof(val);
            } else if (trimmed.find("\"streamingOrigin\"") != std::string::npos) {
                size_t lb = trimmed.find('[');
                size_t rb = trimmed.find(']');
                if (lb != std::string::npos && rb != std::string::npos) {
                    std::string arr = trimmed.substr(lb + 1, rb - lb - 1);
                    std::istringstream as(arr);
                    char comma;
                    as >> level.streamingOrigin.X >> comma
                       >> level.streamingOrigin.Y >> comma
                       >> level.streamingOrigin.Z;
                }
            } else if (trimmed.find("\"entities\"") != std::string::npos) {
                state = IN_ENTITIES;
            }
            break;

        case IN_ENTITIES:
            if (trimmed == "]" || trimmed == "],") {
                state = TOP;
            } else if (trimmed.front() == '{') {
                currentEntity = SerializedEntity();
                currentEntity.id = 0;
                state = IN_ENTITY;
            }
            break;

        case IN_ENTITY:
            if (trimmed.find("\"id\"") != std::string::npos) {
                std::string val = ExtractStringValue(trimmed, "id");
                if (!val.empty()) currentEntity.id = std::stoull(val);
            } else if (trimmed.find("\"components\"") != std::string::npos) {
                state = IN_COMPONENTS;
            } else if (trimmed.front() == '}') {
                level.entities.push_back(currentEntity);
                state = IN_ENTITIES;
            }
            break;

        case IN_COMPONENTS:
            if (trimmed == "]" || trimmed == "],") {
                state = IN_ENTITY;
            } else if (trimmed.front() == '{') {
                componentAccum = trimmed;
                braceDepth = 0;
                for (char c : trimmed) {
                    if (c == '{') ++braceDepth;
                    else if (c == '}') --braceDepth;
                }
                if (braceDepth == 0) {
                    state = IN_COMPONENT; // will process immediately
                } else {
                    state = IN_COMPONENT;
                    break;
                }
                // Fall through to process
                {
                    // Remove trailing comma
                    std::string comp = TrimWS(componentAccum);
                    if (!comp.empty() && comp.back() == ',') comp.pop_back();
                    // Parse: {"type": "Name", "data": {...}}
                    std::string typeName = ExtractStringValue(comp, "type");
                    size_t dataPos = comp.find("\"data\":");
                    if (dataPos != std::string::npos) {
                        std::string dataStr = TrimWS(comp.substr(dataPos + 7));
                        // Remove trailing }
                        if (!dataStr.empty() && dataStr.back() == '}') dataStr.pop_back();
                        dataStr = TrimWS(dataStr);
                        currentEntity.componentTypes.push_back(typeName);
                        currentEntity.componentData.push_back(dataStr);
                    }
                    state = IN_COMPONENTS;
                }
            }
            break;

        case IN_COMPONENT:
            componentAccum += "\n" + trimmed;
            for (char c : trimmed) {
                if (c == '{') ++braceDepth;
                else if (c == '}') --braceDepth;
            }
            if (braceDepth <= 0) {
                std::string comp = TrimWS(componentAccum);
                if (!comp.empty() && comp.back() == ',') comp.pop_back();
                std::string typeName = ExtractStringValue(comp, "type");
                size_t dataPos = comp.find("\"data\":");
                if (dataPos != std::string::npos) {
                    std::string rest = comp.substr(dataPos + 7);
                    // Find the data object: first { to matching }
                    size_t dStart = rest.find('{');
                    if (dStart != std::string::npos) {
                        int d = 0;
                        size_t dEnd = dStart;
                        for (size_t k = dStart; k < rest.size(); ++k) {
                            if (rest[k] == '{') ++d;
                            else if (rest[k] == '}') { --d; if (d == 0) { dEnd = k; break; } }
                        }
                        std::string dataStr = rest.substr(dStart, dEnd - dStart + 1);
                        currentEntity.componentTypes.push_back(typeName);
                        currentEntity.componentData.push_back(dataStr);
                    }
                }
                state = IN_COMPONENTS;
            }
            break;
        }
    }

    return level;
}

SerializedLevel LevelSerializer::DeserializeFromJSON(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) return SerializedLevel();
    std::string content((std::istreambuf_iterator<char>(file)),
                         std::istreambuf_iterator<char>());
    file.close();
    return DeserializeFromJSONString(content);
}

// === Binary Serialization ===

bool LevelSerializer::SerializeToBinary(const SerializedLevel& level, const std::string& filePath) {
    std::ofstream file(filePath, std::ios::binary);
    if (!file.is_open()) return false;

    // Header magic
    const char magic[] = "KOIL";
    file.write(magic, 4);

    // Level name
    size_t nameLen = level.name.size();
    file.write(reinterpret_cast<const char*>(&nameLen), sizeof(nameLen));
    file.write(level.name.c_str(), nameLen);

    // Streaming
    file.write(reinterpret_cast<const char*>(&level.isStreamable), sizeof(level.isStreamable));
    file.write(reinterpret_cast<const char*>(&level.streamingOrigin), sizeof(level.streamingOrigin));
    file.write(reinterpret_cast<const char*>(&level.streamingRadius), sizeof(level.streamingRadius));

    // Entities
    size_t entityCount = level.entities.size();
    file.write(reinterpret_cast<const char*>(&entityCount), sizeof(entityCount));
    for (const auto& e : level.entities) {
        file.write(reinterpret_cast<const char*>(&e.id), sizeof(e.id));
        size_t compCount = e.componentTypes.size();
        file.write(reinterpret_cast<const char*>(&compCount), sizeof(compCount));
        for (size_t j = 0; j < compCount; ++j) {
            size_t typeLen = e.componentTypes[j].size();
            file.write(reinterpret_cast<const char*>(&typeLen), sizeof(typeLen));
            file.write(e.componentTypes[j].c_str(), typeLen);
            size_t dataLen = e.componentData[j].size();
            file.write(reinterpret_cast<const char*>(&dataLen), sizeof(dataLen));
            file.write(e.componentData[j].c_str(), dataLen);
        }
    }

    file.close();
    return true;
}

SerializedLevel LevelSerializer::DeserializeFromBinary(const std::string& filePath) {
    SerializedLevel level;
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) return level;

    // Verify magic
    char magic[4];
    file.read(magic, 4);
    if (std::string(magic, 4) != "KOIL") return level;

    // Level name
    size_t nameLen;
    file.read(reinterpret_cast<char*>(&nameLen), sizeof(nameLen));
    level.name.resize(nameLen);
    file.read(&level.name[0], nameLen);

    // Streaming
    file.read(reinterpret_cast<char*>(&level.isStreamable), sizeof(level.isStreamable));
    file.read(reinterpret_cast<char*>(&level.streamingOrigin), sizeof(level.streamingOrigin));
    file.read(reinterpret_cast<char*>(&level.streamingRadius), sizeof(level.streamingRadius));

    // Entities
    size_t entityCount;
    file.read(reinterpret_cast<char*>(&entityCount), sizeof(entityCount));
    for (size_t i = 0; i < entityCount; ++i) {
        SerializedEntity e;
        file.read(reinterpret_cast<char*>(&e.id), sizeof(e.id));
        size_t compCount;
        file.read(reinterpret_cast<char*>(&compCount), sizeof(compCount));
        for (size_t j = 0; j < compCount; ++j) {
            size_t typeLen;
            file.read(reinterpret_cast<char*>(&typeLen), sizeof(typeLen));
            std::string typeName(typeLen, '\0');
            file.read(&typeName[0], typeLen);
            e.componentTypes.push_back(typeName);

            size_t dataLen;
            file.read(reinterpret_cast<char*>(&dataLen), sizeof(dataLen));
            std::string data(dataLen, '\0');
            file.read(&data[0], dataLen);
            e.componentData.push_back(data);
        }
        level.entities.push_back(e);
    }

    file.close();
    return level;
}

} // namespace koilo
