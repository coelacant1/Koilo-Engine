// SPDX-License-Identifier: GPL-3.0-or-later
#include "koilo/assets/koilomesh_loader.hpp"
#include <fstream>
#include <cstring>
#include <cstdio>

namespace koilo {

KoiloMeshLoader::KoiloMeshLoader()
    : vertexCount_(0)
    , triangleCount_(0)
    , uvCount_(0)
    , morphCount_(0)
    , hasUVs_(false)
    , hasNormals_(false)
{
}

KoiloMeshLoader::~KoiloMeshLoader() {
}

bool KoiloMeshLoader::Load(const char* filepath) {
    // Auto-detect format by reading first bytes
    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        error_ = "Failed to open file: ";
        error_ += filepath;
        return false;
    }
    
    char magic[8];
    file.read(magic, 7);
    magic[7] = '\0';
    file.close();
    
    // Check format: ASCII (KMESH) first, then binary (KOIM), then old format (#)
    if (memcmp(magic, "KMESH", 5) == 0) {
        return LoadAscii(filepath);
    } else if (memcmp(magic, "KOIM", 4) == 0) {
        return LoadBinary(filepath);
    } else if (magic[0] == '#') {
        return LoadAscii(filepath);  // Old OBJ-like format
    } else {
        error_ = "Unknown file format (expected KMESH or KOIM)";
        return false;
    }
}

bool KoiloMeshLoader::LoadBinary(const char* filepath) {
    // Clear previous data
    vertices_.clear();
    triangles_.clear();
    morphs_.clear();
    error_.clear();
    vertexCount_ = 0;
    triangleCount_ = 0;
    morphCount_ = 0;
    hasUVs_ = false;
    hasNormals_ = false;
    
    // Read entire file
    std::ifstream file(filepath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        error_ = "Failed to open file: ";
        error_ += filepath;
        return false;
    }
    
    size_t fileSize = file.tellg();
    file.seekg(0, std::ios::beg);
    
    std::vector<uint8_t> buffer(fileSize);
    if (!file.read(reinterpret_cast<char*>(buffer.data()), fileSize)) {
        error_ = "Failed to read file";
        return false;
    }
    file.close();
    
    // Parse binary data
    size_t offset = 0;
    const uint8_t* data = buffer.data();
    
    // Read header
    if (!ReadHeader(data, fileSize, offset)) {
        return false;
    }
    
    // Read vertices
    if (!ReadVertices(data, fileSize, offset)) {
        return false;
    }
    
    // Read triangles
    if (!ReadTriangles(data, fileSize, offset)) {
        return false;
    }
    
    // Read morphs
    if (!ReadMorphs(data, fileSize, offset, morphCount_)) {
        return false;
    }
    
    // Validate end marker
    if (!ValidateEndMarker(data, fileSize, offset)) {
        return false;
    }
    
    return true;
}

bool KoiloMeshLoader::ReadHeader(const uint8_t* data, size_t size, size_t& offset) {
    if (size < 32) {
        error_ = "File too small for header";
        return false;
    }
    
    // Check magic
    if (memcmp(data + offset, "KOIM", 4) != 0) {
        error_ = "Invalid magic number (expected KOIM)";
        return false;
    }
    offset += 4;
    
    // Read version
    uint32_t version;
    memcpy(&version, data + offset, 4);
    offset += 4;
    
    if (version != 1) {
        error_ = "Unsupported version";
        return false;
    }
    
    // Read counts
    memcpy(&vertexCount_, data + offset, 4);
    offset += 4;
    
    memcpy(&triangleCount_, data + offset, 4);
    offset += 4;
    
    memcpy(&morphCount_, data + offset, 4);
    offset += 4;
    
    // Read flags
    uint32_t flags;
    memcpy(&flags, data + offset, 4);
    offset += 4;
    
    hasUVs_ = (flags & 0x01) != 0;
    hasNormals_ = (flags & 0x02) != 0;
    
    // Skip reserved bytes
    offset += 8;
    
    // Pre-allocate vectors
    vertices_.reserve(vertexCount_ * 3);
    triangles_.reserve(triangleCount_ * 3);
    morphs_.reserve(morphCount_);
    
    return true;
}

bool KoiloMeshLoader::ReadVertices(const uint8_t* data, size_t size, size_t& offset) {
    size_t vertexDataSize = vertexCount_ * 3 * sizeof(float);
    
    if (offset + vertexDataSize > size) {
        error_ = "File truncated (vertices)";
        return false;
    }
    
    vertices_.resize(vertexCount_ * 3);
    memcpy(vertices_.data(), data + offset, vertexDataSize);
    offset += vertexDataSize;
    
    return true;
}

bool KoiloMeshLoader::ReadTriangles(const uint8_t* data, size_t size, size_t& offset) {
    size_t triangleDataSize = triangleCount_ * 3 * sizeof(uint32_t);
    
    if (offset + triangleDataSize > size) {
        error_ = "File truncated (triangles)";
        return false;
    }
    
    triangles_.resize(triangleCount_ * 3);
    memcpy(triangles_.data(), data + offset, triangleDataSize);
    offset += triangleDataSize;
    
    return true;
}

bool KoiloMeshLoader::ReadMorphs(const uint8_t* data, size_t size, size_t& offset, uint32_t morphCount) {
    for (uint32_t i = 0; i < morphCount; ++i) {
        MorphTarget morph;
        
        // Read name length
        if (offset + 1 > size) {
            error_ = "File truncated (morph name length)";
            return false;
        }
        
        uint8_t nameLength = data[offset];
        offset += 1;
        
        // Read name
        if (offset + nameLength > size) {
            error_ = "File truncated (morph name)";
            return false;
        }
        
        morph.name = std::string(reinterpret_cast<const char*>(data + offset), nameLength);
        offset += nameLength;
        
        // Read affected vertex count
        if (offset + 4 > size) {
            error_ = "File truncated (morph affected count)";
            return false;
        }
        
        uint32_t affectedCount;
        memcpy(&affectedCount, data + offset, 4);
        offset += 4;
        
        // Reserve space
        morph.indices.reserve(affectedCount);
        morph.deltaX.reserve(affectedCount);
        morph.deltaY.reserve(affectedCount);
        morph.deltaZ.reserve(affectedCount);
        
        // Read delta data
        for (uint32_t j = 0; j < affectedCount; ++j) {
            if (offset + 16 > size) { // 4 bytes index + 12 bytes delta
                error_ = "File truncated (morph delta)";
                return false;
            }
            
            uint32_t index;
            memcpy(&index, data + offset, 4);
            offset += 4;
            
            float dx, dy, dz;
            memcpy(&dx, data + offset, 4);
            offset += 4;
            memcpy(&dy, data + offset, 4);
            offset += 4;
            memcpy(&dz, data + offset, 4);
            offset += 4;
            
            morph.indices.push_back(index);
            morph.deltaX.push_back(dx);
            morph.deltaY.push_back(dy);
            morph.deltaZ.push_back(dz);
        }
        
        morphs_.push_back(morph);
    }
    
    return true;
}

bool KoiloMeshLoader::ValidateEndMarker(const uint8_t* data, size_t size, size_t offset) {
    if (offset + 4 > size) {
        error_ = "File truncated (end marker)";
        return false;
    }
    
    if (memcmp(data + offset, "ENDM", 4) != 0) {
        error_ = "Invalid end marker (expected ENDM)";
        return false;
    }
    
    return true;
}

const MorphTarget* KoiloMeshLoader::GetMorph(uint32_t index) const {
    if (index >= morphs_.size()) {
        return nullptr;
    }
    return &morphs_[index];
}

const MorphTarget* KoiloMeshLoader::GetMorph(const char* name) const {
    for (const auto& morph : morphs_) {
        if (morph.name == name) {
            return &morph;
        }
    }
    return nullptr;
}

bool KoiloMeshLoader::LoadAscii(const char* filepath) {
    // Clear previous data
    vertices_.clear();
    triangles_.clear();
    uvs_.clear();
    uvTriangles_.clear();
    morphs_.clear();
    error_.clear();
    vertexCount_ = 0;
    triangleCount_ = 0;
    uvCount_ = 0;
    morphCount_ = 0;
    hasUVs_ = false;
    hasNormals_ = false;
    
    // Read entire file as text
    std::ifstream file(filepath);
    if (!file.is_open()) {
        error_ = "Failed to open file: ";
        error_ += filepath;
        return false;
    }
    
    std::string line;
    MorphTarget* currentMorph = nullptr;
    
    // Parser state
    enum class State {
        HEADER,
        VERTICES,
        TRIANGLES,
        UVS,
        UVTRIANGLES,
        MORPH_HEADER,
        MORPH_INDICES,
        MORPH_VECTORS
    };
    State state = State::HEADER;
    
    int lineNum = 0;
    while (std::getline(file, line)) {
        lineNum++;
        
        // Trim whitespace
        size_t start = line.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) continue;  // Empty line
        line = line.substr(start);
        size_t end = line.find_last_not_of(" \t\r\n");
        if (end != std::string::npos) {
            line = line.substr(0, end + 1);
        }
        
        // Skip comments
        if (line[0] == '#') continue;
        
        // Parse sections
        if (line.find("KMESH") == 0) {
            // Header line, version check done
            state = State::HEADER;
        }
        else if (line.find("NAME") == 0) {
            // Optional, skip for now
        }
        else if (line.find("VERTICES") == 0) {
            // VERTICES <count>
            if (sscanf(line.c_str(), "VERTICES %u", &vertexCount_) != 1) {
                error_ = "Failed to parse VERTICES count";
                return false;
            }
            vertices_.reserve(vertexCount_ * 3);
            state = State::VERTICES;
        }
        else if (line.find("TRIANGLES") == 0) {
            // TRIANGLES <count>
            if (sscanf(line.c_str(), "TRIANGLES %u", &triangleCount_) != 1) {
                error_ = "Failed to parse TRIANGLES count";
                return false;
            }
            triangles_.reserve(triangleCount_ * 3);
            state = State::TRIANGLES;
        }
        else if (line.find("UVTRIANGLES") == 0) {
            uint32_t uvTriCount;
            if (sscanf(line.c_str(), "UVTRIANGLES %u", &uvTriCount) != 1) {
                error_ = "Failed to parse UVTRIANGLES count";
                return false;
            }
            uvTriangles_.reserve(uvTriCount * 3);
            state = State::UVTRIANGLES;
        }
        else if (line.find("UVS") == 0) {
            if (sscanf(line.c_str(), "UVS %u", &uvCount_) != 1) {
                error_ = "Failed to parse UVS count";
                return false;
            }
            uvs_.reserve(uvCount_ * 2);
            state = State::UVS;
        }
        else if (line.find("MORPH") == 0) {
            // MORPH <name> <count>
            char name[64];
            uint32_t count;
            if (sscanf(line.c_str(), "MORPH %63s %u", name, &count) == 2) {
                morphs_.push_back(MorphTarget());
                currentMorph = &morphs_.back();
                currentMorph->name = name;
                currentMorph->indices.reserve(count);
                currentMorph->deltaX.reserve(count);
                currentMorph->deltaY.reserve(count);
                currentMorph->deltaZ.reserve(count);
                state = State::MORPH_HEADER;
            }
        }
        else if (line.find("INDICES") == 0) {
            // INDICES <space-separated list>
            if (!currentMorph) {
                error_ = "INDICES without MORPH";
                return false;
            }
            // Parse all indices on the line
            const char* ptr = line.c_str() + 7;  // Skip "INDICES"
            uint32_t idx;
            while (sscanf(ptr, "%u", &idx) == 1) {
                currentMorph->indices.push_back(idx);
                // Move to next number
                while (*ptr == ' ' || *ptr == '\t') ptr++;
                while (*ptr >= '0' && *ptr <= '9') ptr++;
            }
            state = State::MORPH_INDICES;
        }
        else if (line.find("VECTORS") == 0) {
            state = State::MORPH_VECTORS;
        }
        else {
            // Parse data based on state
            if (state == State::VERTICES) {
                float x, y, z;
                if (sscanf(line.c_str(), "%f %f %f", &x, &y, &z) == 3) {
                    vertices_.push_back(x);
                    vertices_.push_back(y);
                    vertices_.push_back(z);
                }
            }
            else if (state == State::TRIANGLES) {
                uint32_t a, b, c;
                if (sscanf(line.c_str(), "%u %u %u", &a, &b, &c) == 3) {
                    triangles_.push_back(a);
                    triangles_.push_back(b);
                    triangles_.push_back(c);
                }
            }
            else if (state == State::UVS) {
                float u, v;
                if (sscanf(line.c_str(), "%f %f", &u, &v) == 2) {
                    uvs_.push_back(u);
                    uvs_.push_back(v);
                }
            }
            else if (state == State::UVTRIANGLES) {
                uint32_t a, b, c;
                if (sscanf(line.c_str(), "%u %u %u", &a, &b, &c) == 3) {
                    uvTriangles_.push_back(a);
                    uvTriangles_.push_back(b);
                    uvTriangles_.push_back(c);
                }
            }
            else if (state == State::MORPH_VECTORS && currentMorph) {
                float dx, dy, dz;
                if (sscanf(line.c_str(), "%f %f %f", &dx, &dy, &dz) == 3) {
                    currentMorph->deltaX.push_back(dx);
                    currentMorph->deltaY.push_back(dy);
                    currentMorph->deltaZ.push_back(dz);
                }
            }
        }
    }
    
    file.close();
    
    morphCount_ = static_cast<uint32_t>(morphs_.size());
    
    // Validate counts
    if (vertices_.size() / 3 != vertexCount_) {
        error_ = "Vertex count mismatch";
        return false;
    }
    if (triangles_.size() / 3 != triangleCount_) {
        error_ = "Triangle count mismatch";
        return false;
    }
    
    // UV validation
    if (!uvs_.empty() && !uvTriangles_.empty()) {
        hasUVs_ = true;
    }
    
    return true;
}

} // namespace koilo
