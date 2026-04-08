// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testledframeencoder.cpp
 * @brief Tests for LEDFrameEncoder: frame building, CRC, verify, and ACK parsing.
 */
#include "testledframeencoder.hpp"
#include <koilo/systems/display/led/led_frame_encoder.hpp>

#include <cstring>
#include <vector>

using namespace koilo;

// -- Frame building --

static void test_BuildFrameProducesCorrectSize() {
    uint8_t rgb[9] = {255, 0, 0, 0, 255, 0, 0, 0, 255};
    auto frame = LEDFrameEncoder::BuildFrame(0, 0, rgb, 3);
    TEST_ASSERT_EQUAL_UINT32(LEDFrameEncoder::FrameSize(3), frame.size());
}

static void test_BuildFrameSyncBytes() {
    uint8_t rgb[3] = {1, 2, 3};
    auto frame = LEDFrameEncoder::BuildFrame(0, 0, rgb, 1);
    TEST_ASSERT_EQUAL_UINT8(0xAA, frame[0]);
    TEST_ASSERT_EQUAL_UINT8(0x55, frame[1]);
}

static void test_BuildFrameHeader() {
    uint8_t rgb[6] = {10, 20, 30, 40, 50, 60};
    auto frame = LEDFrameEncoder::BuildFrame(42, 1, rgb, 2);
    TEST_ASSERT_EQUAL_UINT8(42, frame[2]);  // SEQ
    TEST_ASSERT_EQUAL_UINT8(1, frame[3]);   // SLOT
    TEST_ASSERT_EQUAL_UINT8(2, frame[4]);   // COUNT low byte
    TEST_ASSERT_EQUAL_UINT8(0, frame[5]);   // COUNT high byte
}

static void test_BuildFramePayload() {
    uint8_t rgb[6] = {10, 20, 30, 40, 50, 60};
    auto frame = LEDFrameEncoder::BuildFrame(0, 0, rgb, 2);
    // Payload starts at offset 6
    TEST_ASSERT_EQUAL_UINT8(10, frame[6]);
    TEST_ASSERT_EQUAL_UINT8(20, frame[7]);
    TEST_ASSERT_EQUAL_UINT8(30, frame[8]);
    TEST_ASSERT_EQUAL_UINT8(40, frame[9]);
    TEST_ASSERT_EQUAL_UINT8(50, frame[10]);
    TEST_ASSERT_EQUAL_UINT8(60, frame[11]);
}

static void test_BuildFrameRejectsNull() {
    auto frame = LEDFrameEncoder::BuildFrame(0, 0, nullptr, 1);
    TEST_ASSERT_TRUE(frame.empty());
}

static void test_BuildFrameRejectsZeroCount() {
    uint8_t rgb[3] = {1, 2, 3};
    auto frame = LEDFrameEncoder::BuildFrame(0, 0, rgb, 0);
    TEST_ASSERT_TRUE(frame.empty());
}

static void test_BuildFrameRejectsOverMax() {
    uint8_t rgb[3] = {1, 2, 3};
    auto frame = LEDFrameEncoder::BuildFrame(0, 0, rgb, LEDFrameEncoder::kMaxLEDCount + 1);
    TEST_ASSERT_TRUE(frame.empty());
}

// -- CRC --

static void test_CRC16Deterministic() {
    uint8_t data[] = {1, 2, 3, 4, 5};
    uint16_t crc1 = LEDFrameEncoder::ComputeCRC16(data, 5);
    uint16_t crc2 = LEDFrameEncoder::ComputeCRC16(data, 5);
    TEST_ASSERT_EQUAL_UINT16(crc1, crc2);
}

static void test_CRC16DifferentData() {
    uint8_t a[] = {1, 2, 3};
    uint8_t b[] = {1, 2, 4};
    TEST_ASSERT_NOT_EQUAL(
        LEDFrameEncoder::ComputeCRC16(a, 3),
        LEDFrameEncoder::ComputeCRC16(b, 3));
}

// -- Verify --

static void test_VerifyFrameRoundTrip() {
    uint8_t rgb[9] = {100, 150, 200, 50, 75, 100, 0, 0, 0};
    auto frame = LEDFrameEncoder::BuildFrame(7, 1, rgb, 3);
    TEST_ASSERT_TRUE(LEDFrameEncoder::VerifyFrame(frame.data(), frame.size()));
}

static void test_VerifyFrameDetectsCorruption() {
    uint8_t rgb[3] = {255, 128, 64};
    auto frame = LEDFrameEncoder::BuildFrame(0, 0, rgb, 1);
    // Corrupt one payload byte
    frame[6] ^= 0xFF;
    TEST_ASSERT_FALSE(LEDFrameEncoder::VerifyFrame(frame.data(), frame.size()));
}

static void test_VerifyFrameRejectsBadSync() {
    uint8_t rgb[3] = {1, 2, 3};
    auto frame = LEDFrameEncoder::BuildFrame(0, 0, rgb, 1);
    frame[0] = 0x00; // corrupt sync
    TEST_ASSERT_FALSE(LEDFrameEncoder::VerifyFrame(frame.data(), frame.size()));
}

static void test_VerifyFrameRejectsTruncated() {
    TEST_ASSERT_FALSE(LEDFrameEncoder::VerifyFrame(nullptr, 0));
    uint8_t tiny[4] = {0xAA, 0x55, 0, 0};
    TEST_ASSERT_FALSE(LEDFrameEncoder::VerifyFrame(tiny, 4));
}

static void test_VerifyFrameRejectsWrongLength() {
    uint8_t rgb[3] = {1, 2, 3};
    auto frame = LEDFrameEncoder::BuildFrame(0, 0, rgb, 1);
    // Pass wrong length (one byte too many)
    TEST_ASSERT_FALSE(LEDFrameEncoder::VerifyFrame(frame.data(), frame.size() + 1));
}

// -- ACK parsing --

static void test_ParseAckValid() {
    uint8_t ack[2] = {LEDFrameEncoder::kAckOk, 1};
    auto result = LEDFrameEncoder::ParseAck(ack, 2);
    TEST_ASSERT_TRUE(result.valid);
    TEST_ASSERT_TRUE(result.accepted);
    TEST_ASSERT_EQUAL_UINT8(1, result.slot);
}

static void test_ParseAckNak() {
    uint8_t ack[2] = {LEDFrameEncoder::kAckNak, 0};
    auto result = LEDFrameEncoder::ParseAck(ack, 2);
    TEST_ASSERT_TRUE(result.valid);
    TEST_ASSERT_FALSE(result.accepted);
    TEST_ASSERT_EQUAL_UINT8(0, result.slot);
}

static void test_ParseAckInvalidByte() {
    uint8_t ack[2] = {0x00, 0};
    auto result = LEDFrameEncoder::ParseAck(ack, 2);
    TEST_ASSERT_FALSE(result.valid);
}

static void test_ParseAckTooShort() {
    uint8_t ack[1] = {LEDFrameEncoder::kAckOk};
    auto result = LEDFrameEncoder::ParseAck(ack, 1);
    TEST_ASSERT_FALSE(result.valid);
}

// -- FrameSize --

static void test_FrameSizeCalculation() {
    TEST_ASSERT_EQUAL_UINT32(LEDFrameEncoder::kOverhead + 3,
                              LEDFrameEncoder::FrameSize(1));
    TEST_ASSERT_EQUAL_UINT32(LEDFrameEncoder::kOverhead + 15000,
                              LEDFrameEncoder::FrameSize(5000));
}

// -- Large frame round trip --

static void test_LargeFrameRoundTrip() {
    const uint16_t count = 1000;
    std::vector<uint8_t> rgb(count * 3);
    for (size_t i = 0; i < rgb.size(); ++i) {
        rgb[i] = static_cast<uint8_t>(i & 0xFF);
    }
    auto frame = LEDFrameEncoder::BuildFrame(200, 0, rgb.data(), count);
    TEST_ASSERT_FALSE(frame.empty());
    TEST_ASSERT_TRUE(LEDFrameEncoder::VerifyFrame(frame.data(), frame.size()));

    // Verify payload is preserved
    TEST_ASSERT_EQUAL_MEMORY(rgb.data(), frame.data() + 6, count * 3);
}

void TestLEDFrameEncoder::RunAllTests() {
    RUN_TEST(test_BuildFrameProducesCorrectSize);
    RUN_TEST(test_BuildFrameSyncBytes);
    RUN_TEST(test_BuildFrameHeader);
    RUN_TEST(test_BuildFramePayload);
    RUN_TEST(test_BuildFrameRejectsNull);
    RUN_TEST(test_BuildFrameRejectsZeroCount);
    RUN_TEST(test_BuildFrameRejectsOverMax);
    RUN_TEST(test_CRC16Deterministic);
    RUN_TEST(test_CRC16DifferentData);
    RUN_TEST(test_VerifyFrameRoundTrip);
    RUN_TEST(test_VerifyFrameDetectsCorruption);
    RUN_TEST(test_VerifyFrameRejectsBadSync);
    RUN_TEST(test_VerifyFrameRejectsTruncated);
    RUN_TEST(test_VerifyFrameRejectsWrongLength);
    RUN_TEST(test_ParseAckValid);
    RUN_TEST(test_ParseAckNak);
    RUN_TEST(test_ParseAckInvalidByte);
    RUN_TEST(test_ParseAckTooShort);
    RUN_TEST(test_FrameSizeCalculation);
    RUN_TEST(test_LargeFrameRoundTrip);
}
