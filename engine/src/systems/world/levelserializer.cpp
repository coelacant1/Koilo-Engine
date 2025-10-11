#include <ptx/systems/world/levelserializer.hpp>
#include <ptx/systems/ecs/entitymanager.hpp>
#include <fstream>
#include <iostream>

namespace ptx {

LevelSerializer::LevelSerializer(EntityManager* entityManager, SerializationFormat format)
    : entityManager(entityManager), format(format) {
}

LevelSerializer::~LevelSerializer() {
}

// === Serialization ===

bool LevelSerializer::SerializeLevelToFile(std::shared_ptr<Level> level, const std::string& filePath) {
    if (!level) {
        std::cerr << "Cannot serialize null level" << std::endl;
        return false;
    }

    // Build serialized level
    SerializedLevel serializedLevel;
    serializedLevel.name = level->GetName();
    serializedLevel.isStreamable = level->IsStreamable();
    serializedLevel.streamingOrigin = level->GetStreamingOrigin();
    serializedLevel.streamingRadius = level->GetStreamingRadius();

    // Serialize entities
    for (const Entity& entity : level->GetEntities()) {
        SerializedEntity serializedEntity = SerializeEntity(entity);
        serializedLevel.entities.push_back(serializedEntity);
    }

    // Save to file based on format
    switch (format) {
        case SerializationFormat::JSON:
            return SerializeToJSON(serializedLevel, filePath);
        case SerializationFormat::Binary:
            return SerializeToBinary(serializedLevel, filePath);
        case SerializationFormat::XML:
            return SerializeToXML(serializedLevel, filePath);
        default:
            std::cerr << "Unknown serialization format" << std::endl;
            return false;
    }
}

std::shared_ptr<Level> LevelSerializer::DeserializeLevelFromFile(const std::string& filePath) {
    SerializedLevel serializedLevel;

    // Load from file based on format
    switch (format) {
        case SerializationFormat::JSON:
            serializedLevel = DeserializeFromJSON(filePath);
            break;
        case SerializationFormat::Binary:
            serializedLevel = DeserializeFromBinary(filePath);
            break;
        case SerializationFormat::XML:
            serializedLevel = DeserializeFromXML(filePath);
            break;
        default:
            std::cerr << "Unknown serialization format" << std::endl;
            return nullptr;
    }

    // Create level
    auto level = std::make_shared<Level>(serializedLevel.name);
    level->SetStreamable(serializedLevel.isStreamable);
    level->SetStreamingBounds(serializedLevel.streamingOrigin, serializedLevel.streamingRadius);
    level->SetFilePath(filePath);
    level->SetEntityManager(entityManager);

    // Deserialize entities
    if (entityManager) {
        for (const SerializedEntity& serializedEntity : serializedLevel.entities) {
            Entity entity = DeserializeEntity(serializedEntity);
            level->AddEntity(entity);
        }
    }

    return level;
}

std::string LevelSerializer::SerializeLevelToString(std::shared_ptr<Level> level) {
    // TODO: Implement string serialization
    return "";
}

std::shared_ptr<Level> LevelSerializer::DeserializeLevelFromString(const std::string& data) {
    // TODO: Implement string deserialization
    return nullptr;
}

// === Entity Serialization ===

SerializedEntity LevelSerializer::SerializeEntity(Entity entity) {
    SerializedEntity serialized;
    serialized.id = entity.GetID();

    // TODO: Use reflection to serialize all components
    // For now, return empty entity
    return serialized;
}

Entity LevelSerializer::DeserializeEntity(const SerializedEntity& serializedEntity) {
    if (!entityManager) {
        return Entity(0);
    }

    Entity entity = entityManager->CreateEntity();

    // TODO: Use reflection to deserialize all components
    // For now, return empty entity
    return entity;
}

// === Format-Specific Helpers ===

bool LevelSerializer::SerializeToJSON(const SerializedLevel& level, const std::string& filePath) {
    std::ofstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "Failed to open file for writing: " << filePath << std::endl;
        return false;
    }

    // Write JSON (simplified format)
    file << "{\n";
    file << "  \"name\": \"" << level.name << "\",\n";
    file << "  \"isStreamable\": " << (level.isStreamable ? "true" : "false") << ",\n";
    file << "  \"streamingOrigin\": [" << level.streamingOrigin.x << ", "
         << level.streamingOrigin.y << ", " << level.streamingOrigin.z << "],\n";
    file << "  \"streamingRadius\": " << level.streamingRadius << ",\n";
    file << "  \"entities\": [\n";

    for (size_t i = 0; i < level.entities.size(); ++i) {
        const SerializedEntity& entity = level.entities[i];
        file << "    {\n";
        file << "      \"id\": " << entity.id << ",\n";
        file << "      \"components\": []\n";
        file << "    }";
        if (i < level.entities.size() - 1) {
            file << ",";
        }
        file << "\n";
    }

    file << "  ]\n";
    file << "}\n";

    file.close();
    return true;
}

bool LevelSerializer::SerializeToBinary(const SerializedLevel& level, const std::string& filePath) {
    std::ofstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Failed to open file for writing: " << filePath << std::endl;
        return false;
    }

    // Write binary format (simplified)
    size_t nameLength = level.name.size();
    file.write(reinterpret_cast<const char*>(&nameLength), sizeof(nameLength));
    file.write(level.name.c_str(), nameLength);

    file.write(reinterpret_cast<const char*>(&level.isStreamable), sizeof(level.isStreamable));
    file.write(reinterpret_cast<const char*>(&level.streamingOrigin), sizeof(level.streamingOrigin));
    file.write(reinterpret_cast<const char*>(&level.streamingRadius), sizeof(level.streamingRadius));

    size_t entityCount = level.entities.size();
    file.write(reinterpret_cast<const char*>(&entityCount), sizeof(entityCount));

    // TODO: Write entity data

    file.close();
    return true;
}

bool LevelSerializer::SerializeToXML(const SerializedLevel& level, const std::string& filePath) {
    std::ofstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "Failed to open file for writing: " << filePath << std::endl;
        return false;
    }

    // Write XML (simplified format)
    file << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    file << "<Level>\n";
    file << "  <Name>" << level.name << "</Name>\n";
    file << "  <Streamable>" << (level.isStreamable ? "true" : "false") << "</Streamable>\n";
    file << "  <StreamingOrigin x=\"" << level.streamingOrigin.x
         << "\" y=\"" << level.streamingOrigin.y
         << "\" z=\"" << level.streamingOrigin.z << "\"/>\n";
    file << "  <StreamingRadius>" << level.streamingRadius << "</StreamingRadius>\n";
    file << "  <Entities>\n";

    for (const SerializedEntity& entity : level.entities) {
        file << "    <Entity id=\"" << entity.id << "\">\n";
        file << "    </Entity>\n";
    }

    file << "  </Entities>\n";
    file << "</Level>\n";

    file.close();
    return true;
}

SerializedLevel LevelSerializer::DeserializeFromJSON(const std::string& filePath) {
    SerializedLevel level;

    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "Failed to open file for reading: " << filePath << std::endl;
        return level;
    }

    // TODO: Implement proper JSON parsing
    // For now, return empty level
    file.close();
    return level;
}

SerializedLevel LevelSerializer::DeserializeFromBinary(const std::string& filePath) {
    SerializedLevel level;

    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Failed to open file for reading: " << filePath << std::endl;
        return level;
    }

    // Read binary format (simplified)
    size_t nameLength;
    file.read(reinterpret_cast<char*>(&nameLength), sizeof(nameLength));

    level.name.resize(nameLength);
    file.read(&level.name[0], nameLength);

    file.read(reinterpret_cast<char*>(&level.isStreamable), sizeof(level.isStreamable));
    file.read(reinterpret_cast<char*>(&level.streamingOrigin), sizeof(level.streamingOrigin));
    file.read(reinterpret_cast<char*>(&level.streamingRadius), sizeof(level.streamingRadius));

    size_t entityCount;
    file.read(reinterpret_cast<char*>(&entityCount), sizeof(entityCount));

    // TODO: Read entity data

    file.close();
    return level;
}

SerializedLevel LevelSerializer::DeserializeFromXML(const std::string& filePath) {
    SerializedLevel level;

    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "Failed to open file for reading: " << filePath << std::endl;
        return level;
    }

    // TODO: Implement proper XML parsing
    // For now, return empty level
    file.close();
    return level;
}

} // namespace ptx
