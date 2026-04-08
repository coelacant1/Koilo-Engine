// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file iled_transport.hpp
 * @brief Abstract transport interface for LED frame data.
 *
 * Defines a transport-agnostic interface for sending packed LED frame data
 * to an external controller. Concrete implementations may use SPI, USB,
 * network sockets, or a mock for testing. No platform-specific headers
 * are included -- this is a pure virtual interface.
 *
 * @date 04/06/2026
 * @author Coela Can't
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

namespace koilo {

/**
 * @struct LEDTransportConfig
 * @brief Configuration for an LED transport link.
 */
struct LEDTransportConfig {
    std::string devicePath;   ///< Device path (e.g., "/dev/spidev0.0").
    uint32_t clockHz;         ///< Clock speed in Hz (e.g., 32000000).
    uint8_t mode;             ///< Bus mode (SPI: CPOL/CPHA bits).
    uint8_t bitsPerWord;      ///< Bits per transfer word (typically 8).

    LEDTransportConfig()
        : devicePath("/dev/spidev0.0"),
          clockHz(32000000),
          mode(0),
          bitsPerWord(8) {}
};

/**
 * @struct LEDTransportStats
 * @brief Runtime statistics for an LED transport link.
 */
struct LEDTransportStats {
    uint64_t framesSent;      ///< Total frames successfully sent.
    uint64_t bytesTransferred; ///< Total bytes transferred.
    uint64_t errors;          ///< Total transfer errors.
    uint64_t framesDropped;   ///< Frames dropped (transport busy).
    float lastTransferMs;     ///< Duration of last transfer in milliseconds.

    LEDTransportStats()
        : framesSent(0),
          bytesTransferred(0),
          errors(0),
          framesDropped(0),
          lastTransferMs(0.0f) {}
};

/**
 * @class ILEDTransport
 * @brief Abstract interface for sending LED frame data to an external controller.
 *
 * Implementations handle the physical transport (SPI, USB, etc.) while
 * consumers operate only through this interface. This keeps all
 * platform-specific code isolated in the concrete transport class.
 */
class ILEDTransport {
public:
    /**
     * @brief Virtual destructor.
     */
    virtual ~ILEDTransport() = default;

    /**
     * @brief Initialize the transport link.
     * @param config Transport configuration parameters.
     * @return True if the link was opened successfully.
     */
    virtual bool Initialize(const LEDTransportConfig& config) = 0;

    /**
     * @brief Shut down the transport link and release resources.
     */
    virtual void Shutdown() = 0;

    /**
     * @brief Check if the transport is initialized and ready.
     * @return True if ready to accept data.
     */
    virtual bool IsInitialized() const = 0;

    /**
     * @brief Send a block of data to the remote controller.
     * @param data Pointer to the data buffer.
     * @param len Number of bytes to send.
     * @return True if the data was accepted for transmission.
     */
    virtual bool Send(const uint8_t* data, size_t len) = 0;

    /**
     * @brief Check if the transport is ready to accept a new frame.
     * @return True if a call to Send() will not block or drop.
     *
     * When false, the caller should skip this frame rather than
     * blocking the render loop.
     */
    virtual bool IsReady() const = 0;

    /**
     * @brief Retrieve cumulative transport statistics.
     * @return Current statistics snapshot.
     */
    virtual LEDTransportStats GetStats() const = 0;
};

} // namespace koilo
