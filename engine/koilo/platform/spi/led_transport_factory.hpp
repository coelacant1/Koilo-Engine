// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file led_transport_factory.hpp
 * @brief Platform factory for LED transport implementations.
 *
 * Follows the IScriptFileReader / CreatePlatformFileReader() pattern.
 * Always available as a header; the factory returns nullptr on platforms
 * without a hardware transport.
 *
 * @date 04/06/2026
 * @author Coela Can't
 */

#pragma once

#include <koilo/systems/display/led/iled_transport.hpp>
#include <memory>
#include <string>

namespace koilo {

/**
 * @brief Create a platform-appropriate ILEDTransport implementation.
 * @param transportType Transport type: "usb" for USB CDC ACM (default),
 *        "spi" for Linux spidev. Empty string selects the best available.
 * @return A concrete transport, or nullptr if no hardware transport is
 *         available for the requested type on this platform.
 *
 * Callers must check for nullptr and handle the no-transport case
 * (e.g., fall back to a mock or skip hardware output).
 */
std::unique_ptr<ILEDTransport> CreatePlatformLEDTransport(
    const std::string& transportType = "usb");

} // namespace koilo
