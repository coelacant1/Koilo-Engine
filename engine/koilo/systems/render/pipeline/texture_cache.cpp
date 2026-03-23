// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file texture_cache.cpp
 * @brief Shared GPU texture cache – implementation.
 *
 * Converts Palette and RGB888 textures to RGBA8 and uploads them through
 * the RHI device interface.
 *
 * @date 03/18/2026
 * @author Coela Can't
 */

#include "texture_cache.hpp"
#include "../rhi/rhi_device.hpp"
#include <koilo/assets/image/texture.hpp>
#include <koilo/core/color/color888.hpp>
#include <vector>

namespace koilo::rhi {

// --- Lifetime -----------------------------------------------------------

TextureCache::TextureCache(IRHIDevice* device)
    : device_(device) {}

TextureCache::~TextureCache() { Clear(); }

// --- Query / upload -----------------------------------------------------

RHITexture TextureCache::GetOrUpload(const Texture* texture)
{
    if (!texture)
        return RHITexture{0};

    const uintptr_t key = reinterpret_cast<uintptr_t>(texture);

    // Fast path – already cached.
    auto it = cache_.find(key);
    if (it != cache_.end())
        return it->second;

    const uint32_t w = texture->GetWidth();
    const uint32_t h = texture->GetHeight();
    if (w == 0 || h == 0)
        return RHITexture{0};

    // Convert to RGBA8 (both backends normalise to 4-channel).
    std::vector<uint8_t> rgba(w * h * 4);

    if (texture->GetFormat() == Texture::Format::RGB888) {
        const Color888* pixels = texture->GetPixels();
        for (uint32_t i = 0; i < w * h; ++i) {
            rgba[i * 4 + 0] = pixels[i].r;
            rgba[i * 4 + 1] = pixels[i].g;
            rgba[i * 4 + 2] = pixels[i].b;
            rgba[i * 4 + 3] = 255;
        }
    } else { // Palette mode
        const uint8_t* indices = texture->GetIndices();
        const uint8_t* palette = texture->GetPalette();
        for (uint32_t i = 0; i < w * h; ++i) {
            const uint8_t idx = indices[i];
            rgba[i * 4 + 0] = palette[idx * 3 + 0];
            rgba[i * 4 + 1] = palette[idx * 3 + 1];
            rgba[i * 4 + 2] = palette[idx * 3 + 2];
            rgba[i * 4 + 3] = 255;
        }
    }

    // Upload through the RHI device.
    RHITextureDesc desc{};
    desc.width  = w;
    desc.height = h;
    desc.format = RHIFormat::RGBA8_Unorm;
    desc.usage  = RHITextureUsage::Sampled | RHITextureUsage::TransferDst;

    RHITexture tex = device_->CreateTexture(desc);
    device_->UpdateTexture(tex, rgba.data(), rgba.size(), w, h);

    cache_[key] = tex;
    return tex;
}

// --- Invalidation -------------------------------------------------------

void TextureCache::Invalidate(const Texture* texture)
{
    if (!texture)
        return;

    const uintptr_t key = reinterpret_cast<uintptr_t>(texture);
    auto it = cache_.find(key);
    if (it != cache_.end()) {
        device_->DestroyTexture(it->second);
        cache_.erase(it);
    }
}

void TextureCache::Clear()
{
    for (auto& [key, tex] : cache_)
        device_->DestroyTexture(tex);
    cache_.clear();
}

// --- Accessors ----------------------------------------------------------

size_t TextureCache::Size() const { return cache_.size(); }

} // namespace koilo::rhi
