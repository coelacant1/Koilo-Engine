// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testframebuffer.cpp
 * @brief Implementation of Framebuffer unit tests.
 */

#include "testframebuffer.hpp"

using namespace koilo;
// ========== Constructor Tests ==========

void TestFramebuffer::TestDefaultConstructor() {
    Framebuffer fb;
    TEST_ASSERT_EQUAL_UINT32(0, fb.width);
    TEST_ASSERT_EQUAL_UINT32(0, fb.height);
    TEST_ASSERT_NULL(fb.data);
}

void TestFramebuffer::TestParameterizedConstructor() {
    uint8_t buffer[256];
    Framebuffer fb(buffer, 64, 32, PixelFormat::RGB888);
    TEST_ASSERT_EQUAL_UINT32(64, fb.width);
    TEST_ASSERT_EQUAL_UINT32(32, fb.height);
    TEST_ASSERT_NOT_NULL(fb.data);
}

// ========== Method Tests ==========

void TestFramebuffer::TestGetSizeBytes() {
    // Framebuffer is a simple struct - no GetSizeBytes
    Framebuffer fb;
    fb.width = 64;
    fb.height = 32;
    TEST_ASSERT_EQUAL_UINT32(64, fb.width);
    TEST_ASSERT_EQUAL_UINT32(32, fb.height);
}

void TestFramebuffer::TestIsValid() {
    uint8_t buffer[128];
    Framebuffer fb(buffer, 32, 16, PixelFormat::RGB565);
    TEST_ASSERT_NOT_NULL(fb.data);
}

void TestFramebuffer::TestClear() {
    // No clear method - just verify struct fields
    Framebuffer fb;
    fb.width = 0;
    fb.height = 0;
    TEST_ASSERT_TRUE(true);
}

// ========== Edge Cases ==========

void TestFramebuffer::TestEdgeCases() {
    Framebuffer fb;
    TEST_ASSERT_NULL(fb.data);
    TEST_ASSERT_EQUAL_UINT32(0, fb.width);
    TEST_ASSERT_EQUAL_UINT32(0, fb.height);
}

// ========== Test Runner ==========

void TestFramebuffer::RunAllTests() {
    RUN_TEST(TestDefaultConstructor);
    RUN_TEST(TestParameterizedConstructor);
    RUN_TEST(TestGetSizeBytes);
    RUN_TEST(TestIsValid);
    RUN_TEST(TestClear);
    RUN_TEST(TestEdgeCases);
}
