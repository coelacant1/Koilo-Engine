// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file led_transport_factory.cpp
 * @brief Platform factory for LED transport implementations.
 *
 * Selects between USB serial (CDC ACM) and SPI transports based on the
 * requested transport type. USB is the default for Teensy 4.1 HUB75 setups
 * (higher throughput, no DMA conflicts). SPI remains available as fallback.
 *
 * @date 04/07/2026
 * @author Coela Can't
 */

#include <koilo/platform/spi/led_transport_factory.hpp>

#ifdef KL_HAVE_USB_SERIAL
#include <koilo/platform/usb/usb_led_transport.hpp>
#endif

#ifdef KL_HAVE_SPIDEV
#include <koilo/platform/spi/spi_led_transport.hpp>
#endif

namespace koilo {

std::unique_ptr<ILEDTransport> CreatePlatformLEDTransport(
    const std::string& transportType) {

#ifdef KL_HAVE_USB_SERIAL
    if (transportType == "usb" || transportType.empty()) {
        return std::make_unique<USBLEDTransport>();
    }
#endif

#ifdef KL_HAVE_SPIDEV
    if (transportType == "spi" || transportType.empty()) {
        return std::make_unique<SPILEDTransport>();
    }
#endif

    (void)transportType;
    return nullptr;
}

} // namespace koilo
