// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file schema_version.hpp
 * @brief Versioned binary/text format header helpers.
 *
 * Provides common utilities for reading and writing magic + version headers
 * in binary and text formats. All serialized engine data should embed a
 * schema version so older files can be detected and migrated.
 *
 * Binary format convention:
 *   [magic: 4 bytes] [version: uint32_t] [payload ...]
 *
 * Text format convention:
 *   # koilo <format_name> v<N>
 *   <payload lines ...>
 *
 * @date 03/30/2026
 * @author Coela
 */
#pragma once

#include <cstdint>
#include <cstring>
#include <fstream>
#include <string>

namespace koilo {

/// Result of reading a versioned header from a file.
struct SchemaHeader {
    char     magic[4] = {};
    uint32_t version  = 0;
    bool     valid    = false;  ///< True if magic matched.
};

/// Write a binary schema header (magic + version).
inline bool WriteSchemaHeader(std::ofstream& f, const char magic[4],
                              uint32_t version) {
    f.write(magic, 4);
    f.write(reinterpret_cast<const char*>(&version), sizeof(version));
    return f.good();
}

/// Read a binary schema header. Sets valid=true if magic matches expected.
inline SchemaHeader ReadSchemaHeader(std::ifstream& f,
                                     const char expectedMagic[4]) {
    SchemaHeader hdr{};
    f.read(hdr.magic, 4);
    f.read(reinterpret_cast<char*>(&hdr.version), sizeof(hdr.version));
    hdr.valid = f.good() && std::memcmp(hdr.magic, expectedMagic, 4) == 0;
    return hdr;
}

/// Parse a text-format version header line.
/// Expected format: "# koilo <format_name> v<N>"
/// Returns 0 if line doesn't match or is absent.
inline uint32_t ParseTextSchemaVersion(const std::string& line,
                                       const char* formatName) {
    // Expected: "# koilo <formatName> v<N>"
    std::string prefix = std::string("# koilo ") + formatName + " v";
    if (line.size() > prefix.size() &&
        line.compare(0, prefix.size(), prefix) == 0) {
        try {
            return static_cast<uint32_t>(std::stoul(line.substr(prefix.size())));
        } catch (...) {}
    }
    return 0;
}

/// Build a text-format version header line.
inline std::string MakeTextSchemaHeader(const char* formatName,
                                        uint32_t version) {
    return std::string("# koilo ") + formatName + " v" + std::to_string(version);
}

} // namespace koilo
