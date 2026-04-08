// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testlivepreview.cpp
 * @brief Unit tests for the live preview BMP encoder and HTTP parser.
 *
 * @date 04/06/2026
 * @author Coela Can't
 */

#include "testlivepreview.hpp"

#ifdef KL_HAVE_LIVE_PREVIEW
#include <koilo/systems/display/preview/live_preview_module.hpp>
#endif

#include <unity.h>
#include <vector>
#include <cstring>
#include <cstdint>

using namespace koilo;

// -- BMP encoder tests --------------------------------------------------------

#ifdef KL_HAVE_LIVE_PREVIEW

static uint32_t ReadLE32(const uint8_t* p) {
    return static_cast<uint32_t>(p[0])
         | (static_cast<uint32_t>(p[1]) << 8)
         | (static_cast<uint32_t>(p[2]) << 16)
         | (static_cast<uint32_t>(p[3]) << 24);
}

static uint16_t ReadLE16(const uint8_t* p) {
    return static_cast<uint16_t>(p[0])
         | (static_cast<uint16_t>(p[1]) << 8);
}

static void TestBMPHeaderSignature() {
    uint8_t rgba[16] = { 255, 0, 0, 255,  0, 255, 0, 255,
                         0, 0, 255, 255,  128, 128, 128, 255 };
    std::vector<uint8_t> bmp;
    LivePreviewModule::EncodeBMP(rgba, 2, 2, bmp);

    TEST_ASSERT_TRUE(bmp.size() > 54);
    TEST_ASSERT_EQUAL_UINT8('B', bmp[0]);
    TEST_ASSERT_EQUAL_UINT8('M', bmp[1]);
}

static void TestBMPFileSize() {
    uint8_t rgba[4 * 4] = {};
    std::vector<uint8_t> bmp;
    LivePreviewModule::EncodeBMP(rgba, 2, 2, bmp);

    // row = 2*3 = 6 bytes, padded to 8 (next multiple of 4)
    // image = 8 * 2 = 16
    // file = 54 + 16 = 70
    uint32_t fileSize = ReadLE32(bmp.data() + 2);
    TEST_ASSERT_EQUAL_UINT32(70, fileSize);
    TEST_ASSERT_EQUAL_UINT32(70, static_cast<uint32_t>(bmp.size()));
}

static void TestBMPDimensions() {
    const uint32_t W = 320, H = 200;
    std::vector<uint8_t> rgba(W * H * 4, 0);
    std::vector<uint8_t> bmp;
    LivePreviewModule::EncodeBMP(rgba.data(), W, H, bmp);

    uint32_t width = ReadLE32(bmp.data() + 18);
    int32_t height;
    std::memcpy(&height, bmp.data() + 22, 4);

    TEST_ASSERT_EQUAL_UINT32(W, width);
    // Negative height = top-down scanlines
    TEST_ASSERT_EQUAL_INT32(-200, height);
}

static void TestBMPBitsPerPixel() {
    uint8_t rgba[4] = { 0, 0, 0, 255 };
    std::vector<uint8_t> bmp;
    LivePreviewModule::EncodeBMP(rgba, 1, 1, bmp);

    uint16_t bpp = ReadLE16(bmp.data() + 28);
    TEST_ASSERT_EQUAL_UINT16(24, bpp);
}

static void TestBMPPixelDataRGBAToBGR() {
    // Single pixel: RGBA = (0xAA, 0xBB, 0xCC, 0xFF)
    // BMP should store BGR = (0xCC, 0xBB, 0xAA)
    uint8_t rgba[4] = { 0xAA, 0xBB, 0xCC, 0xFF };
    std::vector<uint8_t> bmp;
    LivePreviewModule::EncodeBMP(rgba, 1, 1, bmp);

    uint32_t pixelOffset = ReadLE32(bmp.data() + 10);
    TEST_ASSERT_EQUAL_UINT8(0xCC, bmp[pixelOffset + 0]); // B
    TEST_ASSERT_EQUAL_UINT8(0xBB, bmp[pixelOffset + 1]); // G
    TEST_ASSERT_EQUAL_UINT8(0xAA, bmp[pixelOffset + 2]); // R
}

static void TestBMPRowPadding() {
    // Width 1: row = 3 bytes, padded to 4
    uint8_t rgba[4] = { 0, 0, 0, 255 };
    std::vector<uint8_t> bmp;
    LivePreviewModule::EncodeBMP(rgba, 1, 1, bmp);

    // padded row = 4, image = 4, file = 54 + 4 = 58
    TEST_ASSERT_EQUAL_UINT32(58, static_cast<uint32_t>(bmp.size()));
}

static void TestBMPRowPaddingWidth3() {
    // Width 3: row = 9 bytes, padded to 12
    std::vector<uint8_t> rgba(3 * 1 * 4, 128);
    std::vector<uint8_t> bmp;
    LivePreviewModule::EncodeBMP(rgba.data(), 3, 1, bmp);

    // padded row = 12, image = 12, file = 54 + 12 = 66
    TEST_ASSERT_EQUAL_UINT32(66, static_cast<uint32_t>(bmp.size()));
}

static void TestBMPMultipleRows() {
    // 4x2 image: row = 12 bytes (no padding needed)
    // Red pixels in row 0, blue pixels in row 1
    uint8_t rgba[4 * 2 * 4];
    for (int i = 0; i < 4; ++i) {
        rgba[i * 4 + 0] = 255; // R
        rgba[i * 4 + 1] = 0;
        rgba[i * 4 + 2] = 0;
        rgba[i * 4 + 3] = 255;
    }
    for (int i = 4; i < 8; ++i) {
        rgba[i * 4 + 0] = 0;
        rgba[i * 4 + 1] = 0;
        rgba[i * 4 + 2] = 255; // B
        rgba[i * 4 + 3] = 255;
    }

    std::vector<uint8_t> bmp;
    LivePreviewModule::EncodeBMP(rgba, 4, 2, bmp);

    uint32_t offset = ReadLE32(bmp.data() + 10);
    // Row 0 (top-down): red -> BGR = (0, 0, 255)
    TEST_ASSERT_EQUAL_UINT8(0, bmp[offset + 0]);
    TEST_ASSERT_EQUAL_UINT8(0, bmp[offset + 1]);
    TEST_ASSERT_EQUAL_UINT8(255, bmp[offset + 2]);

    // Row 1: blue -> BGR = (255, 0, 0)
    uint32_t paddedRow = 12;
    TEST_ASSERT_EQUAL_UINT8(255, bmp[offset + paddedRow + 0]);
    TEST_ASSERT_EQUAL_UINT8(0, bmp[offset + paddedRow + 1]);
    TEST_ASSERT_EQUAL_UINT8(0, bmp[offset + paddedRow + 2]);
}

static void TestBMPLargeImage() {
    const uint32_t W = 640, H = 480;
    std::vector<uint8_t> rgba(W * H * 4, 0);
    std::vector<uint8_t> bmp;
    LivePreviewModule::EncodeBMP(rgba.data(), W, H, bmp);

    uint32_t paddedRow = (W * 3 + 3) & ~3u;
    uint32_t expected = 54 + paddedRow * H;
    TEST_ASSERT_EQUAL_UINT32(expected, static_cast<uint32_t>(bmp.size()));
}

// -- HTTP parser tests --------------------------------------------------------

static void TestHTTPParseRoot() {
    const char* req = "GET / HTTP/1.1\r\nHost: localhost\r\n\r\n";
    std::string path;
    bool ok = LivePreviewModule::ParseHTTPRequest(req, std::strlen(req), path);
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_EQUAL_STRING("/", path.c_str());
}

static void TestHTTPParseStream() {
    const char* req = "GET /stream HTTP/1.1\r\n\r\n";
    std::string path;
    bool ok = LivePreviewModule::ParseHTTPRequest(req, std::strlen(req), path);
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_EQUAL_STRING("/stream", path.c_str());
}

static void TestHTTPParseStreamWithQuery() {
    const char* req = "GET /stream?t=12345 HTTP/1.1\r\n\r\n";
    std::string path;
    bool ok = LivePreviewModule::ParseHTTPRequest(req, std::strlen(req), path);
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_EQUAL_STRING("/stream", path.c_str());
}

static void TestHTTPParseInvalidMethod() {
    const char* req = "POST / HTTP/1.1\r\n\r\n";
    std::string path;
    bool ok = LivePreviewModule::ParseHTTPRequest(req, std::strlen(req), path);
    TEST_ASSERT_FALSE(ok);
}

static void TestHTTPParseTooShort() {
    const char* req = "GET /";
    std::string path;
    bool ok = LivePreviewModule::ParseHTTPRequest(req, std::strlen(req), path);
    TEST_ASSERT_FALSE(ok);
}

static void TestHTTPParseEmpty() {
    std::string path;
    bool ok = LivePreviewModule::ParseHTTPRequest("", 0, path);
    TEST_ASSERT_FALSE(ok);
}

static void TestHTTPParseFavicon() {
    const char* req = "GET /favicon.ico HTTP/1.1\r\n\r\n";
    std::string path;
    bool ok = LivePreviewModule::ParseHTTPRequest(req, std::strlen(req), path);
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_EQUAL_STRING("/favicon.ico", path.c_str());
}

#endif // KL_HAVE_LIVE_PREVIEW

// -- Runner -------------------------------------------------------------------

void RunTestLivePreview() {
#ifdef KL_HAVE_LIVE_PREVIEW
    // BMP encoder
    RUN_TEST(TestBMPHeaderSignature);
    RUN_TEST(TestBMPFileSize);
    RUN_TEST(TestBMPDimensions);
    RUN_TEST(TestBMPBitsPerPixel);
    RUN_TEST(TestBMPPixelDataRGBAToBGR);
    RUN_TEST(TestBMPRowPadding);
    RUN_TEST(TestBMPRowPaddingWidth3);
    RUN_TEST(TestBMPMultipleRows);
    RUN_TEST(TestBMPLargeImage);

    // HTTP parser
    RUN_TEST(TestHTTPParseRoot);
    RUN_TEST(TestHTTPParseStream);
    RUN_TEST(TestHTTPParseStreamWithQuery);
    RUN_TEST(TestHTTPParseInvalidMethod);
    RUN_TEST(TestHTTPParseTooShort);
    RUN_TEST(TestHTTPParseEmpty);
    RUN_TEST(TestHTTPParseFavicon);
#endif
}
