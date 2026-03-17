// SPDX-License-Identifier: GPL-3.0-or-later
#include <koilo/assets/image/texture.hpp>
#include <algorithm>
#include <fstream>
#include <cstring>

namespace koilo {

Texture::Texture() = default;

Texture::Texture(const uint8_t* indices, const uint8_t* palette, uint8_t paletteSize,
                 uint32_t width, uint32_t height)
    : format_(Format::Palette), width_(width), height_(height),
      indices_(indices), palette_(palette), paletteSize_(paletteSize) {}

Texture::Texture(const Color888* pixels, uint32_t width, uint32_t height)
    : format_(Format::RGB888), width_(width), height_(height), pixels_(pixels) {}

Texture::~Texture() {
    FreeOwned();
}

void Texture::FreeOwned() {
    ownedIndices_.clear();
    ownedPixels_.clear();
}

void Texture::CreatePalette(uint32_t width, uint32_t height, const uint8_t* palette, uint8_t paletteSize) {
    FreeOwned();
    width_ = width;
    height_ = height;
    format_ = Format::Palette;
    paletteSize_ = paletteSize;
    palette_ = palette;
    ownedIndices_.resize(width * height, 0);
    indices_ = ownedIndices_.data();
    pixels_ = nullptr;
}

void Texture::CreateRGB(uint32_t width, uint32_t height) {
    FreeOwned();
    width_ = width;
    height_ = height;
    format_ = Format::RGB888;
    ownedPixels_.resize(width * height, Color888(0, 0, 0));
    pixels_ = ownedPixels_.data();
    indices_ = nullptr;
    palette_ = nullptr;
    paletteSize_ = 0;
}

Color888 Texture::SamplePixel(uint32_t x, uint32_t y) const {
    if (x >= width_ || y >= height_) return Color888(0, 0, 0);

    if (format_ == Format::RGB888 && pixels_) {
        return pixels_[y * width_ + x];
    }
    if (format_ == Format::Palette && indices_ && palette_) {
        uint8_t idx = indices_[y * width_ + x];
        uint32_t pos = idx * 3;
        if (idx >= paletteSize_) return Color888(0, 0, 0);
        return Color888(palette_[pos], palette_[pos + 1], palette_[pos + 2]);
    }
    return Color888(0, 0, 0);
}

Color888 Texture::SampleUV(float u, float v) const {
    if (width_ == 0 || height_ == 0) return Color888(0, 0, 0);
    // Clamp UV to [0, 1]
    u = std::max(0.0f, std::min(1.0f, u));
    v = std::max(0.0f, std::min(1.0f, v));
    uint32_t x = static_cast<uint32_t>(u * (width_ - 1));
    uint32_t y = static_cast<uint32_t>(v * (height_ - 1));
    return SamplePixel(x, y);
}

Color888 Texture::SampleRect(float u, float v, uint32_t rx, uint32_t ry, uint32_t rw, uint32_t rh) const {
    if (rw == 0 || rh == 0) return Color888(0, 0, 0);
    u = std::max(0.0f, std::min(1.0f, u));
    v = std::max(0.0f, std::min(1.0f, v));
    uint32_t x = rx + static_cast<uint32_t>(u * (rw - 1));
    uint32_t y = ry + static_cast<uint32_t>(v * (rh - 1));
    return SamplePixel(x, y);
}

void Texture::SetPixel(uint32_t x, uint32_t y, Color888 color) {
    if (format_ != Format::RGB888 || ownedPixels_.empty()) return;
    if (x >= width_ || y >= height_) return;
    ownedPixels_[y * width_ + x] = color;
}

void Texture::SetIndex(uint32_t x, uint32_t y, uint8_t index) {
    if (format_ != Format::Palette || ownedIndices_.empty()) return;
    if (x >= width_ || y >= height_) return;
    ownedIndices_[y * width_ + x] = index;
}

void Texture::SetPalette(const uint8_t* palette, uint8_t size) {
    palette_ = palette;
    paletteSize_ = size;
}

bool Texture::LoadFile(const char* filepath) {
    std::ifstream file(filepath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) return false;
    
    size_t fileSize = static_cast<size_t>(file.tellg());
    if (fileSize < 16) return false;
    file.seekg(0);
    
    // Read header (16 bytes)
    char magic[4];
    file.read(magic, 4);
    if (std::memcmp(magic, "KTEX", 4) != 0) return false;
    
    uint32_t w, h;
    uint8_t channels;
    file.read(reinterpret_cast<char*>(&w), 4);
    file.read(reinterpret_cast<char*>(&h), 4);
    file.read(reinterpret_cast<char*>(&channels), 1);
    file.seekg(16); // Skip reserved bytes
    
    if (w == 0 || h == 0 || (channels != 3 && channels != 4)) return false;
    
    size_t pixelDataSize = static_cast<size_t>(w) * h * channels;
    if (fileSize < 16 + pixelDataSize) return false;
    
    // Read pixel data and convert to RGB888
    FreeOwned();
    width_ = w;
    height_ = h;
    format_ = Format::RGB888;
    
    ownedPixels_.resize(w * h);
    
    if (channels == 3) {
        std::vector<uint8_t> raw(pixelDataSize);
        file.read(reinterpret_cast<char*>(raw.data()), pixelDataSize);
        for (uint32_t i = 0; i < w * h; ++i) {
            ownedPixels_[i] = Color888(raw[i * 3], raw[i * 3 + 1], raw[i * 3 + 2]);
        }
    } else {
        // RGBA - drop alpha
        std::vector<uint8_t> raw(pixelDataSize);
        file.read(reinterpret_cast<char*>(raw.data()), pixelDataSize);
        for (uint32_t i = 0; i < w * h; ++i) {
            ownedPixels_[i] = Color888(raw[i * 4], raw[i * 4 + 1], raw[i * 4 + 2]);
        }
    }
    
    pixels_ = ownedPixels_.data();
    indices_ = nullptr;
    palette_ = nullptr;
    paletteSize_ = 0;
    
    return true;
}

} // namespace koilo
