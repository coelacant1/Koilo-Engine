// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file spi_led_transport.hpp
 * @brief Linux spidev-based LED transport implementation.
 *
 * Concrete ILEDTransport that sends frame data over a Linux SPI bus using
 * the kernel spidev interface (ioctl SPI_IOC_MESSAGE). Only compiled when
 * KL_HAVE_SPIDEV is defined; on other platforms the factory returns nullptr.
 *
 * This is the ONLY file in the engine that includes <linux/spi/spidev.h>.
 *
 * @date 04/06/2026
 * @author Coela Can't
 */

#pragma once

#ifdef KL_HAVE_SPIDEV

#include <koilo/systems/display/led/iled_transport.hpp>
#include <cstdint>
#include <string>

namespace koilo {

/**
 * @class SPILEDTransport
 * @brief ILEDTransport implementation using the Linux spidev kernel interface.
 *
 * Opens a /dev/spidevX.Y device node, configures clock speed and SPI mode,
 * then performs full-duplex ioctl transfers. Transfer statistics are tracked
 * for diagnostics.
 */
class SPILEDTransport : public ILEDTransport {
public:
    SPILEDTransport();
    ~SPILEDTransport() override;

    SPILEDTransport(const SPILEDTransport&) = delete;
    SPILEDTransport& operator=(const SPILEDTransport&) = delete;

    // -- ILEDTransport -------------------------------------------------------

    /// @brief Open the spidev device and configure bus parameters.
    bool Initialize(const LEDTransportConfig& config) override;

    /// @brief Close the spidev file descriptor.
    void Shutdown() override;

    /// @brief True when the device node is open.
    bool IsInitialized() const override;

    /// @brief Perform a blocking SPI transfer.
    bool Send(const uint8_t* data, size_t len) override;

    /// @brief Returns true when the device is open.
    bool IsReady() const override;

    /// @brief Return cumulative transfer statistics.
    LEDTransportStats GetStats() const override;

private:
    int fd_;                  ///< spidev file descriptor (-1 when closed).
    LEDTransportConfig config_;
    LEDTransportStats stats_;

    /// @brief Apply SPI mode, bits-per-word, and clock via ioctl.
    bool ConfigureBus();
};

} // namespace koilo

#endif // KL_HAVE_SPIDEV
