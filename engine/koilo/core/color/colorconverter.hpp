// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file colorconverter.hpp
 * @brief Efficient color format conversion utilities for display output.
 *
 * Provides optimized conversion between internal color formats and display
 * framebuffer formats, minimizing overhead in the rendering pipeline.
 *
 * @date 11/10/2025
 * @version 1.0
 * @author Coela
 */

#pragma once

#include "color.hpp"
#include "color888.hpp"
#include "color565.hpp"
#include "colorgray.hpp"
#include "colormono.hpp"
#include <koilo/systems/display/pixelformat.hpp>
#include <cstdint>
#include <koilo/registry/reflect_macros.hpp>

namespace koilo {

/**
 * @class ColorConverter
 * @brief Utility class for efficient color format conversions.
 *
 * Provides batch conversion and optimized paths for common format conversions
 * in the rendering pipeline.
 */
class ColorConverter {
public:
    /**
     * @brief Write a color to a framebuffer pixel location.
     * @param dest Pointer to destination pixel in framebuffer
     * @param color Source color (any IColor implementation)
     * @param format Target pixel format
     */
    static void WritePixel(void* dest, const IColor& color, PixelFormat format) {
        if (!dest) return;

        switch (format) {
            case PixelFormat::RGB888: {
                uint8_t* rgb = static_cast<uint8_t*>(dest);
                color.ToRGB888(rgb[0], rgb[1], rgb[2]);
                break;
            }
            case PixelFormat::BGR888: {
                uint8_t* bgr = static_cast<uint8_t*>(dest);
                uint8_t r, g, b;
                color.ToRGB888(r, g, b);
                bgr[0] = b; bgr[1] = g; bgr[2] = r;
                break;
            }
            case PixelFormat::RGB565: {
                *static_cast<uint16_t*>(dest) = color.ToRGB565();
                break;
            }
            case PixelFormat::RGBA8888: {
                uint8_t* rgba = static_cast<uint8_t*>(dest);
                color.ToRGBA8888(rgba[0], rgba[1], rgba[2], rgba[3]);
                break;
            }
            case PixelFormat::BGRA8888: {
                uint8_t* bgra = static_cast<uint8_t*>(dest);
                uint8_t r, g, b, a;
                color.ToRGBA8888(r, g, b, a);
                bgra[0] = b; bgra[1] = g; bgra[2] = r; bgra[3] = a;
                break;
            }
            case PixelFormat::Grayscale8: {
                *static_cast<uint8_t*>(dest) = color.ToGrayscale8();
                break;
            }
            case PixelFormat::Monochrome: {
                // Monochrome typically stores 8 pixels per byte
                // This sets a single bit - caller must handle bit packing
                *static_cast<uint8_t*>(dest) = color.ToMonochrome() ? 0xFF : 0x00;
                break;
            }
        }
    }

    /**
     * @brief Write Color888 directly (optimized path).
     */
    static void WritePixel(void* dest, const Color888& color, PixelFormat format) {
        if (!dest) return;

        switch (format) {
            case PixelFormat::RGB888: {
                uint8_t* rgb = static_cast<uint8_t*>(dest);
                rgb[0] = color.r;
                rgb[1] = color.g;
                rgb[2] = color.b;
                break;
            }
            case PixelFormat::BGR888: {
                uint8_t* bgr = static_cast<uint8_t*>(dest);
                bgr[0] = color.b;
                bgr[1] = color.g;
                bgr[2] = color.r;
                break;
            }
            case PixelFormat::RGB565: {
                *static_cast<uint16_t*>(dest) = RGB888ToRGB565(color.r, color.g, color.b);
                break;
            }
            case PixelFormat::RGBA8888: {
                uint8_t* rgba = static_cast<uint8_t*>(dest);
                rgba[0] = color.r;
                rgba[1] = color.g;
                rgba[2] = color.b;
                rgba[3] = 255;
                break;
            }
            case PixelFormat::BGRA8888: {
                uint8_t* bgra = static_cast<uint8_t*>(dest);
                bgra[0] = color.b;
                bgra[1] = color.g;
                bgra[2] = color.r;
                bgra[3] = 255;
                break;
            }
            case PixelFormat::Grayscale8: {
                *static_cast<uint8_t*>(dest) = RGBToGrayscale(color.r, color.g, color.b);
                break;
            }
            case PixelFormat::Monochrome: {
                *static_cast<uint8_t*>(dest) = RGBToMonochrome(color.r, color.g, color.b) ? 0xFF : 0x00;
                break;
            }
        }
    }

    /**
     * @brief Write Color565 directly (optimized path).
     */
    static void WritePixel(void* dest, const Color565& color, PixelFormat format) {
        if (!dest) return;

        switch (format) {
            case PixelFormat::RGB565: {
                *static_cast<uint16_t*>(dest) = color.ToRGB565();
                break;
            }
            case PixelFormat::RGB888: {
                uint8_t* rgb = static_cast<uint8_t*>(dest);
                rgb[0] = color.GetR8();
                rgb[1] = color.GetG8();
                rgb[2] = color.GetB8();
                break;
            }
            case PixelFormat::BGR888: {
                uint8_t* bgr = static_cast<uint8_t*>(dest);
                bgr[0] = color.GetB8();
                bgr[1] = color.GetG8();
                bgr[2] = color.GetR8();
                break;
            }
            case PixelFormat::RGBA8888: {
                uint8_t* rgba = static_cast<uint8_t*>(dest);
                rgba[0] = color.GetR8();
                rgba[1] = color.GetG8();
                rgba[2] = color.GetB8();
                rgba[3] = 255;
                break;
            }
            case PixelFormat::BGRA8888: {
                uint8_t* bgra = static_cast<uint8_t*>(dest);
                bgra[0] = color.GetB8();
                bgra[1] = color.GetG8();
                bgra[2] = color.GetR8();
                bgra[3] = 255;
                break;
            }
            case PixelFormat::Grayscale8: {
                *static_cast<uint8_t*>(dest) = color.ToGrayscale8();
                break;
            }
            case PixelFormat::Monochrome: {
                *static_cast<uint8_t*>(dest) = color.ToMonochrome() ? 0xFF : 0x00;
                break;
            }
        }
    }

    /**
     * @brief Write ColorGray directly (optimized path).
     */
    static void WritePixel(void* dest, const ColorGray& color, PixelFormat format) {
        if (!dest) return;

        uint8_t gray = color.value;

        switch (format) {
            case PixelFormat::Grayscale8: {
                *static_cast<uint8_t*>(dest) = gray;
                break;
            }
            case PixelFormat::RGB888: {
                uint8_t* rgb = static_cast<uint8_t*>(dest);
                rgb[0] = rgb[1] = rgb[2] = gray;
                break;
            }
            case PixelFormat::BGR888: {
                uint8_t* bgr = static_cast<uint8_t*>(dest);
                bgr[0] = bgr[1] = bgr[2] = gray;
                break;
            }
            case PixelFormat::RGB565: {
                *static_cast<uint16_t*>(dest) = RGB888ToRGB565(gray, gray, gray);
                break;
            }
            case PixelFormat::RGBA8888: {
                uint8_t* rgba = static_cast<uint8_t*>(dest);
                rgba[0] = rgba[1] = rgba[2] = gray;
                rgba[3] = 255;
                break;
            }
            case PixelFormat::BGRA8888: {
                uint8_t* bgra = static_cast<uint8_t*>(dest);
                bgra[0] = bgra[1] = bgra[2] = gray;
                bgra[3] = 255;
                break;
            }
            case PixelFormat::Monochrome: {
                *static_cast<uint8_t*>(dest) = (gray >= 128) ? 0xFF : 0x00;
                break;
            }
        }
    }

    /**
     * @brief Write ColorMono directly (optimized path).
     */
    static void WritePixel(void* dest, const ColorMono& color, PixelFormat format) {
        if (!dest) return;

        uint8_t value = color.on ? 255 : 0;

        switch (format) {
            case PixelFormat::Monochrome: {
                *static_cast<uint8_t*>(dest) = color.on ? 0xFF : 0x00;
                break;
            }
            case PixelFormat::Grayscale8: {
                *static_cast<uint8_t*>(dest) = value;
                break;
            }
            case PixelFormat::RGB888: {
                uint8_t* rgb = static_cast<uint8_t*>(dest);
                rgb[0] = rgb[1] = rgb[2] = value;
                break;
            }
            case PixelFormat::BGR888: {
                uint8_t* bgr = static_cast<uint8_t*>(dest);
                bgr[0] = bgr[1] = bgr[2] = value;
                break;
            }
            case PixelFormat::RGB565: {
                *static_cast<uint16_t*>(dest) = color.on ? 0xFFFF : 0x0000;
                break;
            }
            case PixelFormat::RGBA8888: {
                uint8_t* rgba = static_cast<uint8_t*>(dest);
                rgba[0] = rgba[1] = rgba[2] = value;
                rgba[3] = 255;
                break;
            }
            case PixelFormat::BGRA8888: {
                uint8_t* bgra = static_cast<uint8_t*>(dest);
                bgra[0] = bgra[1] = bgra[2] = value;
                bgra[3] = 255;
                break;
            }
        }
    }

    /**
     * @brief Read a color from framebuffer pixel location.
     */
    static void ReadPixel(const void* src, IColor& color, PixelFormat format) {
        if (!src) return;

        switch (format) {
            case PixelFormat::RGB888: {
                const uint8_t* rgb = static_cast<const uint8_t*>(src);
                color.FromRGB888(rgb[0], rgb[1], rgb[2]);
                break;
            }
            case PixelFormat::BGR888: {
                const uint8_t* bgr = static_cast<const uint8_t*>(src);
                color.FromRGB888(bgr[2], bgr[1], bgr[0]);
                break;
            }
            case PixelFormat::RGB565: {
                color.FromRGB565(*static_cast<const uint16_t*>(src));
                break;
            }
            case PixelFormat::RGBA8888: {
                const uint8_t* rgba = static_cast<const uint8_t*>(src);
                color.FromRGB888(rgba[0], rgba[1], rgba[2]);
                break;
            }
            case PixelFormat::BGRA8888: {
                const uint8_t* bgra = static_cast<const uint8_t*>(src);
                color.FromRGB888(bgra[2], bgra[1], bgra[0]);
                break;
            }
            case PixelFormat::Grayscale8: {
                color.FromGrayscale8(*static_cast<const uint8_t*>(src));
                break;
            }
            case PixelFormat::Monochrome: {
                color.FromMonochrome(*static_cast<const uint8_t*>(src) != 0);
                break;
            }
        }
    }

    /**
     * @brief Batch convert an array of Color888 to a framebuffer.
     * @param dest Destination framebuffer pointer
     * @param src Source Color888 array
     * @param count Number of pixels to convert
     * @param format Target pixel format
     * @param destStride Bytes between rows in dest (0 = packed)
     */
    static void ConvertArray(void* dest, const Color888* src, size_t count, 
                            PixelFormat format, size_t destStride = 0) {
        if (!dest || !src || count == 0) return;

        size_t pixelSize = GetBytesPerPixel(format);
        if (destStride == 0) destStride = pixelSize;

        uint8_t* dptr = static_cast<uint8_t*>(dest);
        
        for (size_t i = 0; i < count; ++i) {
            WritePixel(dptr, src[i], format);
            dptr += pixelSize;
        }
    }

    KL_BEGIN_FIELDS(ColorConverter)
        /* No reflected fields. */
    KL_END_FIELDS

    KL_BEGIN_METHODS(ColorConverter)
        /* Write pixel */ KL_SMETHOD_OVLD(ColorConverter, WritePixel, void, void *, const IColor &, PixelFormat),
        /* Write pixel */ KL_SMETHOD_OVLD(ColorConverter, WritePixel, void, void *, const Color888 &, PixelFormat),
        /* Write pixel */ KL_SMETHOD_OVLD(ColorConverter, WritePixel, void, void *, const Color565 &, PixelFormat),
        /* Write pixel */ KL_SMETHOD_OVLD(ColorConverter, WritePixel, void, void *, const ColorGray &, PixelFormat),
        /* Write pixel */ KL_SMETHOD_OVLD(ColorConverter, WritePixel, void, void *, const ColorMono &, PixelFormat),
        KL_SMETHOD_AUTO(ColorConverter::ReadPixel, "Read pixel"),
        KL_SMETHOD_AUTO(ColorConverter::ConvertArray, "Convert array")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(ColorConverter)
        /* No reflected ctors. */
    KL_END_DESCRIBE(ColorConverter)

};

} // namespace koilo
