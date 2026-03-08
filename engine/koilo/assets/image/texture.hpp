// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file texture.hpp
 * @brief Owned pixel buffer for 2D texture data.
 *
 * Texture holds pixel data as either:
 * - Palette-indexed (uint8_t indices + RGB palette) - MCU-friendly, 1 byte/pixel
 * - Raw Color888 buffer - for BMP loading or procedural generation
 *
 * Unlike Image, Texture has no embedded transform - positioning is handled
 * by the Mesh/SceneNode that references it. Texture is pure data.
 *
 * @date 2026-02-21
 * @author Coela
 */
#pragma once

#include <cstdint>
#include <cstddef>
#include <koilo/core/color/color888.hpp>
#include <koilo/core/math/vector2d.hpp>
#include <koilo/registry/reflect_macros.hpp>

namespace koilo {

class Texture {
public:
    enum class Format : uint8_t {
        Palette,   ///< 1 byte/pixel index + RGB palette (MCU default)
        RGB888     ///< 3 bytes/pixel direct Color888 (BMP / procedural)
    };

    // Construct empty texture (must call Create* before use).
    Texture();

    // Construct a palette-indexed texture from external data (non-owning).
    Texture(const uint8_t* indices, const uint8_t* palette, uint8_t paletteSize,
            uint32_t width, uint32_t height);

    // Construct a raw RGB888 texture from external data (non-owning).
    Texture(const Color888* pixels, uint32_t width, uint32_t height);

    ~Texture();

    // --- Factory methods for owned buffers ---

    // Allocate an owned palette-indexed buffer (filled with index 0).
    void CreatePalette(uint32_t width, uint32_t height, const uint8_t* palette, uint8_t paletteSize);

    // Allocate an owned RGB888 buffer (filled with black).
    void CreateRGB(uint32_t width, uint32_t height);

    // --- Accessors ---

    uint32_t GetWidth() const { return width_; }
    uint32_t GetHeight() const { return height_; }
    Format GetFormat() const { return format_; }

    // Sample color at normalized UV coordinates (0-1 range).
    Color888 SampleUV(float u, float v) const;

    // Sample color at pixel coordinates (clamped).
    Color888 SamplePixel(uint32_t x, uint32_t y) const;

    // Set a pixel in an owned RGB888 buffer.
    void SetPixel(uint32_t x, uint32_t y, Color888 color);

    // Set a pixel index in an owned palette buffer.
    void SetIndex(uint32_t x, uint32_t y, uint8_t index);

    // --- Palette access ---

    const uint8_t* GetPalette() const { return palette_; }
    uint8_t GetPaletteSize() const { return paletteSize_; }

    // Replace palette pointer (non-owning, caller manages lifetime).
    void SetPalette(const uint8_t* palette, uint8_t size);

    // --- Raw data access ---

    const uint8_t* GetIndices() const { return indices_; }
    const Color888* GetPixels() const { return pixels_; }

    // --- Sub-rectangle sampling for sprite sheets ---

    // Sample UV within a sub-rectangle (in pixel coordinates).
    // Maps u,v from [0,1] into [rx, rx+rw] × [ry, ry+rh].
    Color888 SampleRect(float u, float v, uint32_t rx, uint32_t ry, uint32_t rw, uint32_t rh) const;

    // Load a .ktex binary texture file.
    bool LoadFile(const char* filepath);

    // Get raw RGB data pointer (3 bytes per pixel). Returns nullptr if not RGB888.
    const uint8_t* GetRGBData() const {
        return pixels_ ? reinterpret_cast<const uint8_t*>(pixels_) : nullptr;
    }

private:
    Format format_ = Format::Palette;
    uint32_t width_ = 0;
    uint32_t height_ = 0;

    // Palette mode
    const uint8_t* indices_ = nullptr;
    const uint8_t* palette_ = nullptr;
    uint8_t paletteSize_ = 0;

    // RGB mode
    const Color888* pixels_ = nullptr;

    // Owned buffer tracking
    uint8_t* ownedIndices_ = nullptr;
    Color888* ownedPixels_ = nullptr;

    void FreeOwned();

public:
    KL_BEGIN_FIELDS(Texture)
    KL_END_FIELDS

    KL_BEGIN_METHODS(Texture)
        KL_METHOD_AUTO(Texture, GetWidth, "Get width"),
        KL_METHOD_AUTO(Texture, GetHeight, "Get height"),
        KL_METHOD_AUTO(Texture, SampleUV, "Sample at UV"),
        KL_METHOD_AUTO(Texture, SamplePixel, "Sample at pixel"),
        KL_METHOD_AUTO(Texture, SetPixel, "Set pixel"),
        KL_METHOD_AUTO(Texture, SetIndex, "Set index"),
        KL_METHOD_AUTO(Texture, CreateRGB, "Create RGB buffer"),
        KL_METHOD_AUTO(Texture, GetPaletteSize, "Get palette size"),
        KL_METHOD_AUTO(Texture, LoadFile, "Load .ktex file"),
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(Texture)
        KL_CTOR0(Texture),
        KL_CTOR(Texture, const uint8_t*, const uint8_t*, uint8_t, uint32_t, uint32_t),
        KL_CTOR(Texture, const Color888*, uint32_t, uint32_t)
    KL_END_DESCRIBE(Texture)
};

} // namespace koilo
