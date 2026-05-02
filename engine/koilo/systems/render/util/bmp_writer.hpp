// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
/**
 * @file bmp_writer.hpp
 * @brief Tiny header-only BMP24 writer (no dependencies).
 *
 * Used by the screenshot service to dump captured swapchain pixels.  BMP
 * was chosen because it requires zero external libs and the readback
 * buffers we have are already RGBA8 in memory.
 */

#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

namespace koilo::render::util {

/// Write an RGBA8 pixel buffer to a 24-bpp uncompressed BMP file.
///
/// @param path        Destination file path.
/// @param rgba        RGBA8 pixel buffer (size >= width*height*4 bytes).
/// @param width       Image width in pixels.
/// @param height      Image height in pixels.
/// @param topDown     If true, pixel row 0 is the top of the image (Vulkan,
///                    Software RHI).  If false, row 0 is the bottom (OpenGL).
/// @return            true on success, false on file-open or short-write.
inline bool WriteBMP24(const std::string& path,
                       const uint8_t* rgba,
                       uint32_t width,
                       uint32_t height,
                       bool topDown = true) {
    if (!rgba || width == 0 || height == 0) return false;

    const uint32_t rowBytes = width * 3;
    const uint32_t pad      = (4 - (rowBytes % 4)) % 4;
    const uint32_t stride   = rowBytes + pad;
    const uint32_t imgSize  = stride * height;
    const uint32_t fileSize = 14 + 40 + imgSize;

    FILE* fp = std::fopen(path.c_str(), "wb");
    if (!fp) return false;

    // BITMAPFILEHEADER (14 bytes)
    uint8_t fh[14] = {
        'B', 'M',
        static_cast<uint8_t>(fileSize),
        static_cast<uint8_t>(fileSize >> 8),
        static_cast<uint8_t>(fileSize >> 16),
        static_cast<uint8_t>(fileSize >> 24),
        0, 0, 0, 0,                              // reserved
        54, 0, 0, 0                              // pixel data offset
    };
    if (std::fwrite(fh, 1, 14, fp) != 14) { std::fclose(fp); return false; }

    // BITMAPINFOHEADER (40 bytes).  Negative height = top-down rows.
    int32_t  hSigned = topDown ? -static_cast<int32_t>(height)
                               :  static_cast<int32_t>(height);
    uint32_t w = width;
    uint8_t  ih[40] = {
        40, 0, 0, 0,                             // header size
        static_cast<uint8_t>(w),
        static_cast<uint8_t>(w >> 8),
        static_cast<uint8_t>(w >> 16),
        static_cast<uint8_t>(w >> 24),
        static_cast<uint8_t>(hSigned),
        static_cast<uint8_t>(hSigned >> 8),
        static_cast<uint8_t>(hSigned >> 16),
        static_cast<uint8_t>(hSigned >> 24),
        1, 0,                                    // planes
        24, 0,                                   // bits per pixel
        0, 0, 0, 0,                              // compression = BI_RGB
        static_cast<uint8_t>(imgSize),
        static_cast<uint8_t>(imgSize >> 8),
        static_cast<uint8_t>(imgSize >> 16),
        static_cast<uint8_t>(imgSize >> 24),
        0x13, 0x0B, 0, 0,                        // x ppm = 2835
        0x13, 0x0B, 0, 0,                        // y ppm = 2835
        0, 0, 0, 0,                              // colors used
        0, 0, 0, 0                               // important colors
    };
    if (std::fwrite(ih, 1, 40, fp) != 40) { std::fclose(fp); return false; }

    // Pixel data: RGBA -> BGR with row padding.
    std::vector<uint8_t> row(stride, 0);
    for (uint32_t y = 0; y < height; ++y) {
        const uint8_t* src = rgba + y * width * 4;
        uint8_t* dst = row.data();
        for (uint32_t x = 0; x < width; ++x) {
            dst[0] = src[2];   // B
            dst[1] = src[1];   // G
            dst[2] = src[0];   // R
            dst += 3; src += 4;
        }
        if (std::fwrite(row.data(), 1, stride, fp) != stride) {
            std::fclose(fp); return false;
        }
    }

    std::fclose(fp);
    return true;
}

} // namespace koilo::render::util
