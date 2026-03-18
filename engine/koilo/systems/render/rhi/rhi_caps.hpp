// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file rhi_caps.hpp
 * @brief RHI capability queries - feature flags and hardware limits.
 *
 * Backends populate RHILimits at device creation time.  Feature support
 * is queried per-flag so higher-level code can gracefully degrade.
 *
 * @date 03/18/2026
 * @author Coela Can't
 */
#pragma once
#include <cstdint>

namespace koilo::rhi {

/// Optional GPU features that may or may not be available.
enum class RHIFeature : uint16_t {
    TimestampQueries   = 1 << 0,
    ComputeShaders     = 1 << 1,
    MultiDrawIndirect  = 1 << 2,
    BindlessTextures   = 1 << 3,
    PushConstants      = 1 << 4,
    StorageBuffers     = 1 << 5,
    GeometryShaders    = 1 << 6,
    TessellationShaders = 1 << 7,
    AsyncCompute       = 1 << 8,
    DepthClamp         = 1 << 9,
};

inline RHIFeature operator|(RHIFeature a, RHIFeature b) {
    return static_cast<RHIFeature>(
        static_cast<uint16_t>(a) | static_cast<uint16_t>(b));
}
inline bool HasFeature(RHIFeature caps, RHIFeature flag) {
    return (static_cast<uint16_t>(caps) & static_cast<uint16_t>(flag)) != 0;
}

/// Hardware limits reported by the backend at device creation.
struct RHILimits {
    uint32_t maxTextureSize          = 4096;
    uint32_t maxTexture3DSize        = 256;
    uint32_t maxCubeMapSize          = 4096;
    uint32_t maxFramebufferWidth     = 4096;
    uint32_t maxFramebufferHeight    = 4096;
    uint32_t maxColorAttachments     = 4;
    uint32_t maxVertexAttributes     = 16;
    uint32_t maxVertexBindings       = 16;
    uint32_t maxUniformBufferSize    = 65536;
    uint64_t maxStorageBufferSize    = 128ULL * 1024 * 1024;
    uint32_t maxPushConstantSize     = 128;
    uint32_t maxBoundDescriptorSets  = 4;
    uint32_t maxSamplers             = 16;
    uint32_t maxDrawIndirectCount    = 1;
    float    maxAnisotropy           = 1.0f;
    uint64_t totalDeviceMemory       = 0;   ///< VRAM in bytes (0 = unknown)
};

} // namespace koilo::rhi
