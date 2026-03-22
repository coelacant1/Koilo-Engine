// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file texture_cache.hpp
 * @brief Shared GPU texture cache using the RHI abstraction layer.
 *
 * Converts engine Texture assets to RHI texture handles on first use and
 * caches them so that both the OpenGL and Vulkan backends avoid duplicating
 * the Texture -> GPU-texture upload logic.
 *
 * @date 03/18/2026
 * @author Coela Can't
 */
#pragma once

#include "../rhi/rhi_types.hpp"
#include <unordered_map>
#include <cstdint>
#include "../../../registry/reflect_macros.hpp"

namespace koilo     { class Texture; }
namespace koilo::rhi { class IRHIDevice; }

namespace koilo::rhi {

// --- TextureCache -------------------------------------------------------

class TextureCache {
public:
    explicit TextureCache(IRHIDevice* device);
    ~TextureCache();

    TextureCache(const TextureCache&)            = delete;
    TextureCache& operator=(const TextureCache&) = delete;

    /**
     * @brief  Return the RHI texture for a Texture asset, uploading if needed.
     * @param  texture  Source engine texture (may be nullptr).
     * @return RHI handle, or null handle {0} when the texture is null or empty.
     */
    RHITexture GetOrUpload(const Texture* texture);

    /**
     * @brief Remove a single texture from the cache and destroy its GPU resource.
     * @param texture  The engine texture to invalidate.
     */
    void Invalidate(const Texture* texture);

    /**
     * @brief Destroy every cached GPU texture and clear the cache.
     */
    void Clear();

    /**
     * @brief Number of textures currently held in the cache.
     */
    size_t Size() const;

private:
    IRHIDevice*                          device_;
    std::unordered_map<uintptr_t, RHITexture> cache_;

    KL_BEGIN_FIELDS(TextureCache)
        /* No reflected fields. */
    KL_END_FIELDS

    KL_BEGIN_METHODS(TextureCache)
        /* No reflected methods. */
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(TextureCache)
        KL_CTOR(TextureCache, IRHIDevice*)
    KL_END_DESCRIBE(TextureCache)

};

} // namespace koilo::rhi
