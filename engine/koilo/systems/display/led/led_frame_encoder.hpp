// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file led_frame_encoder.hpp
 * @brief Wire protocol encoder/decoder for LED frame data.
 *
 * Provides pure data-transform functions for building and parsing
 * the framed wire protocol used between the engine and an external
 * LED controller (e.g., Teensy 4.1 over SPI). No I/O or platform
 * headers -- fully testable on any host.
 *
 * Wire format (host -> controller):
 *   [SYNC 2B] [SEQ 1B] [SLOT 1B] [COUNT 2B] [PAYLOAD N*3B] [CRC16 2B]
 *
 * @date 04/06/2026
 * @author Coela Can't
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace koilo {

/**
 * @struct LEDAckResult
 * @brief Parsed acknowledgement from the controller.
 */
struct LEDAckResult {
    bool valid;      ///< True if the ACK was parsed successfully.
    bool accepted;   ///< True if the controller accepted the frame (ACK vs NAK).
    uint8_t slot;    ///< Buffer slot the controller reports as free.

    LEDAckResult() : valid(false), accepted(false), slot(0) {}
};

/**
 * @class LEDFrameEncoder
 * @brief Encodes and decodes the LED wire protocol.
 *
 * All methods are static and side-effect-free. The encoder has no
 * knowledge of how frames are transported -- it only transforms data.
 */
class LEDFrameEncoder {
public:
    static constexpr uint8_t kSyncByte0 = 0xAA;    ///< First sync byte.
    static constexpr uint8_t kSyncByte1 = 0x55;    ///< Second sync byte.
    static constexpr size_t kHeaderSize = 6;        ///< SYNC(2) + SEQ(1) + SLOT(1) + COUNT(2).
    static constexpr size_t kTrailerSize = 2;       ///< CRC16(2).
    static constexpr size_t kOverhead = kHeaderSize + kTrailerSize;
    static constexpr uint8_t kAckOk = 0x06;        ///< ACK byte (controller accepted frame).
    static constexpr uint8_t kAckNak = 0x15;       ///< NAK byte (CRC error or reject).
    static constexpr size_t kAckSize = 2;           ///< ACK(1) + SLOT(1).
    static constexpr uint16_t kMaxLEDCount = 8192;  ///< Protocol maximum LED count.

    /**
     * @brief Build a complete wire frame from raw RGB payload.
     * @param seq Rolling sequence number (0-255).
     * @param slot Target double-buffer slot (0 or 1).
     * @param rgb Packed RGB triplets in strip order.
     * @param ledCount Number of LEDs (rgb must be ledCount * 3 bytes).
     * @return Encoded frame including sync, header, payload, and CRC.
     *         Empty vector on invalid input.
     */
    static std::vector<uint8_t> BuildFrame(uint8_t seq,
                                           uint8_t slot,
                                           const uint8_t* rgb,
                                           uint16_t ledCount);

    /**
     * @brief Compute CRC-CCITT (0xFFFF initial) over a data buffer.
     * @param data Pointer to the data.
     * @param len Number of bytes.
     * @return 16-bit CRC value.
     */
    static uint16_t ComputeCRC16(const uint8_t* data, size_t len);

    /**
     * @brief Verify the CRC of a received frame.
     * @param frame Pointer to the full frame (including sync and CRC).
     * @param len Length of the frame in bytes.
     * @return True if the CRC matches.
     */
    static bool VerifyFrame(const uint8_t* frame, size_t len);

    /**
     * @brief Parse a controller acknowledgement.
     * @param data Pointer to the ACK data (2 bytes minimum).
     * @param len Length of available data.
     * @return Parsed ACK result (valid flag indicates parse success).
     */
    static LEDAckResult ParseAck(const uint8_t* data, size_t len);

    /**
     * @brief Calculate the total frame size for a given LED count.
     * @param ledCount Number of LEDs.
     * @return Total frame size in bytes (header + payload + CRC).
     */
    static size_t FrameSize(uint16_t ledCount);
};

} // namespace koilo
