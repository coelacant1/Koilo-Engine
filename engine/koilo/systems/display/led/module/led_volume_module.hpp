// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file led_volume_module.hpp
 * @brief Kernel module for LED volume output.
 *
 * Assembles the LED volume pipeline: reads a layout JSON, creates
 * VolumeCamera instances per pixel group, constructs a transport via
 * the platform factory, builds the LEDDisplayBackend, and registers
 * console commands and CVars for runtime control.
 *
 * This is the ONLY place where concrete LED types are assembled.
 * All other components operate through interfaces.
 *
 * Conditionally compiled behind KL_HAVE_LED_VOLUME.
 *
 * @date 04/06/2026
 * @author Coela Can't
 */

#pragma once

#ifdef KL_HAVE_LED_VOLUME

#include <koilo/kernel/module.hpp>
#include <koilo/systems/display/backends/led/led_display_backend.hpp>
#include <koilo/systems/display/led/iled_transport.hpp>
#include <koilo/systems/display/led/camera_layout.hpp>
#include <koilo/systems/scene/camera/volumecamera.hpp>
#include <koilo/systems/render/core/pixelgroup.hpp>
#include <memory>
#include <vector>
#include <string>

namespace koilo {

/**
 * @class LEDVolumeModule
 * @brief Kernel module that drives the LED volume output pipeline.
 *
 * Lifecycle:
 * 1. Init: Creates transport, display backend, registers console commands
 * 2. Tick: Monitors transport stats, handles freeze/test patterns
 * 3. Shutdown: Tears down backend and transport in reverse order
 *
 * Registered as "koilo.led_volume" with the kernel module system.
 */
class LEDVolumeModule {
public:
    LEDVolumeModule();
    ~LEDVolumeModule();

    LEDVolumeModule(const LEDVolumeModule&) = delete;
    LEDVolumeModule& operator=(const LEDVolumeModule&) = delete;

    /**
     * @brief Get the module descriptor for kernel registration.
     * @return Reference to the static ModuleDesc.
     */
    static const ModuleDesc& GetModuleDesc();

    /**
     * @brief Get the singleton instance (valid after Init).
     * @return Pointer to the module instance, or nullptr if not initialized.
     */
    static LEDVolumeModule* Instance() { return instance_.get(); }

    /**
     * @brief Get the LED display backend.
     * @return Pointer to the backend, or nullptr if not initialized.
     */
    LEDDisplayBackend* GetBackend() const { return backend_.get(); }

    /**
     * @brief Get the transport.
     * @return Pointer to the transport, or nullptr if not initialized.
     */
    ILEDTransport* GetTransport() const { return transport_.get(); }

    /**
     * @brief Get the LED count.
     * @return Total LED count from configuration.
     */
    uint16_t GetLEDCount() const;

    /**
     * @brief Fill a test pattern into the given buffer.
     * @param pattern Pattern name ("red", "green", "blue", "white", "gradient").
     * @param buffer Output RGB buffer (must be ledCount * 3 bytes).
     * @param ledCount Number of LEDs.
     */
    static void FillTestPattern(const std::string& pattern,
                                uint8_t* buffer, uint16_t ledCount);

private:
    static bool Init(KoiloKernel& kernel);
    static void Tick(float dt);
    static void OnMessage(const Message& msg);
    static void Shutdown();

    void RegisterCommands();

    static std::unique_ptr<LEDVolumeModule> instance_;

    KoiloKernel*                      kernel_;
    std::unique_ptr<ILEDTransport>    transport_;
    std::unique_ptr<LEDDisplayBackend> backend_;
    LEDCameraLayout                      cameraLayout_;
    std::unique_ptr<PixelGroup>       ledPixelGroup_;
    std::unique_ptr<VolumeCamera>     volumeCamera_;
    std::vector<uint8_t>              rgbBuffer_; ///< RGBA8->RGB888 conversion buffer
};

} // namespace koilo

#endif // KL_HAVE_LED_VOLUME
