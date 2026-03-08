// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testdisplayconfig.cpp
 * @brief Implementation of DisplayConfig unit tests.
 */

#include "testdisplayconfig.hpp"

using namespace koilo;
using namespace koilo::scripting;

// ========== Constructor Tests ==========

void TestDisplayConfig::TestDefaultConstructor() {
    DisplayConfig obj;
    TEST_ASSERT_EQUAL_STRING("desktop", obj.GetType().c_str());
    TEST_ASSERT_EQUAL(1280, obj.GetWidth());
    TEST_ASSERT_EQUAL(720, obj.GetHeight());
    TEST_ASSERT_EQUAL(192, obj.GetPixelWidth());
    TEST_ASSERT_EQUAL(94, obj.GetPixelHeight());
    TEST_ASSERT_EQUAL(50, obj.GetBrightness());
    TEST_ASSERT_EQUAL(60, obj.GetTargetFPS());
}

void TestDisplayConfig::TestParameterizedConstructor() {
    // DisplayConfig only has a default constructor; verify it works
    DisplayConfig obj;
    obj.SetType("embedded");
    obj.SetWidth(320);
    obj.SetHeight(240);
    TEST_ASSERT_EQUAL_STRING("embedded", obj.GetType().c_str());
    TEST_ASSERT_EQUAL(320, obj.GetWidth());
    TEST_ASSERT_EQUAL(240, obj.GetHeight());
}

// ========== Method Tests ==========

void TestDisplayConfig::TestSetType() {
    DisplayConfig obj;
    obj.SetType("embedded");
    TEST_ASSERT_EQUAL_STRING("embedded", obj.GetType().c_str());
}

void TestDisplayConfig::TestSetWidth() {
    DisplayConfig obj;
    obj.SetWidth(1920);
    TEST_ASSERT_EQUAL(1920, obj.GetWidth());
}

void TestDisplayConfig::TestSetHeight() {
    DisplayConfig obj;
    obj.SetHeight(1080);
    TEST_ASSERT_EQUAL(1080, obj.GetHeight());
}

void TestDisplayConfig::TestSetPixelWidth() {
    DisplayConfig obj;
    obj.SetPixelWidth(256);
    TEST_ASSERT_EQUAL(256, obj.GetPixelWidth());
}

void TestDisplayConfig::TestSetPixelHeight() {
    DisplayConfig obj;
    obj.SetPixelHeight(128);
    TEST_ASSERT_EQUAL(128, obj.GetPixelHeight());
}

void TestDisplayConfig::TestSetBrightness() {
    DisplayConfig obj;
    obj.SetBrightness(75);
    TEST_ASSERT_EQUAL(75, obj.GetBrightness());
}

void TestDisplayConfig::TestSetTargetFPS() {
    DisplayConfig obj;
    obj.SetTargetFPS(30);
    TEST_ASSERT_EQUAL(30, obj.GetTargetFPS());
}

void TestDisplayConfig::TestGetType() {
    DisplayConfig obj;
    TEST_ASSERT_EQUAL_STRING("desktop", obj.GetType().c_str());
}

void TestDisplayConfig::TestGetWidth() {
    DisplayConfig obj;
    TEST_ASSERT_EQUAL(1280, obj.GetWidth());
}

void TestDisplayConfig::TestGetHeight() {
    DisplayConfig obj;
    TEST_ASSERT_EQUAL(720, obj.GetHeight());
}

void TestDisplayConfig::TestGetPixelWidth() {
    DisplayConfig obj;
    TEST_ASSERT_EQUAL(192, obj.GetPixelWidth());
}

void TestDisplayConfig::TestGetPixelHeight() {
    DisplayConfig obj;
    TEST_ASSERT_EQUAL(94, obj.GetPixelHeight());
}

void TestDisplayConfig::TestGetBrightness() {
    DisplayConfig obj;
    TEST_ASSERT_EQUAL(50, obj.GetBrightness());
}

void TestDisplayConfig::TestGetTargetFPS() {
    DisplayConfig obj;
    TEST_ASSERT_EQUAL(60, obj.GetTargetFPS());
}

// ========== Edge Cases ==========

void TestDisplayConfig::TestEdgeCases() {
    // ToConfigMap with defaults
    DisplayConfig obj;
    auto cfg = obj.ToConfigMap();
    TEST_ASSERT_EQUAL_STRING("desktop", cfg["type"].c_str());
    TEST_ASSERT_EQUAL_STRING("1280", cfg["width"].c_str());
    TEST_ASSERT_EQUAL_STRING("720", cfg["height"].c_str());
    TEST_ASSERT_EQUAL_STRING("60", cfg["target_fps"].c_str());

    // Zero values should not appear in map
    DisplayConfig zero;
    zero.SetWidth(0);
    zero.SetHeight(0);
    auto cfg2 = zero.ToConfigMap();
    TEST_ASSERT_TRUE(cfg2.find("width") == cfg2.end());
    TEST_ASSERT_TRUE(cfg2.find("height") == cfg2.end());
}

// ========== Test Runner ==========

void TestDisplayConfig::RunAllTests() {
    RUN_TEST(TestDefaultConstructor);
    RUN_TEST(TestParameterizedConstructor);
    RUN_TEST(TestSetType);
    RUN_TEST(TestSetWidth);
    RUN_TEST(TestSetHeight);
    RUN_TEST(TestSetPixelWidth);
    RUN_TEST(TestSetPixelHeight);
    RUN_TEST(TestSetBrightness);
    RUN_TEST(TestSetTargetFPS);
    RUN_TEST(TestGetType);
    RUN_TEST(TestGetWidth);
    RUN_TEST(TestGetHeight);
    RUN_TEST(TestGetPixelWidth);
    RUN_TEST(TestGetPixelHeight);
    RUN_TEST(TestGetBrightness);
    RUN_TEST(TestGetTargetFPS);
    RUN_TEST(TestEdgeCases);
}
