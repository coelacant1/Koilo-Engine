// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file led_volume_module.cpp
 * @brief Implementation of the LED volume kernel module.
 *
 * @date 04/06/2026
 * @author Coela Can't
 */

#ifdef KL_HAVE_LED_VOLUME

#include <koilo/systems/display/led/module/led_volume_module.hpp>
#include <koilo/platform/spi/led_transport_factory.hpp>
#include <koilo/kernel/kernel.hpp>
#include <koilo/kernel/cvar/cvar_system.hpp>
#include <koilo/kernel/console/console_module.hpp>
#include <koilo/kernel/logging/log.hpp>
#include <koilo/systems/scene/camera/camerabase.hpp>
#include <koilo/systems/display/framebuffer.hpp>
#include <koilo/systems/render/pipeline/render_pipeline.hpp>
#include <koilo/systems/render/rhi/rhi_device.hpp>
#include <cstring>
#include <cstdio>
#include <sstream>
#include <iomanip>

namespace koilo {

// -- CVars (self-registering at static init) ----------------------------------

static AutoCVar_Int    cvar_ledBrightness("led.brightness",
    "Global LED brightness (0-255)", 255);

static AutoCVar_Float  cvar_ledGamma("led.gamma",
    "Gamma correction exponent", 2.2f);

static AutoCVar_Int    cvar_ledSpiClockHz("led.spi_clock_hz",
    "SPI clock speed in Hz", 32000000);

static AutoCVar_String cvar_ledSpiDevice("led.spi_device",
    "SPI device path", "/dev/spidev0.0");

static AutoCVar_String cvar_ledTransportType("led.transport_type",
    "LED transport type: usb or spi", "usb");

static AutoCVar_String cvar_ledUsbDevice("led.usb_device",
    "USB serial device path for LED data", "/dev/ttyACM1");

static AutoCVar_Int    cvar_ledCount("led.count",
    "Total number of LEDs", 300);

static AutoCVar_Int    cvar_ledFboWidth("led.fbo_width",
    "Volume camera FBO width", 128);

static AutoCVar_Int    cvar_ledFboHeight("led.fbo_height",
    "Volume camera FBO height", 128);

static AutoCVar_Bool   cvar_ledEnabled("led.enabled",
    "Enable LED volume output", true);

static AutoCVar_String cvar_ledCameraLayout("led.camera_layout",
    "Path to .klcam camera layout file", "");

static AutoCVar_Bool cvar_ledFlipX("led.flip_x",
    "Mirror LED X axis (for panels with reversed horizontal wiring)", false);

static AutoCVar_Bool cvar_ledFlipY("led.flip_y",
    "Mirror LED Y axis", false);

static AutoCVar_String cvar_ledFilter("led.filter",
    "LED pixel sampling filter: nearest or bilinear", "bilinear");

// -- Static instance ----------------------------------------------------------

std::unique_ptr<LEDVolumeModule> LEDVolumeModule::instance_;

// -- Lifecycle ----------------------------------------------------------------

LEDVolumeModule::LEDVolumeModule()
    : kernel_(nullptr) {}

LEDVolumeModule::~LEDVolumeModule() = default;

const ModuleDesc& LEDVolumeModule::GetModuleDesc() {
    static ModuleDesc desc{};
    desc.name         = "koilo.led_volume";
    desc.version      = KL_VERSION(0, 1, 0);
    desc.requiredCaps = Cap::None;
    desc.Init         = &LEDVolumeModule::Init;
    desc.Tick         = &LEDVolumeModule::Tick;
    desc.OnMessage    = &LEDVolumeModule::OnMessage;
    desc.Shutdown     = &LEDVolumeModule::Shutdown;
    return desc;
}

bool LEDVolumeModule::Init(KoiloKernel& kernel) {
    instance_ = std::make_unique<LEDVolumeModule>();
    instance_->kernel_ = &kernel;

    if (!cvar_ledEnabled.Get()) {
        KL_LOG("led_volume", "LED volume disabled via led.enabled CVar");
        return true;
    }

    // Load camera layout (determines LED count)
    std::string layoutPath = cvar_ledCameraLayout.Get();
    uint16_t ledCount = static_cast<uint16_t>(cvar_ledCount.Get());

    if (!layoutPath.empty()) {
        if (instance_->cameraLayout_.LoadFromFile(layoutPath)) {
            // Apply axis flips before creating pixel group
            if (cvar_ledFlipX.Get()) instance_->cameraLayout_.FlipX();
            if (cvar_ledFlipY.Get()) instance_->cameraLayout_.FlipY();

            instance_->ledPixelGroup_.reset(
                instance_->cameraLayout_.CreatePixelGroup());

            if (instance_->ledPixelGroup_) {
                // VolumeCamera with passthrough gamma/brightness
                // (backend applies its own post-processing)
                instance_->volumeCamera_ = std::make_unique<VolumeCamera>(
                    instance_->ledPixelGroup_.get());
                instance_->volumeCamera_->SetGamma(1.0f);
                instance_->volumeCamera_->SetBrightness(255);
                instance_->volumeCamera_->SetNearestFilter(
                    cvar_ledFilter.Get() == "nearest");

                uint32_t layoutCount = instance_->cameraLayout_.GetCount();
                if (layoutCount > 0 &&
                    layoutCount <= LEDFrameEncoder::kMaxLEDCount) {
                    ledCount = static_cast<uint16_t>(layoutCount);
                }
                KL_LOG("led_volume", "Loaded camera layout '%s' (%u LEDs)",
                       instance_->cameraLayout_.GetName().c_str(),
                       static_cast<unsigned>(ledCount));
            }
        } else {
            KL_WARN("led_volume",
                     "Failed to load camera layout: %s", layoutPath.c_str());
        }
    }

    // Create transport via platform factory (USB or SPI based on CVar)
    std::string transportType = cvar_ledTransportType.Get();
    instance_->transport_ = CreatePlatformLEDTransport(transportType);

    if (instance_->transport_) {
        LEDTransportConfig tcfg;
        // Select device path based on transport type
        if (transportType == "usb") {
            tcfg.devicePath = cvar_ledUsbDevice.Get();
        } else {
            tcfg.devicePath = cvar_ledSpiDevice.Get();
        }
        tcfg.clockHz     = static_cast<uint32_t>(cvar_ledSpiClockHz.Get());
        tcfg.mode        = 0;
        tcfg.bitsPerWord = 8;

        if (!instance_->transport_->Initialize(tcfg)) {
            KL_WARN("led_volume",
                     "%s transport init failed; LED output unavailable",
                     transportType.c_str());
            instance_->transport_.reset();
        }
    } else {
        KL_LOG("led_volume",
               "No platform LED transport available (mock or headless mode)");
    }

    // Build display backend
    LEDDisplayConfig dcfg;
    dcfg.ledCount   = ledCount;
    dcfg.brightness = static_cast<uint8_t>(cvar_ledBrightness.Get());
    dcfg.gamma      = cvar_ledGamma.Get();
    dcfg.refreshRate = 60;
    dcfg.name       = "LEDVolume";

    instance_->backend_ = std::make_unique<LEDDisplayBackend>(
        instance_->transport_.get(), dcfg);

    if (!instance_->backend_->Initialize()) {
        KL_WARN("led_volume", "LED display backend init failed");
        instance_->backend_.reset();
    }

    // Register console commands
    instance_->RegisterCommands();

    kernel.Services().Register("led_volume", instance_.get());

    KL_LOG("led_volume", "LED volume module initialized (%d LEDs%s)",
           static_cast<int>(ledCount),
           instance_->volumeCamera_ ? ", camera layout active" : "");
    return true;
}

void LEDVolumeModule::Tick(float /*dt*/) {
    if (!instance_ || !instance_->backend_ || !instance_->volumeCamera_) {
        return;
    }

    if (!cvar_ledEnabled.Get()) return;

    static uint32_t tickCount = 0;
    ++tickCount;

    // Get render pipeline via kernel services (registered by SDL3Host)
    auto* pipeline = static_cast<rhi::RenderPipeline*>(
        instance_->kernel_->Services().Get<void>("render_backend"));
    if (!pipeline) return;

    auto* device = pipeline->GetDevice();
    if (!device) return;

    // Read the previous frame's rendered pixels (RGBA8 format)
    const uint8_t* swPixels = device->GetSwapchainPixels();
    if (!swPixels) return;

    uint32_t fbWidth = 0, fbHeight = 0;
    device->GetSwapchainSize(fbWidth, fbHeight);
    if (fbWidth == 0 || fbHeight == 0) return;

    // Convert RGBA8 swapchain to RGB888 for VolumeCamera sampling
    // (reuse a persistent buffer to avoid per-frame allocation)
    size_t rgbSize = static_cast<size_t>(fbWidth) * fbHeight * 3;
    instance_->rgbBuffer_.resize(rgbSize);

    const size_t pixelCount = static_cast<size_t>(fbWidth) * fbHeight;
    for (size_t i = 0; i < pixelCount; ++i) {
        instance_->rgbBuffer_[i * 3 + 0] = swPixels[i * 4 + 0];
        instance_->rgbBuffer_[i * 3 + 1] = swPixels[i * 4 + 1];
        instance_->rgbBuffer_[i * 3 + 2] = swPixels[i * 4 + 2];
    }

    if (tickCount <= 5 || tickCount % 300 == 0) {
        size_t cidx = (fbHeight / 2 * fbWidth + fbWidth / 2) * 3;
        uint64_t sum = 0;
        for (size_t i = 0; i < rgbSize; ++i) sum += instance_->rgbBuffer_[i];
        std::fprintf(stderr,
            "[led_diag] tick=%u sw=%ux%u center=(%u,%u,%u) sum=%lu\n",
            tickCount, fbWidth, fbHeight,
            instance_->rgbBuffer_[cidx],
            instance_->rgbBuffer_[cidx+1],
            instance_->rgbBuffer_[cidx+2],
            static_cast<unsigned long>(sum));
    }

    // Dump one PPM frame at tick 60 (~2s in) for remote debugging
    if (tickCount == 60) {
        FILE* f = std::fopen("/tmp/koilo_frame.ppm", "wb");
        if (f) {
            std::fprintf(f, "P6\n%u %u\n255\n", fbWidth, fbHeight);
            std::fwrite(instance_->rgbBuffer_.data(), 1, rgbSize, f);
            std::fclose(f);
            std::fprintf(stderr, "[led_diag] Wrote /tmp/koilo_frame.ppm (%ux%u)\n",
                         fbWidth, fbHeight);
        }
    }

    // Sample at each LED UV position
    instance_->volumeCamera_->SamplePixels(
        instance_->rgbBuffer_.data(), fbWidth, fbHeight);

    const uint8_t* ledData = instance_->volumeCamera_->GetPackedOutput();
    if (!ledData) return;

    uint16_t ledCount = instance_->backend_->GetLEDCount();
    Framebuffer fb(const_cast<uint8_t*>(ledData), ledCount, 1,
                   PixelFormat::RGB888, ledCount * 3);

    instance_->backend_->Present(fb);
}

void LEDVolumeModule::OnMessage(const Message& /*msg*/) {
    // Reserved for future message handling (e.g., layout reload).
}

void LEDVolumeModule::Shutdown() {
    if (!instance_) {
        return;
    }

    if (instance_->kernel_) {
        auto& svc = instance_->kernel_->Services();
        if (svc.Has("led_volume")) {
            svc.Unregister("led_volume");
        }
    }

    instance_->volumeCamera_.reset();
    instance_->ledPixelGroup_.reset();

    if (instance_->backend_) {
        instance_->backend_->Shutdown();
        instance_->backend_.reset();
    }

    if (instance_->transport_) {
        instance_->transport_->Shutdown();
        instance_->transport_.reset();
    }

    instance_.reset();
}

uint16_t LEDVolumeModule::GetLEDCount() const {
    return backend_ ? backend_->GetLEDCount() : 0;
}

// -- Test Patterns ------------------------------------------------------------

void LEDVolumeModule::FillTestPattern(const std::string& pattern,
                                       uint8_t* buffer,
                                       uint16_t ledCount) {
    const size_t bytes = static_cast<size_t>(ledCount) * 3;

    if (pattern == "red") {
        for (size_t i = 0; i < bytes; i += 3) {
            buffer[i] = 255; buffer[i + 1] = 0; buffer[i + 2] = 0;
        }
    } else if (pattern == "green") {
        for (size_t i = 0; i < bytes; i += 3) {
            buffer[i] = 0; buffer[i + 1] = 255; buffer[i + 2] = 0;
        }
    } else if (pattern == "blue") {
        for (size_t i = 0; i < bytes; i += 3) {
            buffer[i] = 0; buffer[i + 1] = 0; buffer[i + 2] = 255;
        }
    } else if (pattern == "white") {
        std::memset(buffer, 255, bytes);
    } else if (pattern == "gradient") {
        for (uint16_t i = 0; i < ledCount; ++i) {
            uint8_t val = static_cast<uint8_t>((i * 255) / (ledCount > 1 ? ledCount - 1 : 1));
            buffer[i * 3]     = val;
            buffer[i * 3 + 1] = val;
            buffer[i * 3 + 2] = val;
        }
    } else {
        // Unknown pattern -- fill black
        std::memset(buffer, 0, bytes);
    }
}

// -- Console Commands ---------------------------------------------------------

void LEDVolumeModule::RegisterCommands() {
    auto* console = kernel_->Services().Get<void>("console");
    if (!console) {
        return;
    }

    // The console module exposes a CommandRegistry. We access it through
    // the kernel service registry the same way other modules do.
    auto* consoleModule = static_cast<koilo::ConsoleModule*>(
        kernel_->Services().Get<void>("console"));

    if (!consoleModule) {
        return;
    }

    auto& cmds = consoleModule->Commands();

    // -- led.status --
    cmds.Register({"led.status", "led.status", "Show LED volume status",
        [](KoiloKernel& /*k*/, const std::vector<std::string>& /*args*/)
            -> ConsoleResult {
            auto* mod = LEDVolumeModule::Instance();
            if (!mod || !mod->backend_) {
                return ConsoleResult::Ok("LED volume module not active.");
            }

            std::ostringstream os;
            os << "LED Volume Status:\n"
               << "  LEDs:     " << mod->backend_->GetLEDCount() << "\n"
               << "  Frames:   " << mod->backend_->GetFrameCount() << "\n"
               << "  Dropped:  " << mod->backend_->GetDroppedFrames() << "\n"
               << "  Frozen:   " << (mod->backend_->IsFrozen() ? "yes" : "no") << "\n"
               << "  Gamma:    " << mod->backend_->GetGamma() << "\n"
               << "  Transport:" << (mod->transport_ ? " connected" : " none");
            return ConsoleResult::Ok(os.str());
        },
        nullptr
    });

    // -- led.stats --
    cmds.Register({"led.stats", "led.stats", "Show LED transport statistics",
        [](KoiloKernel& /*k*/, const std::vector<std::string>& /*args*/)
            -> ConsoleResult {
            auto* mod = LEDVolumeModule::Instance();
            if (!mod || !mod->transport_) {
                return ConsoleResult::Ok("No LED transport available.");
            }

            auto stats = mod->transport_->GetStats();
            std::ostringstream os;
            os << "LED Transport Stats:\n"
               << "  Frames sent:  " << stats.framesSent << "\n"
               << "  Bytes total:  " << stats.bytesTransferred << "\n"
               << "  Errors:       " << stats.errors << "\n"
               << "  Dropped:      " << stats.framesDropped << "\n"
               << "  Last xfer:    " << std::fixed << std::setprecision(2)
               << stats.lastTransferMs << " ms";
            return ConsoleResult::Ok(os.str());
        },
        nullptr
    });

    // -- led.brightness --
    cmds.Register({"led.brightness", "led.brightness [0-255]",
        "Get/set LED brightness",
        [](KoiloKernel& /*k*/, const std::vector<std::string>& args)
            -> ConsoleResult {
            auto* mod = LEDVolumeModule::Instance();
            if (!mod || !mod->backend_) {
                return ConsoleResult::Error("LED volume not active.");
            }

            if (args.empty()) {
                return ConsoleResult::Ok(
                    "Brightness: " + std::to_string(
                        static_cast<int>(mod->backend_->GetInfo().refreshRate)));
            }

            int val = std::stoi(args[0]);
            if (val < 0 || val > 255) {
                return ConsoleResult::Error("Brightness must be 0-255.");
            }
            mod->backend_->SetBrightness(static_cast<uint8_t>(val));
            cvar_ledBrightness.Set(val);
            return ConsoleResult::Ok(
                "Brightness set to " + std::to_string(val));
        },
        nullptr
    });

    // -- led.gamma --
    cmds.Register({"led.gamma", "led.gamma [value]",
        "Get/set LED gamma correction",
        [](KoiloKernel& /*k*/, const std::vector<std::string>& args)
            -> ConsoleResult {
            auto* mod = LEDVolumeModule::Instance();
            if (!mod || !mod->backend_) {
                return ConsoleResult::Error("LED volume not active.");
            }

            if (args.empty()) {
                std::ostringstream os;
                os << "Gamma: " << mod->backend_->GetGamma();
                return ConsoleResult::Ok(os.str());
            }

            float val = std::stof(args[0]);
            if (val < 0.1f || val > 5.0f) {
                return ConsoleResult::Error("Gamma must be 0.1 to 5.0.");
            }
            mod->backend_->SetGamma(val);
            cvar_ledGamma.Set(val);
            return ConsoleResult::Ok(
                "Gamma set to " + std::to_string(val));
        },
        nullptr
    });

    // -- led.freeze --
    cmds.Register({"led.freeze", "led.freeze [on|off]",
        "Freeze/unfreeze LED output",
        [](KoiloKernel& /*k*/, const std::vector<std::string>& args)
            -> ConsoleResult {
            auto* mod = LEDVolumeModule::Instance();
            if (!mod || !mod->backend_) {
                return ConsoleResult::Error("LED volume not active.");
            }

            if (args.empty()) {
                return ConsoleResult::Ok(
                    std::string("Frozen: ") +
                    (mod->backend_->IsFrozen() ? "yes" : "no"));
            }

            bool freeze = (args[0] == "on" || args[0] == "1" ||
                           args[0] == "true");
            mod->backend_->SetFrozen(freeze);
            return ConsoleResult::Ok(
                freeze ? "Output frozen." : "Output resumed.");
        },
        nullptr
    });

    // -- led.test --
    cmds.Register({"led.test", "led.test <pattern>",
        "Send a test pattern (red, green, blue, white, gradient)",
        [](KoiloKernel& /*k*/, const std::vector<std::string>& args)
            -> ConsoleResult {
            auto* mod = LEDVolumeModule::Instance();
            if (!mod || !mod->backend_) {
                return ConsoleResult::Error("LED volume not active.");
            }

            std::string pattern = args.empty() ? "white" : args[0];
            uint16_t count = mod->backend_->GetLEDCount();
            std::vector<uint8_t> buf(static_cast<size_t>(count) * 3);
            FillTestPattern(pattern, buf.data(), count);

            Framebuffer fb(buf.data(), count, 1,
                           PixelFormat::RGB888, count * 3);
            bool ok = mod->backend_->Present(fb);

            return ok ? ConsoleResult::Ok("Test pattern '" + pattern + "' sent.")
                      : ConsoleResult::Error("Failed to send test pattern.");
        },
        nullptr
    });
}

} // namespace koilo

#endif // KL_HAVE_LED_VOLUME
