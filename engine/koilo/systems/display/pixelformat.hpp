// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file pixelformat.hpp
 * @brief Pixel format definitions for display systems.
 *
 * @date 11/10/2025
 * @author Coela
 */

#pragma once

#include <cstdint>
#include <koilo/registry/reflect_macros.hpp>

namespace koilo {

/**
 * @enum PixelFormat
 * @brief Supported pixel formats for framebuffers.
 */
enum class PixelFormat : uint8_t {
    RGB888,      ///< 24-bit RGB (8 bits per channel)
    RGB565,      ///< 16-bit RGB (5-6-5 bits)
    RGBA8888,    ///< 32-bit RGBA (8 bits per channel)
    Grayscale8,  ///< 8-bit grayscale
    Monochrome,  ///< 1-bit monochrome
    BGR888,      ///< 24-bit BGR (reversed RGB)
    BGRA8888,    ///< 32-bit BGRA (reversed RGBA)
};

/**
 * @brief Get bytes per pixel for a given format.
 * @param format The pixel format.
 * @return Number of bytes per pixel.
 */
inline uint32_t GetBytesPerPixel(PixelFormat format) {
    switch (format) {
        case PixelFormat::RGB888:      return 3;
        case PixelFormat::RGB565:      return 2;
        case PixelFormat::RGBA8888:    return 4;
        case PixelFormat::Grayscale8:  return 1;
        case PixelFormat::Monochrome:  return 1;  // Note: 8 pixels per byte, but return 1 for calculations
        case PixelFormat::BGR888:      return 3;
        case PixelFormat::BGRA8888:    return 4;
        default:                       return 3;
    }
}

/**
 * @brief Get bits per pixel for a given format.
 * @param format The pixel format.
 * @return Number of bits per pixel.
 */
inline uint32_t GetBitsPerPixel(PixelFormat format) {
    switch (format) {
        case PixelFormat::RGB888:      return 24;
        case PixelFormat::RGB565:      return 16;
        case PixelFormat::RGBA8888:    return 32;
        case PixelFormat::Grayscale8:  return 8;
        case PixelFormat::Monochrome:  return 1;
        case PixelFormat::BGR888:      return 24;
        case PixelFormat::BGRA8888:    return 32;
        default:                       return 24;
    }
}

/**
 * @brief Get string name for pixel format.
 */
inline const char* GetPixelFormatName(PixelFormat format) {
    switch (format) {
        case PixelFormat::RGB888:      return "RGB888";
        case PixelFormat::RGB565:      return "RGB565";
        case PixelFormat::RGBA8888:    return "RGBA8888";
        case PixelFormat::Grayscale8:  return "Grayscale8";
        case PixelFormat::Monochrome:  return "Monochrome";
        case PixelFormat::BGR888:      return "BGR888";
        case PixelFormat::BGRA8888:    return "BGRA8888";
        default:                       return "Unknown";
    }
}

} // namespace koilo
