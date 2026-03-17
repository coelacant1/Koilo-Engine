// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file asset_converter.hpp
 * @brief Extensible format conversion interface for the asset pipeline.
 *
 * IAssetConverter defines a source->target conversion (e.g. .bmp->.ktex).
 * ConverterRegistry holds all registered converters and provides lookup
 * by source extension.
 *
 * @date 01/19/2026
 * @author Coela
 */
#pragma once

#include <koilo/kernel/asset/asset_handle.hpp>
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

namespace koilo {

/**
 * @class IAssetConverter
 * @brief Interface for converting assets from external to engine formats.
 */
class IAssetConverter {
public:
    virtual ~IAssetConverter() = default;

    /// Human-readable converter name (e.g. "BMP to KTEX").
    virtual const char* GetName() const = 0;

    /// Source file extension including dot (e.g. ".bmp").
    virtual const char* GetSourceExtension() const = 0;

    /// Target file extension including dot (e.g. ".ktex").
    virtual const char* GetTargetExtension() const = 0;

    /// What AssetType this converter produces.
    virtual AssetType GetAssetType() const = 0;

    /// Check if this converter can handle the given file path.
    virtual bool CanConvert(const std::string& srcPath) const = 0;

    /// Convert source file to target file. Returns true on success.
    virtual bool Convert(const std::string& srcPath,
                         const std::string& dstPath) = 0;

    /// Get the last error message (empty if no error).
    virtual const std::string& GetLastError() const = 0;
};

/**
 * @class ConverterRegistry
 * @brief Stores registered IAssetConverters and provides lookup by extension.
 */
class ConverterRegistry {
public:
    ConverterRegistry() = default;
    ~ConverterRegistry() = default;

    /// Register a converter. Ownership is transferred.
    void Register(std::unique_ptr<IAssetConverter> converter);

    /// Find a converter for the given source extension (e.g. ".bmp").
    /// Returns nullptr if none registered.
    IAssetConverter* FindByExtension(const std::string& extension) const;

    /// Find a converter that can handle the given file path.
    /// Tries CanConvert() on each registered converter.
    IAssetConverter* FindForFile(const std::string& path) const;

    /// Get all registered source extensions.
    std::vector<std::string> GetSupportedExtensions() const;

    /// Number of registered converters.
    size_t Count() const { return converters_.size(); }

private:
    std::vector<std::unique_ptr<IAssetConverter>> converters_;

    /// Extension -> converter index for O(1) lookup.
    std::unordered_map<std::string, size_t> extensionIndex_;
};

} // namespace koilo
