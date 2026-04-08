// SPDX-License-Identifier: GPL-3.0-or-later
#include <koilo/systems/display/led/led_frame_encoder.hpp>

#include <cstring>

namespace koilo {

std::vector<uint8_t> LEDFrameEncoder::BuildFrame(uint8_t seq,
                                                  uint8_t slot,
                                                  const uint8_t* rgb,
                                                  uint16_t ledCount) {
    if (!rgb || ledCount == 0 || ledCount > kMaxLEDCount) {
        return {};
    }

    const size_t payloadSize = static_cast<size_t>(ledCount) * 3;
    const size_t totalSize = kOverhead + payloadSize;

    std::vector<uint8_t> frame;
    frame.reserve(totalSize);

    // Sync bytes
    frame.push_back(kSyncByte0);
    frame.push_back(kSyncByte1);

    // Header: SEQ, SLOT, COUNT (little-endian)
    frame.push_back(seq);
    frame.push_back(slot);
    frame.push_back(static_cast<uint8_t>(ledCount & 0xFF));
    frame.push_back(static_cast<uint8_t>((ledCount >> 8) & 0xFF));

    // Payload
    frame.insert(frame.end(), rgb, rgb + payloadSize);

    // CRC over SEQ + SLOT + COUNT + PAYLOAD (skip sync bytes)
    const uint16_t crc = ComputeCRC16(frame.data() + 2, frame.size() - 2);
    frame.push_back(static_cast<uint8_t>(crc & 0xFF));
    frame.push_back(static_cast<uint8_t>((crc >> 8) & 0xFF));

    return frame;
}

uint16_t LEDFrameEncoder::ComputeCRC16(const uint8_t* data, size_t len) {
    uint16_t crc = 0xFFFF;

    for (size_t i = 0; i < len; ++i) {
        crc ^= static_cast<uint16_t>(data[i]);
        for (int bit = 0; bit < 8; ++bit) {
            if (crc & 1) {
                crc = (crc >> 1) ^ 0x8408; // CCITT reflected polynomial
            } else {
                crc >>= 1;
            }
        }
    }

    return crc;
}

bool LEDFrameEncoder::VerifyFrame(const uint8_t* frame, size_t len) {
    if (!frame || len < kOverhead) {
        return false;
    }

    // Check sync bytes
    if (frame[0] != kSyncByte0 || frame[1] != kSyncByte1) {
        return false;
    }

    // Extract LED count from header
    const uint16_t ledCount = static_cast<uint16_t>(frame[4])
                            | (static_cast<uint16_t>(frame[5]) << 8);

    // Verify frame length matches
    if (len != FrameSize(ledCount)) {
        return false;
    }

    // Compute CRC over SEQ+SLOT+COUNT+PAYLOAD
    const size_t crcRegionLen = len - 2 - 2; // exclude sync(2) and trailing CRC(2)
    const uint16_t computed = ComputeCRC16(frame + 2, crcRegionLen);

    // Extract stored CRC (little-endian, at end of frame)
    const uint16_t stored = static_cast<uint16_t>(frame[len - 2])
                          | (static_cast<uint16_t>(frame[len - 1]) << 8);

    return computed == stored;
}

LEDAckResult LEDFrameEncoder::ParseAck(const uint8_t* data, size_t len) {
    LEDAckResult result;

    if (!data || len < kAckSize) {
        return result;
    }

    result.valid = (data[0] == kAckOk || data[0] == kAckNak);
    result.accepted = (data[0] == kAckOk);
    result.slot = data[1];

    return result;
}

size_t LEDFrameEncoder::FrameSize(uint16_t ledCount) {
    return kOverhead + static_cast<size_t>(ledCount) * 3;
}

} // namespace koilo
