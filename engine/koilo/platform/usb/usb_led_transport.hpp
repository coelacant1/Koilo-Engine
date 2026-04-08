// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file usb_led_transport.hpp
 * @brief USB serial (CDC ACM) LED transport implementation.
 *
 * Concrete ILEDTransport that sends frame data to a Teensy 4.1 over a USB
 * CDC ACM link (/dev/ttyACMx). The Teensy runs USB_DUAL_SERIAL firmware:
 * one CDC interface for debug serial, another for LED frame data.
 *
 * Under the hood, USB CDC ACM uses bulk endpoints -- at USB High-Speed
 * (480 Mbps) this gives 10-20+ MB/s practical throughput, far exceeding
 * the ~1.5 MB/s needed for 4096 LEDs at 120 fps.
 *
 * Only compiled when KL_HAVE_USB_SERIAL is defined (UNIX systems).
 *
 * @date 04/07/2026
 * @author Coela Can't
 */

#pragma once

#ifdef KL_HAVE_USB_SERIAL

#include <koilo/systems/display/led/iled_transport.hpp>
#include <cstdint>
#include <string>

namespace koilo {

/**
 * @class USBLEDTransport
 * @brief ILEDTransport implementation using a USB serial (CDC ACM) link.
 *
 * Opens a /dev/ttyACMx device in raw mode (no line discipline, no echo,
 * no flow control). Transfer speed is governed by the USB bus, not the
 * baud rate -- the CDC ACM baud setting is ignored by the Teensy USB stack.
 *
 * The devicePath field from LEDTransportConfig selects the serial device.
 * All SPI-specific config fields (clockHz, mode, bitsPerWord) are ignored.
 */
class USBLEDTransport : public ILEDTransport {
public:
    USBLEDTransport();
    ~USBLEDTransport() override;

    USBLEDTransport(const USBLEDTransport&) = delete;
    USBLEDTransport& operator=(const USBLEDTransport&) = delete;

    /// @brief Open the USB serial device and configure raw mode.
    bool Initialize(const LEDTransportConfig& config) override;

    /// @brief Close the device file descriptor.
    void Shutdown() override;

    /// @brief True when the device is open.
    bool IsInitialized() const override;

    /// @brief Write a block of data to the USB serial device.
    bool Send(const uint8_t* data, size_t len) override;

    /// @brief Returns true when the device is open.
    bool IsReady() const override;

    /// @brief Return cumulative transfer statistics.
    LEDTransportStats GetStats() const override;

private:
    int fd_;                  ///< Serial device file descriptor (-1 when closed).
    LEDTransportConfig config_;
    LEDTransportStats stats_;

    /// @brief Configure termios for raw binary transfer.
    bool ConfigurePort();
};

} // namespace koilo

#endif // KL_HAVE_USB_SERIAL
