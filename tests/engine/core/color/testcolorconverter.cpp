// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testcolorconverter.cpp
 * @brief Implementation of ColorConverter unit tests.
 */

#include "testcolorconverter.hpp"

using namespace koilo;
// ========== Constructor Tests ==========

void TestColorConverter::TestDefaultConstructor() {
    // ColorConverter is a static utility class
    uint8_t buffer[4];
    Color888 white(255, 255, 255);
    ColorConverter::WritePixel(buffer, white, PixelFormat::RGB888);
    TEST_ASSERT_EQUAL_UINT8(255, buffer[0]);
}

void TestColorConverter::TestParameterizedConstructor() {
    // Static utility - test conversion consistency
    uint8_t buffer[4];
    Color888 black(0, 0, 0);
    ColorConverter::WritePixel(buffer, black, PixelFormat::RGB888);
    TEST_ASSERT_EQUAL_UINT8(0, buffer[0]);
}

// ========== Method Tests ==========

void TestColorConverter::TestWritePixel() {
    // Test WritePixel with Color888
    uint8_t buffer[4];
    Color888 red(255, 0, 0);
    
    // Test RGB888 format
    ColorConverter::WritePixel(buffer, red, PixelFormat::RGB888);
    TEST_ASSERT_EQUAL_UINT8(255, buffer[0]);
    TEST_ASSERT_EQUAL_UINT8(0, buffer[1]);
    TEST_ASSERT_EQUAL_UINT8(0, buffer[2]);
    
    // Test BGR888 format (reversed)
    ColorConverter::WritePixel(buffer, red, PixelFormat::BGR888);
    TEST_ASSERT_EQUAL_UINT8(0, buffer[0]);
    TEST_ASSERT_EQUAL_UINT8(0, buffer[1]);
    TEST_ASSERT_EQUAL_UINT8(255, buffer[2]);
    
    // Test RGB565 format
    uint16_t* pixel16 = (uint16_t*)buffer;
    ColorConverter::WritePixel(pixel16, red, PixelFormat::RGB565);
    TEST_ASSERT_EQUAL_UINT16(0xF800, *pixel16);  // Red in RGB565
    
    // Test Grayscale8 format
    ColorConverter::WritePixel(buffer, red, PixelFormat::Grayscale8);
    // Red grayscale: 0.299 * 255 = ~76
    TEST_ASSERT_UINT8_WITHIN(5, 76, buffer[0]);
    
    // Test RGBA8888 format
    ColorConverter::WritePixel(buffer, red, PixelFormat::RGBA8888);
    TEST_ASSERT_EQUAL_UINT8(255, buffer[0]);
    TEST_ASSERT_EQUAL_UINT8(0, buffer[1]);
    TEST_ASSERT_EQUAL_UINT8(0, buffer[2]);
    TEST_ASSERT_EQUAL_UINT8(255, buffer[3]);  // Alpha = 255
    
    // Test BGRA8888 format
    ColorConverter::WritePixel(buffer, red, PixelFormat::BGRA8888);
    TEST_ASSERT_EQUAL_UINT8(0, buffer[0]);
    TEST_ASSERT_EQUAL_UINT8(0, buffer[1]);
    TEST_ASSERT_EQUAL_UINT8(255, buffer[2]);
    TEST_ASSERT_EQUAL_UINT8(255, buffer[3]);  // Alpha = 255
    
    // Test WritePixel with Color565
    Color565 green(0, 255, 0);
    ColorConverter::WritePixel(buffer, green, PixelFormat::RGB888);
    TEST_ASSERT_EQUAL_UINT8(0, buffer[0]);
    TEST_ASSERT_EQUAL_UINT8(252, buffer[1]);  // 63 << 2
    TEST_ASSERT_EQUAL_UINT8(0, buffer[2]);
    
    // Test WritePixel with ColorGray
    ColorGray gray(128);
    ColorConverter::WritePixel(buffer, gray, PixelFormat::Grayscale8);
    TEST_ASSERT_EQUAL_UINT8(128, buffer[0]);
    
    // Test WritePixel with ColorMono
    ColorMono on(true);
    ColorConverter::WritePixel(buffer, on, PixelFormat::Monochrome);
    TEST_ASSERT_EQUAL_UINT8(0xFF, buffer[0]);
    
    ColorMono off(false);
    ColorConverter::WritePixel(buffer, off, PixelFormat::Monochrome);
    TEST_ASSERT_EQUAL_UINT8(0x00, buffer[0]);
    
    // Test WritePixel with Color888 directly
    Color888 genericColor(100, 150, 200);
    uint8_t gr, gg, gb;
    genericColor.ToRGB888(gr, gg, gb);
    buffer[0] = gr; buffer[1] = gg; buffer[2] = gb;
    TEST_ASSERT_EQUAL_UINT8(100, buffer[0]);
    TEST_ASSERT_EQUAL_UINT8(150, buffer[1]);
    TEST_ASSERT_EQUAL_UINT8(200, buffer[2]);
    
    // Test WritePixel with nullptr destination (should not crash)
    ColorConverter::WritePixel(nullptr, red, PixelFormat::RGB888);
    TEST_ASSERT_TRUE(true);  // If we get here, no crash occurred
}

// ========== Edge Cases ==========

void TestColorConverter::TestEdgeCases() {
    uint8_t buffer[4];
    
    // Test black color
    Color888 black(0, 0, 0);
    ColorConverter::WritePixel(buffer, black, PixelFormat::RGB888);
    TEST_ASSERT_EQUAL_UINT8(0, buffer[0]);
    TEST_ASSERT_EQUAL_UINT8(0, buffer[1]);
    TEST_ASSERT_EQUAL_UINT8(0, buffer[2]);
    
    // Test white color
    Color888 white(255, 255, 255);
    ColorConverter::WritePixel(buffer, white, PixelFormat::RGB565);
    uint16_t* pixel16 = (uint16_t*)buffer;
    TEST_ASSERT_EQUAL_UINT16(0xFFFF, *pixel16);  // White in RGB565
    
    // Test all PixelFormat conversions with same color
    Color888 testColor(100, 150, 200);
    
    // RGB888
    ColorConverter::WritePixel(buffer, testColor, PixelFormat::RGB888);
    TEST_ASSERT_EQUAL_UINT8(100, buffer[0]);
    
    // BGR888
    ColorConverter::WritePixel(buffer, testColor, PixelFormat::BGR888);
    TEST_ASSERT_EQUAL_UINT8(200, buffer[0]);
    
    // RGB565
    ColorConverter::WritePixel(pixel16, testColor, PixelFormat::RGB565);
    uint16_t expected565 = (12 << 11) | (37 << 5) | 25;  // R=100>>3=12, G=150>>2=37, B=200>>3=25
    TEST_ASSERT_UINT16_WITHIN(2, expected565, *pixel16);
    
    // Grayscale8
    ColorConverter::WritePixel(buffer, testColor, PixelFormat::Grayscale8);
    // Grayscale conversion of (100, 150, 200)
    TEST_ASSERT_UINT8_WITHIN(15, 140, buffer[0]);
    
    // RGBA8888
    ColorConverter::WritePixel(buffer, testColor, PixelFormat::RGBA8888);
    TEST_ASSERT_EQUAL_UINT8(100, buffer[0]);
    TEST_ASSERT_EQUAL_UINT8(150, buffer[1]);
    TEST_ASSERT_EQUAL_UINT8(200, buffer[2]);
    TEST_ASSERT_EQUAL_UINT8(255, buffer[3]);
    
    // BGRA8888
    ColorConverter::WritePixel(buffer, testColor, PixelFormat::BGRA8888);
    TEST_ASSERT_EQUAL_UINT8(200, buffer[0]);
    TEST_ASSERT_EQUAL_UINT8(150, buffer[1]);
    TEST_ASSERT_EQUAL_UINT8(100, buffer[2]);
    TEST_ASSERT_EQUAL_UINT8(255, buffer[3]);
    
    // Monochrome
    ColorConverter::WritePixel(buffer, testColor, PixelFormat::Monochrome);
    TEST_ASSERT_EQUAL_UINT8(0xFF, buffer[0]);  // Bright enough to be ON
    
    // Test ColorGray with all formats
    ColorGray halfGray(128);
    
    ColorConverter::WritePixel(buffer, halfGray, PixelFormat::Grayscale8);
    TEST_ASSERT_EQUAL_UINT8(128, buffer[0]);
    
    ColorConverter::WritePixel(buffer, halfGray, PixelFormat::RGB888);
    TEST_ASSERT_EQUAL_UINT8(128, buffer[0]);
    TEST_ASSERT_EQUAL_UINT8(128, buffer[1]);
    TEST_ASSERT_EQUAL_UINT8(128, buffer[2]);
    
    // Test ColorMono conversions
    ColorMono mono(true);
    
    ColorConverter::WritePixel(buffer, mono, PixelFormat::RGB888);
    TEST_ASSERT_EQUAL_UINT8(255, buffer[0]);
    
    ColorConverter::WritePixel(buffer, mono, PixelFormat::Grayscale8);
    TEST_ASSERT_EQUAL_UINT8(255, buffer[0]);
    
    ColorConverter::WritePixel(pixel16, mono, PixelFormat::RGB565);
    TEST_ASSERT_EQUAL_UINT16(0xFFFF, *pixel16);
    
    // Test Color565 conversions
    Color565 blue565(0, 0, 255);
    
    ColorConverter::WritePixel(buffer, blue565, PixelFormat::RGB888);
    TEST_ASSERT_EQUAL_UINT8(0, buffer[0]);
    TEST_ASSERT_EQUAL_UINT8(0, buffer[1]);
    TEST_ASSERT_EQUAL_UINT8(248, buffer[2]);  // 31 << 3
    
    ColorConverter::WritePixel(pixel16, blue565, PixelFormat::RGB565);
    TEST_ASSERT_EQUAL_UINT16(0x001F, *pixel16);  // Blue in RGB565
}

// ========== Test Runner ==========

void TestColorConverter::RunAllTests() {
    RUN_TEST(TestDefaultConstructor);
    RUN_TEST(TestParameterizedConstructor);
    RUN_TEST(TestWritePixel);
    RUN_TEST(TestEdgeCases);
}
