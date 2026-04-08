// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testspitransport.cpp
 * @brief Tests for ILEDTransport contract (via MockLEDTransport),
 *        the transport factory, and the full encode-transport round-trip.
 *
 * The mock exercises the complete ILEDTransport interface contract.
 * The factory test verifies CreatePlatformLEDTransport() compiles and
 * returns the correct value for the current platform.
 *
 * @date 04/06/2026
 * @author Coela Can't
 */

#include "testspitransport.hpp"
#include <unity.h>
#include <koilo/platform/spi/mock_led_transport.hpp>
#include <koilo/platform/spi/led_transport_factory.hpp>
#include <koilo/systems/display/led/led_frame_encoder.hpp>
#include <cstring>

using namespace koilo;

// ---- Mock transport contract tests -----------------------------------------

static void test_MockInitializeAndShutdown() {
    MockLEDTransport mock;
    TEST_ASSERT_FALSE(mock.IsInitialized());

    LEDTransportConfig cfg;
    cfg.devicePath = "/dev/spidev0.0";
    cfg.clockHz = 16000000;
    TEST_ASSERT_TRUE(mock.Initialize(cfg));
    TEST_ASSERT_TRUE(mock.IsInitialized());
    TEST_ASSERT_EQUAL_STRING("/dev/spidev0.0", mock.GetConfig().devicePath.c_str());
    TEST_ASSERT_EQUAL_UINT32(16000000, mock.GetConfig().clockHz);

    mock.Shutdown();
    TEST_ASSERT_FALSE(mock.IsInitialized());
}

static void test_MockDoubleInitializeFails() {
    MockLEDTransport mock;
    LEDTransportConfig cfg;
    TEST_ASSERT_TRUE(mock.Initialize(cfg));
    TEST_ASSERT_FALSE(mock.Initialize(cfg));
}

static void test_MockSendRecordsData() {
    MockLEDTransport mock;
    LEDTransportConfig cfg;
    mock.Initialize(cfg);

    uint8_t data[] = {0xAA, 0x55, 0x01, 0x02, 0x03};
    TEST_ASSERT_TRUE(mock.Send(data, sizeof(data)));
    TEST_ASSERT_EQUAL(1, mock.GetSendCount());

    const auto& buf = mock.GetLastSentBuffer();
    TEST_ASSERT_EQUAL(sizeof(data), buf.size());
    TEST_ASSERT_EQUAL_UINT8(0xAA, buf[0]);
    TEST_ASSERT_EQUAL_UINT8(0x03, buf[4]);
}

static void test_MockSendFailsWhenNotInitialized() {
    MockLEDTransport mock;
    uint8_t data[] = {0x01};
    TEST_ASSERT_FALSE(mock.Send(data, 1));

    LEDTransportStats stats = mock.GetStats();
    TEST_ASSERT_EQUAL_UINT64(1, stats.errors);
    TEST_ASSERT_EQUAL_UINT64(0, stats.framesSent);
}

static void test_MockSendDropsWhenNotReady() {
    MockLEDTransport mock;
    LEDTransportConfig cfg;
    mock.Initialize(cfg);
    mock.SetReady(false);

    uint8_t data[] = {0x01};
    TEST_ASSERT_FALSE(mock.IsReady());
    TEST_ASSERT_FALSE(mock.Send(data, 1));

    LEDTransportStats stats = mock.GetStats();
    TEST_ASSERT_EQUAL_UINT64(1, stats.framesDropped);
    TEST_ASSERT_EQUAL_UINT64(0, stats.framesSent);
}

static void test_MockFailNextSend() {
    MockLEDTransport mock;
    LEDTransportConfig cfg;
    mock.Initialize(cfg);
    mock.SetFailNextSend(true);

    uint8_t data[] = {0x01, 0x02};
    TEST_ASSERT_FALSE(mock.Send(data, 2));

    LEDTransportStats stats = mock.GetStats();
    TEST_ASSERT_EQUAL_UINT64(1, stats.errors);

    // Next send should succeed (fail flag is one-shot).
    TEST_ASSERT_TRUE(mock.Send(data, 2));
    stats = mock.GetStats();
    TEST_ASSERT_EQUAL_UINT64(1, stats.framesSent);
}

static void test_MockStatsAccumulate() {
    MockLEDTransport mock;
    LEDTransportConfig cfg;
    mock.Initialize(cfg);

    uint8_t a[] = {0x01, 0x02, 0x03};
    uint8_t b[] = {0x04, 0x05};
    mock.Send(a, 3);
    mock.Send(b, 2);

    LEDTransportStats stats = mock.GetStats();
    TEST_ASSERT_EQUAL_UINT64(2, stats.framesSent);
    TEST_ASSERT_EQUAL_UINT64(5, stats.bytesTransferred);
    TEST_ASSERT_EQUAL_UINT64(0, stats.errors);
}

static void test_MockClearHistory() {
    MockLEDTransport mock;
    LEDTransportConfig cfg;
    mock.Initialize(cfg);

    uint8_t data[] = {0xFF};
    mock.Send(data, 1);
    TEST_ASSERT_EQUAL(1, mock.GetSendCount());

    mock.ClearHistory();
    TEST_ASSERT_EQUAL(0, mock.GetSendCount());
}

static void test_MockIsReadyWhenInitialized() {
    MockLEDTransport mock;
    TEST_ASSERT_FALSE(mock.IsReady());

    LEDTransportConfig cfg;
    mock.Initialize(cfg);
    TEST_ASSERT_TRUE(mock.IsReady());

    mock.SetReady(false);
    TEST_ASSERT_FALSE(mock.IsReady());

    mock.SetReady(true);
    TEST_ASSERT_TRUE(mock.IsReady());
}

static void test_MockShutdownClearsHistory() {
    MockLEDTransport mock;
    LEDTransportConfig cfg;
    mock.Initialize(cfg);

    uint8_t data[] = {0x42};
    mock.Send(data, 1);
    TEST_ASSERT_EQUAL(1, mock.GetSendCount());

    mock.Shutdown();
    TEST_ASSERT_EQUAL(0, mock.GetSendCount());
}

static void test_MockSendEmptyIsNoop() {
    MockLEDTransport mock;
    LEDTransportConfig cfg;
    mock.Initialize(cfg);

    // Sending nullptr with len 0 should succeed silently.
    // (Matches SPILEDTransport behavior.)
    uint8_t dummy = 0;
    TEST_ASSERT_TRUE(mock.Send(&dummy, 0));
}

// ---- Factory tests ---------------------------------------------------------

static void test_FactoryReturnsValue() {
    auto transport = CreatePlatformLEDTransport();
#ifdef KL_HAVE_SPIDEV
    // On Linux with spidev, factory should return a real transport.
    TEST_ASSERT_NOT_NULL(transport.get());
#else
    // On all other platforms (including this test build), nullptr is expected.
    TEST_ASSERT_NULL(transport.get());
#endif
}

// ---- Encode + transport round-trip -----------------------------------------

static void test_EncodeAndSendViaTransport() {
    MockLEDTransport mock;
    LEDTransportConfig cfg;
    mock.Initialize(cfg);

    // Encode a 3-LED frame.
    uint8_t rgb[9] = {
        255,   0,   0,  // red
          0, 255,   0,  // green
          0,   0, 255   // blue
    };
    auto frame = LEDFrameEncoder::BuildFrame(0, 0, rgb, 3);
    TEST_ASSERT_TRUE(frame.size() > 0);

    TEST_ASSERT_TRUE(mock.Send(frame.data(), frame.size()));
    TEST_ASSERT_EQUAL(1, mock.GetSendCount());

    const auto& sent = mock.GetLastSentBuffer();
    TEST_ASSERT_EQUAL(frame.size(), sent.size());
    TEST_ASSERT_EQUAL_MEMORY(frame.data(), sent.data(), frame.size());

    // Verify the frame is valid.
    TEST_ASSERT_TRUE(LEDFrameEncoder::VerifyFrame(sent.data(), sent.size()));
}

static void test_EncodeAndSendLargeFrame() {
    MockLEDTransport mock;
    LEDTransportConfig cfg;
    mock.Initialize(cfg);

    // 1000 LEDs (realistic strip size).
    const uint16_t count = 1000;
    std::vector<uint8_t> rgb(count * 3, 0x80);
    auto frame = LEDFrameEncoder::BuildFrame(42, 1, rgb.data(), count);
    TEST_ASSERT_TRUE(frame.size() > 0);

    TEST_ASSERT_TRUE(mock.Send(frame.data(), frame.size()));

    const auto& sent = mock.GetLastSentBuffer();
    TEST_ASSERT_TRUE(LEDFrameEncoder::VerifyFrame(sent.data(), sent.size()));

    LEDTransportStats stats = mock.GetStats();
    TEST_ASSERT_EQUAL_UINT64(1, stats.framesSent);
    TEST_ASSERT_EQUAL_UINT64(frame.size(), stats.bytesTransferred);
}

static void test_TransportDropsWhenBusy() {
    MockLEDTransport mock;
    LEDTransportConfig cfg;
    mock.Initialize(cfg);

    uint8_t rgb[3] = {0xFF, 0x00, 0x00};
    auto frame = LEDFrameEncoder::BuildFrame(0, 0, rgb, 1);

    // First send succeeds.
    TEST_ASSERT_TRUE(mock.Send(frame.data(), frame.size()));

    // Simulate busy transport.
    mock.SetReady(false);
    TEST_ASSERT_FALSE(mock.Send(frame.data(), frame.size()));

    LEDTransportStats stats = mock.GetStats();
    TEST_ASSERT_EQUAL_UINT64(1, stats.framesSent);
    TEST_ASSERT_EQUAL_UINT64(1, stats.framesDropped);
}

static void test_StatsReflectMultipleFrames() {
    MockLEDTransport mock;
    LEDTransportConfig cfg;
    mock.Initialize(cfg);

    uint8_t rgb[6] = {0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00};
    uint64_t totalBytes = 0;

    for (uint8_t seq = 0; seq < 10; ++seq) {
        auto frame = LEDFrameEncoder::BuildFrame(seq, seq % 2, rgb, 2);
        TEST_ASSERT_TRUE(mock.Send(frame.data(), frame.size()));
        totalBytes += frame.size();
    }

    LEDTransportStats stats = mock.GetStats();
    TEST_ASSERT_EQUAL_UINT64(10, stats.framesSent);
    TEST_ASSERT_EQUAL_UINT64(totalBytes, stats.bytesTransferred);
    TEST_ASSERT_EQUAL_UINT64(0, stats.errors);
    TEST_ASSERT_EQUAL_UINT64(0, stats.framesDropped);
}

// ---- Default config values -------------------------------------------------

static void test_DefaultConfigValues() {
    LEDTransportConfig cfg;
    TEST_ASSERT_EQUAL_STRING("/dev/spidev0.0", cfg.devicePath.c_str());
    TEST_ASSERT_EQUAL_UINT32(32000000, cfg.clockHz);
    TEST_ASSERT_EQUAL_UINT8(0, cfg.mode);
    TEST_ASSERT_EQUAL_UINT8(8, cfg.bitsPerWord);
}

static void test_DefaultStatsValues() {
    LEDTransportStats stats;
    TEST_ASSERT_EQUAL_UINT64(0, stats.framesSent);
    TEST_ASSERT_EQUAL_UINT64(0, stats.bytesTransferred);
    TEST_ASSERT_EQUAL_UINT64(0, stats.errors);
    TEST_ASSERT_EQUAL_UINT64(0, stats.framesDropped);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, stats.lastTransferMs);
}

// ---- RunAllTests -----------------------------------------------------------

void TestSPITransport::RunAllTests() {
    // Mock transport contract
    RUN_TEST(test_MockInitializeAndShutdown);
    RUN_TEST(test_MockDoubleInitializeFails);
    RUN_TEST(test_MockSendRecordsData);
    RUN_TEST(test_MockSendFailsWhenNotInitialized);
    RUN_TEST(test_MockSendDropsWhenNotReady);
    RUN_TEST(test_MockFailNextSend);
    RUN_TEST(test_MockStatsAccumulate);
    RUN_TEST(test_MockClearHistory);
    RUN_TEST(test_MockIsReadyWhenInitialized);
    RUN_TEST(test_MockShutdownClearsHistory);
    RUN_TEST(test_MockSendEmptyIsNoop);

    // Factory
    RUN_TEST(test_FactoryReturnsValue);

    // Encode + transport round-trip
    RUN_TEST(test_EncodeAndSendViaTransport);
    RUN_TEST(test_EncodeAndSendLargeFrame);
    RUN_TEST(test_TransportDropsWhenBusy);
    RUN_TEST(test_StatsReflectMultipleFrames);

    // Config/stats defaults
    RUN_TEST(test_DefaultConfigValues);
    RUN_TEST(test_DefaultStatsValues);
}
