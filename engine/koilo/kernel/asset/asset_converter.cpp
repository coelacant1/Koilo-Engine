// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file asset_converter.cpp
 * @brief ConverterRegistry implementation.
 */
#include <koilo/kernel/asset/asset_converter.hpp>
#include <algorithm>

namespace koilo {

void ConverterRegistry::Register(std::unique_ptr<IAssetConverter> converter) {
    if (!converter) return;
    std::string ext = converter->GetSourceExtension();
    // Normalize to lowercase.
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    size_t idx = converters_.size();
    extensionIndex_[ext] = idx;
    converters_.push_back(std::move(converter));
}

IAssetConverter* ConverterRegistry::FindByExtension(const std::string& extension) const {
    std::string ext = extension;
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    auto it = extensionIndex_.find(ext);
    if (it == extensionIndex_.end()) return nullptr;
    return converters_[it->second].get();
}

IAssetConverter* ConverterRegistry::FindForFile(const std::string& path) const {
    for (const auto& conv : converters_) {
        if (conv->CanConvert(path)) return conv.get();
    }
    return nullptr;
}

std::vector<std::string> ConverterRegistry::GetSupportedExtensions() const {
    std::vector<std::string> result;
    result.reserve(extensionIndex_.size());
    for (const auto& [ext, idx] : extensionIndex_) {
        result.push_back(ext);
    }
    return result;
}

} // namespace koilo
