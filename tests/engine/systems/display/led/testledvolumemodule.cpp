// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testledvolumemodule.cpp
 * @brief Tests for LEDVolumeModule test patterns and static helpers.
 *
 * These tests validate the module's FillTestPattern utility and the
 * ModuleDesc structure without requiring a running kernel. The module's
 * lifecycle (Init/Tick/Shutdown) depends on the kernel and is integration
 * tested separately.
 *
 * @date 04/06/2026
 * @author Coela Can't
 */

#include "testledvolumemodule.hpp"
#include <unity.h>

#ifdef KL_HAVE_LED_VOLUME

#include <koilo/systems/display/led/module/led_volume_module.hpp>
#include <cstring>

using namespace koilo;

// -- Test pattern tests -------------------------------------------------------

static void TestFillPatternRed() {
    const uint16_t count = 5;
    uint8_t buf[15] = {0};
    LEDVolumeModule::FillTestPattern("red", buf, count);

    for (uint16_t i = 0; i < count; ++i) {
        TEST_ASSERT_EQUAL_UINT8(255, buf[i * 3 + 0]);
        TEST_ASSERT_EQUAL_UINT8(0,   buf[i * 3 + 1]);
        TEST_ASSERT_EQUAL_UINT8(0,   buf[i * 3 + 2]);
    }
}

static void TestFillPatternGreen() {
    const uint16_t count = 5;
    uint8_t buf[15] = {0};
    LEDVolumeModule::FillTestPattern("green", buf, count);

    for (uint16_t i = 0; i < count; ++i) {
        TEST_ASSERT_EQUAL_UINT8(0,   buf[i * 3 + 0]);
        TEST_ASSERT_EQUAL_UINT8(255, buf[i * 3 + 1]);
        TEST_ASSERT_EQUAL_UINT8(0,   buf[i * 3 + 2]);
    }
}

static void TestFillPatternBlue() {
    const uint16_t count = 5;
    uint8_t buf[15] = {0};
    LEDVolumeModule::FillTestPattern("blue", buf, count);

    for (uint16_t i = 0; i < count; ++i) {
        TEST_ASSERT_EQUAL_UINT8(0,   buf[i * 3 + 0]);
        TEST_ASSERT_EQUAL_UINT8(0,   buf[i * 3 + 1]);
        TEST_ASSERT_EQUAL_UINT8(255, buf[i * 3 + 2]);
    }
}

static void TestFillPatternWhite() {
    const uint16_t count = 3;
    uint8_t buf[9] = {0};
    LEDVolumeModule::FillTestPattern("white", buf, count);

    for (size_t i = 0; i < 9; ++i) {
        TEST_ASSERT_EQUAL_UINT8(255, buf[i]);
    }
}

static void TestFillPatternGradient() {
    const uint16_t count = 256;
    uint8_t buf[768] = {0};
    LEDVolumeModule::FillTestPattern("gradient", buf, count);

    // First LED should be 0, last should be 255
    TEST_ASSERT_EQUAL_UINT8(0,   buf[0]);
    TEST_ASSERT_EQUAL_UINT8(255, buf[(count - 1) * 3]);

    // Each LED should have R=G=B
    for (uint16_t i = 0; i < count; ++i) {
        TEST_ASSERT_EQUAL_UINT8(buf[i * 3], buf[i * 3 + 1]);
        TEST_ASSERT_EQUAL_UINT8(buf[i * 3], buf[i * 3 + 2]);
    }
}

static void TestFillPatternUnknownIsBlack() {
    const uint16_t count = 3;
    uint8_t buf[9];
    std::memset(buf, 0xFF, sizeof(buf));
    LEDVolumeModule::FillTestPattern("unknown_pattern", buf, count);

    for (size_t i = 0; i < 9; ++i) {
        TEST_ASSERT_EQUAL_UINT8(0, buf[i]);
    }
}

static void TestModuleDescFields() {
    const ModuleDesc& desc = LEDVolumeModule::GetModuleDesc();
    TEST_ASSERT_NOT_NULL(desc.name);
    TEST_ASSERT_EQUAL_STRING("koilo.led_volume", desc.name);
    TEST_ASSERT_NOT_NULL(reinterpret_cast<const void*>(desc.Init));
    TEST_ASSERT_NOT_NULL(reinterpret_cast<const void*>(desc.Tick));
    TEST_ASSERT_NOT_NULL(reinterpret_cast<const void*>(desc.Shutdown));
}

static void TestFillPatternGradientSingleLED() {
    const uint16_t count = 1;
    uint8_t buf[3] = {0};
    LEDVolumeModule::FillTestPattern("gradient", buf, count);

    // Single LED gradient: val = (0 * 255) / 0 => division by zero guard
    // Implementation uses (ledCount > 1 ? ledCount - 1 : 1), so val = 0
    TEST_ASSERT_EQUAL_UINT8(0, buf[0]);
}

// -- Test Runner --------------------------------------------------------------

void TestLEDVolumeModule::RunAllTests() {
    RUN_TEST(TestFillPatternRed);
    RUN_TEST(TestFillPatternGreen);
    RUN_TEST(TestFillPatternBlue);
    RUN_TEST(TestFillPatternWhite);
    RUN_TEST(TestFillPatternGradient);
    RUN_TEST(TestFillPatternUnknownIsBlack);
    RUN_TEST(TestModuleDescFields);
    RUN_TEST(TestFillPatternGradientSingleLED);
}

#else // !KL_HAVE_LED_VOLUME

void TestLEDVolumeModule::RunAllTests() {
    // LED volume feature not enabled; module tests are skipped.
}

#endif // KL_HAVE_LED_VOLUME
