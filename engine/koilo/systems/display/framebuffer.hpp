// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file framebuffer.hpp
 * @brief Framebuffer structure for display systems.
 *
 * @date 11/10/2025
 * @author Coela
 */

#pragma once

#include <cstdint>
#include <cstring>
#include "pixelformat.hpp"
#include <koilo/registry/reflect_macros.hpp>

namespace koilo {

/**
 * @struct Framebuffer
 * @brief Represents a framebuffer with pixel data.
 *
 * A framebuffer contains the raw pixel data and metadata about its format,
 * dimensions, and memory layout.
 */
struct Framebuffer {
    void* data;              ///< Pointer to pixel data (not owned)
    uint32_t width;          ///< Width in pixels
    uint32_t height;         ///< Height in pixels
    PixelFormat format;      ///< Pixel format
    uint32_t stride;         ///< Bytes per row (may include padding)
    
    /**
     * @brief Default constructor.
     */
    Framebuffer()
        : data(nullptr), width(0), height(0), 
          format(PixelFormat::RGB888), stride(0) {}
    
    /**
     * @brief Constructor with parameters.
     */
    Framebuffer(void* data, uint32_t width, uint32_t height, 
                PixelFormat format, uint32_t stride = 0)
        : data(data), width(width), height(height), format(format),
          stride(stride == 0 ? width * GetBytesPerPixel(format) : stride) {}
    
    /**
     * @brief Get total size in bytes.
     */
    uint32_t GetSizeBytes() const {
        return stride * height;
    }
    
    /**
     * @brief Check if framebuffer is valid.
     */
    bool IsValid() const {
        return data != nullptr && width > 0 && height > 0;
    }
    
    /**
     * @brief Clear framebuffer to black.
     */
    void Clear() {
        if (data) {
            std::memset(data, 0, GetSizeBytes());
        }
    }
    
    /**
     * @brief Get pointer to pixel at (x, y).
     */
    void* GetPixelPtr(uint32_t x, uint32_t y) {
        if (!data || x >= width || y >= height) return nullptr;
        return static_cast<uint8_t*>(data) + (y * stride + x * GetBytesPerPixel(format));
    }
    
    /**
     * @brief Get const pointer to pixel at (x, y).
     */
    const void* GetPixelPtr(uint32_t x, uint32_t y) const {
        if (!data || x >= width || y >= height) return nullptr;
        return static_cast<const uint8_t*>(data) + (y * stride + x * GetBytesPerPixel(format));
    }

    KL_BEGIN_FIELDS(Framebuffer)
        KL_FIELD(Framebuffer, width, "Width", 0, 65535),
        KL_FIELD(Framebuffer, height, "Height", 0, 65535),
        KL_FIELD(Framebuffer, stride, "Stride", 0, 4294967295)
    KL_END_FIELDS

    KL_BEGIN_METHODS(Framebuffer)
        KL_METHOD_AUTO(Framebuffer, GetSizeBytes, "Get size bytes"),
        KL_METHOD_AUTO(Framebuffer, IsValid, "Is valid"),
        KL_METHOD_AUTO(Framebuffer, Clear, "Clear")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(Framebuffer)
        KL_CTOR0(Framebuffer),
        KL_CTOR(Framebuffer, void*, uint32_t, uint32_t, PixelFormat, uint32_t)
    KL_END_DESCRIBE(Framebuffer)
};

} // namespace koilo
