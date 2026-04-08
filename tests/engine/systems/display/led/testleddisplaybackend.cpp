// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testleddisplaybackend.cpp
 * @brief Tests for LEDDisplayBackend.
 *
 * Uses MockLEDTransport to verify the backend's lifecycle, gamma
 * correction, frame encoding, slot alternation, and freeze behavior
 * without requiring any hardware.
 *
 * @date 04/06/2026
 * @author Coela Can't
 */

#include "testleddisplaybackend.hpp"
#include <unity.h>

#ifdef KL_HAVE_LED_VOLUME

#include <koilo/systems/display/backends/led/led_display_backend.hpp>
#include <koilo/platform/spi/mock_led_transport.hpp>
#include <koilo/systems/display/led/led_frame_encoder.hpp>
#include <cmath>
#include <cstring>

using namespace koilo;

// -- Helpers ------------------------------------------------------------------

static MockLEDTransport* MakeMock() {
    auto* mock = new MockLEDTransport();
    LEDTransportConfig cfg;
    mock->Initialize(cfg);
    return mock;
}

static LEDDisplayConfig MakeConfig(uint16_t leds = 10) {
    LEDDisplayConfig cfg;
    cfg.ledCount = leds;
    cfg.brightness = 255;
    cfg.gamma = 1.0f; // linear for predictable tests
    cfg.refreshRate = 60;
    cfg.name = "TestLED";
    return cfg;
}

static Framebuffer MakeFB(uint8_t* data, uint16_t leds) {
    return Framebuffer(data, leds, 1, PixelFormat::RGB888, leds * 3);
}

// -- Tests --------------------------------------------------------------------

static void TestInitRequiresTransport() {
    LEDDisplayConfig cfg = MakeConfig(10);
    LEDDisplayBackend backend(nullptr, cfg);
    TEST_ASSERT_FALSE(backend.Initialize());
    TEST_ASSERT_FALSE(backend.IsInitialized());
}

static void TestInitRequiresNonZeroLEDs() {
    auto* mock = MakeMock();
    LEDDisplayConfig cfg = MakeConfig(0);
    LEDDisplayBackend backend(mock, cfg);
    TEST_ASSERT_FALSE(backend.Initialize());
    delete mock;
}

static void TestInitSuccess() {
    auto* mock = MakeMock();
    LEDDisplayConfig cfg = MakeConfig(10);
    LEDDisplayBackend backend(mock, cfg);
    TEST_ASSERT_TRUE(backend.Initialize());
    TEST_ASSERT_TRUE(backend.IsInitialized());
    backend.Shutdown();
    TEST_ASSERT_FALSE(backend.IsInitialized());
    delete mock;
}

static void TestGetInfo() {
    auto* mock = MakeMock();
    LEDDisplayConfig cfg = MakeConfig(50);
    LEDDisplayBackend backend(mock, cfg);
    backend.Initialize();

    DisplayInfo info = backend.GetInfo();
    TEST_ASSERT_EQUAL_UINT32(50, info.width);
    TEST_ASSERT_EQUAL_UINT32(1, info.height);
    TEST_ASSERT_EQUAL_UINT32(60, info.refreshRate);
    TEST_ASSERT_TRUE(backend.HasCapability(DisplayCapability::RGB888));
    TEST_ASSERT_TRUE(backend.HasCapability(DisplayCapability::DoubleBuffering));
    TEST_ASSERT_FALSE(backend.HasCapability(DisplayCapability::TouchInput));

    backend.Shutdown();
    delete mock;
}

static void TestPresentSendsFrame() {
    auto* mock = MakeMock();
    LEDDisplayConfig cfg = MakeConfig(3);
    LEDDisplayBackend backend(mock, cfg);
    backend.Initialize();

    uint8_t rgb[9] = {255, 0, 0,  0, 255, 0,  0, 0, 255};
    Framebuffer fb = MakeFB(rgb, 3);
    TEST_ASSERT_TRUE(backend.Present(fb));

    TEST_ASSERT_EQUAL_UINT64(1, backend.GetFrameCount());
    TEST_ASSERT_EQUAL_UINT64(0, backend.GetDroppedFrames());

    // Verify mock received data
    TEST_ASSERT_EQUAL(1, static_cast<int>(mock->GetSendCount()));

    // Verify the sent data is a valid wire frame
    const auto& frame = mock->GetSentBuffer(0);
    TEST_ASSERT_TRUE(LEDFrameEncoder::VerifyFrame(frame.data(), frame.size()));

    backend.Shutdown();
    delete mock;
}

static void TestPresentSlotAlternation() {
    auto* mock = MakeMock();
    LEDDisplayConfig cfg = MakeConfig(3);
    LEDDisplayBackend backend(mock, cfg);
    backend.Initialize();

    uint8_t rgb[9] = {0};
    Framebuffer fb = MakeFB(rgb, 3);

    backend.Present(fb); // slot 0
    backend.Present(fb); // slot 1
    backend.Present(fb); // slot 0

    TEST_ASSERT_EQUAL_UINT64(3, backend.GetFrameCount());

    // Check slot bytes in wire frames (offset 3 = slot byte)
    TEST_ASSERT_EQUAL(3, static_cast<int>(mock->GetSendCount()));
    TEST_ASSERT_EQUAL_UINT8(0, mock->GetSentBuffer(0)[3]); // first frame slot 0
    TEST_ASSERT_EQUAL_UINT8(1, mock->GetSentBuffer(1)[3]); // second frame slot 1
    TEST_ASSERT_EQUAL_UINT8(0, mock->GetSentBuffer(2)[3]); // third frame slot 0

    backend.Shutdown();
    delete mock;
}

static void TestPresentSequenceIncrement() {
    auto* mock = MakeMock();
    LEDDisplayConfig cfg = MakeConfig(3);
    LEDDisplayBackend backend(mock, cfg);
    backend.Initialize();

    uint8_t rgb[9] = {0};
    Framebuffer fb = MakeFB(rgb, 3);

    backend.Present(fb);
    backend.Present(fb);
    backend.Present(fb);

    // SEQ byte is at offset 2
    TEST_ASSERT_EQUAL_UINT8(0, mock->GetSentBuffer(0)[2]);
    TEST_ASSERT_EQUAL_UINT8(1, mock->GetSentBuffer(1)[2]);
    TEST_ASSERT_EQUAL_UINT8(2, mock->GetSentBuffer(2)[2]);

    backend.Shutdown();
    delete mock;
}

static void TestPresentDropsWhenNotReady() {
    auto* mock = MakeMock();
    LEDDisplayConfig cfg = MakeConfig(3);
    LEDDisplayBackend backend(mock, cfg);
    backend.Initialize();

    mock->SetReady(false);

    uint8_t rgb[9] = {0};
    Framebuffer fb = MakeFB(rgb, 3);
    TEST_ASSERT_FALSE(backend.Present(fb));
    TEST_ASSERT_EQUAL_UINT64(0, backend.GetFrameCount());
    TEST_ASSERT_EQUAL_UINT64(1, backend.GetDroppedFrames());

    backend.Shutdown();
    delete mock;
}

static void TestPresentFrozenReturnsTrue() {
    auto* mock = MakeMock();
    LEDDisplayConfig cfg = MakeConfig(3);
    LEDDisplayBackend backend(mock, cfg);
    backend.Initialize();

    backend.SetFrozen(true);
    TEST_ASSERT_TRUE(backend.IsFrozen());

    uint8_t rgb[9] = {0};
    Framebuffer fb = MakeFB(rgb, 3);
    TEST_ASSERT_TRUE(backend.Present(fb));
    TEST_ASSERT_EQUAL_UINT64(0, backend.GetFrameCount()); // no frame sent

    backend.SetFrozen(false);
    TEST_ASSERT_FALSE(backend.IsFrozen());

    backend.Shutdown();
    delete mock;
}

static void TestPresentRejectsSmallBuffer() {
    auto* mock = MakeMock();
    LEDDisplayConfig cfg = MakeConfig(10);
    LEDDisplayBackend backend(mock, cfg);
    backend.Initialize();

    uint8_t rgb[9] = {0}; // only 3 LEDs worth, need 10
    Framebuffer fb = MakeFB(rgb, 3);
    TEST_ASSERT_FALSE(backend.Present(fb));

    backend.Shutdown();
    delete mock;
}

static void TestGammaCorrectionLinear() {
    auto* mock = MakeMock();
    LEDDisplayConfig cfg = MakeConfig(1);
    cfg.gamma = 1.0f;
    cfg.brightness = 255;
    LEDDisplayBackend backend(mock, cfg);
    backend.Initialize();

    uint8_t rgb[3] = {128, 64, 200};
    Framebuffer fb = MakeFB(rgb, 1);
    backend.Present(fb);

    // With gamma=1.0, brightness=255, output should be near identical to input
    const auto& frame = mock->GetSentBuffer(0);
    // Payload starts at offset kHeaderSize (6)
    TEST_ASSERT_EQUAL_UINT8(128, frame[6]);
    TEST_ASSERT_EQUAL_UINT8(64,  frame[7]);
    TEST_ASSERT_EQUAL_UINT8(200, frame[8]);

    backend.Shutdown();
    delete mock;
}

static void TestGammaCorrectionNonLinear() {
    auto* mock = MakeMock();
    LEDDisplayConfig cfg = MakeConfig(1);
    cfg.gamma = 2.2f;
    cfg.brightness = 255;
    LEDDisplayBackend backend(mock, cfg);
    backend.Initialize();

    uint8_t rgb[3] = {128, 128, 128};
    Framebuffer fb = MakeFB(rgb, 1);
    backend.Present(fb);

    // With gamma=2.2, 128/255 -> pow(0.502, 2.2) * 255 ~ 55
    const auto& frame = mock->GetSentBuffer(0);
    uint8_t corrected = frame[6];
    float expected = std::pow(128.0f / 255.0f, 2.2f) * 255.0f;
    int expectedInt = static_cast<int>(expected + 0.5f);
    TEST_ASSERT_INT_WITHIN(2, expectedInt, corrected);

    backend.Shutdown();
    delete mock;
}

static void TestBrightnessScaling() {
    auto* mock = MakeMock();
    LEDDisplayConfig cfg = MakeConfig(1);
    cfg.gamma = 1.0f;
    cfg.brightness = 128; // ~50%
    LEDDisplayBackend backend(mock, cfg);
    backend.Initialize();

    uint8_t rgb[3] = {255, 255, 255};
    Framebuffer fb = MakeFB(rgb, 1);
    backend.Present(fb);

    // gamma=1, brightness=128/255 -> output ~ 128
    const auto& frame = mock->GetSentBuffer(0);
    uint8_t val = frame[6];
    // Allow small rounding variance
    TEST_ASSERT_INT_WITHIN(2, 128, val);

    backend.Shutdown();
    delete mock;
}

static void TestSetBrightnessRebuildsLUT() {
    auto* mock = MakeMock();
    LEDDisplayConfig cfg = MakeConfig(1);
    cfg.gamma = 1.0f;
    cfg.brightness = 255;
    LEDDisplayBackend backend(mock, cfg);
    backend.Initialize();

    // Present at full brightness
    uint8_t rgb[3] = {200, 200, 200};
    Framebuffer fb = MakeFB(rgb, 1);
    backend.Present(fb);

    // Change to half brightness
    backend.SetBrightness(128);
    backend.Present(fb);

    uint8_t fullBright = mock->GetSentBuffer(0)[6];
    uint8_t halfBright = mock->GetSentBuffer(1)[6];
    TEST_ASSERT_TRUE(halfBright < fullBright);

    backend.Shutdown();
    delete mock;
}

static void TestSetGammaRebuildsLUT() {
    auto* mock = MakeMock();
    LEDDisplayConfig cfg = MakeConfig(1);
    cfg.gamma = 1.0f;
    cfg.brightness = 255;
    LEDDisplayBackend backend(mock, cfg);
    backend.Initialize();

    uint8_t rgb[3] = {128, 128, 128};
    Framebuffer fb = MakeFB(rgb, 1);
    backend.Present(fb);

    backend.SetGamma(2.2f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 2.2f, backend.GetGamma());
    backend.Present(fb);

    uint8_t linear = mock->GetSentBuffer(0)[6];
    uint8_t gamma22 = mock->GetSentBuffer(1)[6];
    TEST_ASSERT_TRUE(gamma22 < linear);

    backend.Shutdown();
    delete mock;
}

static void TestClearSendsBlackFrame() {
    auto* mock = MakeMock();
    LEDDisplayConfig cfg = MakeConfig(3);
    LEDDisplayBackend backend(mock, cfg);
    backend.Initialize();

    TEST_ASSERT_TRUE(backend.Clear());
    TEST_ASSERT_EQUAL_UINT64(1, backend.GetFrameCount());

    // Verify payload is all zeros
    const auto& frame = mock->GetSentBuffer(0);
    for (size_t i = LEDFrameEncoder::kHeaderSize;
         i < frame.size() - LEDFrameEncoder::kTrailerSize; ++i) {
        TEST_ASSERT_EQUAL_UINT8(0, frame[i]);
    }

    backend.Shutdown();
    delete mock;
}

static void TestSetRefreshRate() {
    auto* mock = MakeMock();
    LEDDisplayConfig cfg = MakeConfig(10);
    LEDDisplayBackend backend(mock, cfg);
    backend.Initialize();

    TEST_ASSERT_TRUE(backend.SetRefreshRate(120));
    TEST_ASSERT_EQUAL_UINT32(120, backend.GetInfo().refreshRate);

    backend.Shutdown();
    delete mock;
}

static void TestOrientationNotSupported() {
    auto* mock = MakeMock();
    LEDDisplayConfig cfg = MakeConfig(10);
    LEDDisplayBackend backend(mock, cfg);
    backend.Initialize();

    TEST_ASSERT_FALSE(backend.SetOrientation(Orientation::Landscape_90));

    backend.Shutdown();
    delete mock;
}

static void TestGetLEDCount() {
    auto* mock = MakeMock();
    LEDDisplayConfig cfg = MakeConfig(42);
    LEDDisplayBackend backend(mock, cfg);
    backend.Initialize();

    TEST_ASSERT_EQUAL_UINT16(42, backend.GetLEDCount());

    backend.Shutdown();
    delete mock;
}

static void TestSendFailureDoesNotIncrementFrameCount() {
    auto* mock = MakeMock();
    LEDDisplayConfig cfg = MakeConfig(3);
    LEDDisplayBackend backend(mock, cfg);
    backend.Initialize();

    mock->SetFailNextSend(true);

    uint8_t rgb[9] = {0};
    Framebuffer fb = MakeFB(rgb, 3);
    TEST_ASSERT_FALSE(backend.Present(fb));
    TEST_ASSERT_EQUAL_UINT64(0, backend.GetFrameCount());

    backend.Shutdown();
    delete mock;
}

// -- Test Runner --------------------------------------------------------------

void TestLEDDisplayBackend::RunAllTests() {
    RUN_TEST(TestInitRequiresTransport);
    RUN_TEST(TestInitRequiresNonZeroLEDs);
    RUN_TEST(TestInitSuccess);
    RUN_TEST(TestGetInfo);
    RUN_TEST(TestPresentSendsFrame);
    RUN_TEST(TestPresentSlotAlternation);
    RUN_TEST(TestPresentSequenceIncrement);
    RUN_TEST(TestPresentDropsWhenNotReady);
    RUN_TEST(TestPresentFrozenReturnsTrue);
    RUN_TEST(TestPresentRejectsSmallBuffer);
    RUN_TEST(TestGammaCorrectionLinear);
    RUN_TEST(TestGammaCorrectionNonLinear);
    RUN_TEST(TestBrightnessScaling);
    RUN_TEST(TestSetBrightnessRebuildsLUT);
    RUN_TEST(TestSetGammaRebuildsLUT);
    RUN_TEST(TestClearSendsBlackFrame);
    RUN_TEST(TestSetRefreshRate);
    RUN_TEST(TestOrientationNotSupported);
    RUN_TEST(TestGetLEDCount);
    RUN_TEST(TestSendFailureDoesNotIncrementFrameCount);
}

#else // !KL_HAVE_LED_VOLUME

void TestLEDDisplayBackend::RunAllTests() {
    // LED volume feature not enabled; backend tests are skipped.
}

#endif // KL_HAVE_LED_VOLUME
