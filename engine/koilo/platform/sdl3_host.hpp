// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
/**
 * @file sdl3_host.hpp
 * @brief Reusable SDL3 host application for KoiloEngine examples.
 *
 * Usage:
 *   #include <koilo/platform/sdl3_host.hpp>
 *   int main(int argc, char** argv) {
 *       koilo::SDL3Host host;
 *       host.scriptPath  = "examples/my_example/my_example.ks";
 *       host.windowTitle = "KoiloEngine - My Example";
 *       return host.Run(argc, argv);
 *   }
 *
 * Command-line flags:
 *   --script <path>       Path to .ks script file
 *   --title <name>        Window title (default: "KoiloEngine")
 *   --software            Force software rendering
 *   --uncapped            Disable frame rate cap (ignore script SetTargetFPS)
 *   --no-profiler         Disable the performance profiler
 *   --profiler            Enable profiler with periodic terminal reports
 *   --vsync               Enable vertical sync
 *   --modules-dir <path>  Pre-load .so modules from directory
 *   --console             Start with dev console enabled
 */

#include <koilo/scripting/koiloscript_engine.hpp>
#include <koilo/kernel/module_loader.hpp>
#include <koilo/kernel/kernel.hpp>
#include <koilo/kernel/register_services.hpp>
#include <koilo/systems/ui/ui.hpp>
#include <koilo/systems/render/core/pixelgroup.hpp>
#include <koilo/systems/scene/camera/camera.hpp>
#include <koilo/systems/scene/camera/cameralayout.hpp>
#include <koilo/systems/scene/scene.hpp>
#include <koilo/systems/render/canvas2d.hpp>
#include <koilo/systems/render/irenderbackend.hpp>
#include <koilo/systems/render/igpu_render_backend.hpp>
#include <koilo/systems/display/igpu_display_backend.hpp>
#include <koilo/platform/sdl3_input_helper.hpp>
#include <koilo/platform/desktop_file_reader.hpp>
#include <koilo/core/platform/thread_affinity.hpp>
#include <koilo/core/time/timemanager.hpp>
#ifdef KL_HAVE_OPENGL_BACKEND
#include <koilo/systems/display/backends/gpu/openglbackend.hpp>
#endif
#ifdef KL_HAVE_VULKAN_BACKEND
#include <koilo/systems/display/backends/gpu/vulkanbackend.hpp>
#endif
#include <koilo/systems/display/framebuffer.hpp>
#include <koilo/systems/render/gl/render_backend_factory.hpp>
#include <koilo/systems/profiling/performanceprofiler.hpp>
#include <SDL3/SDL.h>
#include <cstdio>
#include <cstring>
#include <chrono>
#include <thread>
#include <algorithm>
#include <atomic>
#include <koilo/kernel/cvar/cvar_system.hpp>
#include <koilo/systems/render/render_cvars.hpp>
#include <koilo/kernel/sim_cvars.hpp>
#include <koilo/kernel/logging/log.hpp>

namespace koilo {

struct SDL3Host {
    // Required
    const char* scriptPath  = nullptr;
    const char* windowTitle = "KoiloEngine";

    // Defaults (overridable before Run or via CLI flags)
    bool enableProfiler  = false;
    bool enableVSync     = false;
    bool forceSoftware   = false;
    bool forceUncapped   = false;
    bool enableConsole   = false;
    bool preferVulkan    = false;
    int  profilerInterval = 120;  // Print report every N frames (0 = never)
    const char* modulesDir = nullptr;
    std::atomic<bool>* externalStop = nullptr;  // If set, RunLoop exits when *externalStop == false

    int Run(int argc, char** argv) {
        ParseArgs(argc, argv);

        platform::PinToPerformanceCore();
        platform::DesktopFileReader reader;
        KoiloKernel kernel;
        scripting::KoiloScriptEngine engine(&reader);
        engine.SetKernel(&kernel);
        kernel.Services().Register("script", &engine);
        kernel.Services().Register("script_bridge", static_cast<IScriptBridge*>(&engine));
        RegisterCoreServices(kernel);

        if (modulesDir) {
            int loaded = engine.GetModuleLoader().ScanAndLoad(modulesDir);
            KL_LOG("koilo", "Loaded %d module(s) from %s", loaded, modulesDir);
        }

        if (!LoadAndBuild(engine)) return 1;
        int rc = RunLoop(argc, argv, engine, kernel);
        kernel.Shutdown();
        return rc;
    }

    // For hosts that need to configure the engine before loading (e.g. modules)
    int RunWithEngine(int argc, char** argv,
                      scripting::KoiloScriptEngine& engine) {
        ParseArgs(argc, argv);
        KoiloKernel kernel;
        engine.SetKernel(&kernel);
        kernel.Services().Register("script", &engine);
        kernel.Services().Register("script_bridge", static_cast<IScriptBridge*>(&engine));
        RegisterCoreServices(kernel);
        if (!LoadAndBuild(engine)) return 1;
        int rc = RunLoop(argc, argv, engine, kernel);
        kernel.Shutdown();
        return rc;
    }

    /// Run display using an externally-owned kernel and engine.
    /// The caller is responsible for kernel lifecycle (init, shutdown).
    int RunWithKernel(int argc, char** argv,
                      scripting::KoiloScriptEngine& engine,
                      KoiloKernel& kernel) {
        ParseArgs(argc, argv);
        if (!LoadAndBuild(engine)) return 1;
        return RunLoop(argc, argv, engine, kernel);
    }

private:
    bool LoadAndBuild(scripting::KoiloScriptEngine& engine) {
        if (!scriptPath || !engine.LoadScript(scriptPath)) {
            KL_ERR("koilo", "Failed to load script: %s - %s",
                   scriptPath ? scriptPath : "(null)", engine.GetError());
            return false;
        }
        if (!engine.BuildScene()) {
            KL_ERR("koilo", "Failed to build scene: %s", engine.GetError());
            return false;
        }
        return true;
    }

    int RunLoop(int /*argc*/, char** /*argv*/,
                scripting::KoiloScriptEngine& engine,
                KoiloKernel& kernel) {
        auto info = engine.GetDisplayInfo();
        int winW = info.width      > 0 ? info.width      : 960;
        int winH = info.height     > 0 ? info.height     : 540;
        int pixW = info.pixelWidth > 0 ? info.pixelWidth : 192;
        int pixH = info.pixelHeight> 0 ? info.pixelHeight: 108;

        // Frame cap - seed r.maxfps CVar from script info
        if (info.capFPS && info.targetFPS > 0.0f)
            cvar_r_maxfps.Set(static_cast<int>(info.targetFPS));
        if (forceUncapped)
            cvar_r_maxfps.Set(0);

        // Create display backend
        std::unique_ptr<IDisplayBackend> displayOwner;
        IGPUDisplayBackend* gpuDisplayPtr = nullptr;
#ifdef KL_HAVE_VULKAN_BACKEND
        VulkanBackend* vkDisplay = nullptr;
        if (preferVulkan) {
            auto vk = std::make_unique<VulkanBackend>(
                static_cast<uint32_t>(winW), static_cast<uint32_t>(winH),
                std::string(windowTitle), false, true);
            if (vk->Initialize()) {
                vkDisplay = vk.get();
                gpuDisplayPtr = vk.get();
                displayOwner = std::move(vk);
            } else {
                KL_WARN("SDL3Host", "Vulkan init failed, falling back to OpenGL");
            }
        }
#endif
        if (!displayOwner) {
#ifdef KL_HAVE_OPENGL_BACKEND
            auto gl = std::make_unique<OpenGLBackend>(winW, winH, windowTitle, false, true);
            if (!gl->Initialize()) {
                KL_ERR("SDL3Host", "Failed to initialize OpenGL display backend");
                return 1;
            }
            gpuDisplayPtr = gl.get();
            displayOwner = std::move(gl);
#else
            KL_ERR("SDL3Host", "No display backend available");
            return 1;
#endif
        }
        IGPUDisplayBackend& gpuDisplay = *gpuDisplayPtr;
        if (enableVSync) cvar_r_vsync.Set(true);
        gpuDisplay.SetVSyncEnabled(cvar_r_vsync.Get());
        if (forceSoftware) gpuDisplay.SetNearestFiltering(true);

        // Select render backend
        if (!forceSoftware) {
#ifdef KL_HAVE_VULKAN_BACKEND
            if (vkDisplay)
                engine.SetRenderBackend(TryCreateVulkanRenderBackend(vkDisplay));
            else
#endif
                engine.SetRenderBackend(CreateBestRenderBackend());
        } else {
            engine.SetRenderBackend(CreateBestSoftwareBackend());
        }

        auto* gpuBackend = dynamic_cast<IGPURenderBackend*>(engine.GetRenderBackend());

        // Profiler
        auto& profiler = PerformanceProfiler::GetInstance();
        profiler.SetEnabled(enableProfiler);
        if (enableProfiler) profiler.SetHistorySize(120);

        // Frame state
        std::vector<Color888> fb(pixW * pixH);
        bool   running    = true;
        bool   firstFrame = true;
        int    frameNum   = 0;
        uint64_t freq     = SDL_GetPerformanceFrequency();
        uint64_t lastTick = SDL_GetPerformanceCounter();

        // Work-time accumulator for estimated max FPS
        double workTimeAccum = 0.0;
        int    workFrames    = 0;
        double estMaxFPS     = 0.0;

        while (running) {
            if (externalStop && !externalStop->load(std::memory_order_relaxed))
                { running = false; break; }
            if (enableProfiler) profiler.BeginFrame();

            uint64_t now = SDL_GetPerformanceCounter();
            float  dt  = static_cast<float>(now - lastTick) / static_cast<float>(freq);
            lastTick   = now;
            TimeManager::GetInstance().Tick(dt);

            // Determine if the UI is idle (no visual changes pending)
            UI* uiPtr = engine.GetUI();
            bool uiIdle = uiPtr && uiPtr->IsIdle();
            bool sceneActive = engine.GetScene() != nullptr;

            // Events - use blocking wait when UI-only and idle to save CPU/GPU
            SDL_Event e;
            bool hadEvents = false;
            auto checkStop = [&]() {
                if (externalStop && !externalStop->load(std::memory_order_relaxed))
                    running = false;
            };
            if (!sceneActive && uiIdle) {
                // Block up to 16ms waiting for input events
                if (SDL_WaitEventTimeout(&e, 16)) {
                    hadEvents = true;
                    checkStop();
                    if (running) do {
                        if (e.type == SDL_EVENT_QUIT) { running = false; break; }
                        if (e.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED)
                            { running = false; break; }
                        if (e.type == SDL_EVENT_WINDOW_RESIZED) {
                            winW = e.window.data1;
                            winH = e.window.data2;
                        }
                        if (!HandleSDL3Event(engine, e))
                            running = false;
                    } while (running && SDL_PollEvent(&e));
                } else {
                    checkStop();
                }
            } else {
                while (SDL_PollEvent(&e)) {
                    hadEvents = true;
                    if (e.type == SDL_EVENT_QUIT) { running = false; break; }
                    if (e.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED)
                        { running = false; break; }
                    if (e.type == SDL_EVENT_WINDOW_RESIZED) {
                        winW = e.window.data1;
                        winH = e.window.data2;
#ifdef KL_HAVE_VULKAN_BACKEND
                        if (vkDisplay) {
                            vkDisplay->NotifyResized();
                        }
#endif
                    }
                    if (!HandleSDL3Event(engine, e))
                        running = false;
                }
            }

            // Re-check idle state after processing events
            uiIdle = uiPtr && uiPtr->IsIdle();

            // If shutdown was requested, exit before starting a new frame.
            // This avoids submitting command buffers with stale image layouts
            // during the teardown sequence.
            if (!running) break;

            // When UI-only and still idle after events, skip the full render cycle.
            // Require at least 2 rendered frames first so the initial frame gets
            // presented via SwapOnly (which submits the previous iteration's work).
            if (!sceneActive && uiIdle && !hadEvents && frameNum >= 2) {
                if (enableProfiler) profiler.EndFrame();
                continue;
            }

            uint64_t workStart = SDL_GetPerformanceCounter();
            kernel.BeginFrame();

            if (gpuBackend) {
                if (!firstFrame) gpuDisplay.SwapOnly();
                firstFrame = false;

                // Simulation pause: skip tick unless stepping
                bool simPaused = cvar_sim_pause.Get();
                if (!simPaused || g_simStepFrames.load() > 0) {
                    if (simPaused) g_simStepFrames.fetch_sub(1);
                    kernel.Tick(dt);
                    engine.ExecuteUpdate();
                    if (engine.HasError()) {
                        KL_ERR("koilo", "Script error: %s", engine.GetError());
                        running = false; break;
                    }
                }

                engine.RenderFrameGPU();
                gpuDisplay.PrepareDefaultFramebuffer(winW, winH);
                gpuBackend->BlitToScreen(winW, winH);
                gpuBackend->CompositeCanvasOverlays(winW, winH);
#ifdef KL_HAVE_VULKAN_BACKEND
                if (vkDisplay) {
                    // Vulkan UI: render into the active swapchain render pass
                    if (uiPtr) {
                        engine.UpdateUIAnimations(dt);
                        if (!uiPtr->InitializeVulkanGPU(vkDisplay)) {
                            // Vulkan UI init failed - skip UI rendering
                        } else {
                            VkCommandBuffer cmd = vkDisplay->BeginFrame();
                            if (cmd) uiPtr->RenderVulkanGPU(winW, winH, cmd);
                        }
                    }
                } else
#endif
                {
                    engine.RenderUIOverlay(winW, winH, dt);
                }
            } else {
                // Software render path
                bool simPaused = cvar_sim_pause.Get();
                if (!simPaused || g_simStepFrames.load() > 0) {
                    if (simPaused) g_simStepFrames.fetch_sub(1);
                    kernel.Tick(dt);
                    engine.ExecuteUpdate();
                    if (engine.HasError()) {
                        KL_ERR("koilo", "Script error: %s", engine.GetError());
                        running = false; break;
                    }
                }

                engine.RenderFrame(fb.data(), pixW, pixH);
                engine.UpdateUIAnimations(dt);
                Framebuffer framebuffer(
                    reinterpret_cast<uint8_t*>(fb.data()),
                    pixW, pixH, PixelFormat::RGB888);
                gpuDisplay.PresentNoSwap(framebuffer);
                gpuDisplay.SwapOnly();
            }

            kernel.EndFrame();

            uint64_t workEnd = SDL_GetPerformanceCounter();
            double workMs = static_cast<double>(workEnd - workStart) /
                            static_cast<double>(freq) * 1000.0;
            workTimeAccum += workMs;
            workFrames++;

            // Update rolling estimated max FPS and expose to scripts
            if (workFrames >= 60) {
                double avgWorkMs = workTimeAccum / workFrames;
                estMaxFPS = (avgWorkMs > 0.01) ? 1000.0 / avgWorkMs : 9999.0;
                workTimeAccum = 0.0;
                workFrames = 0;
            }
            engine.SetGlobal("mfps", scripting::Value(estMaxFPS));

            if (enableProfiler) profiler.EndFrame();
            frameNum++;

            // Periodic report
            if (enableProfiler && profilerInterval > 0 &&
                frameNum % profilerInterval == 0) {
                profiler.PrintReport();
            }

            // Frame cap (re-read r.maxfps CVar each frame)
            int cvarMaxFps = cvar_r_maxfps.Get();
            float curTargetFrameTime = (cvarMaxFps > 0) ? (1.0f / static_cast<float>(cvarMaxFps)) : 0.0f;
            if (!forceUncapped && curTargetFrameTime > 0.0f) {
                uint64_t frameEnd = SDL_GetPerformanceCounter();
                float elapsed = static_cast<float>(frameEnd - now) /
                                static_cast<float>(freq);
                float remaining = curTargetFrameTime - elapsed;
                if (remaining > 0.001f) {
                    std::this_thread::sleep_for(
                        std::chrono::microseconds(
                            static_cast<long long>(remaining * 1000000.0f)));
                }
            }

            // Estimated max FPS console report (every 300 frames when capped)
            if (cvarMaxFps > 0 && !forceUncapped && frameNum % 300 == 0 && estMaxFPS > 0.0) {
                KL_LOG("Host", "Est. max FPS: %.0f  (capped at %d)",
                       estMaxFPS, cvarMaxFps);
            }
        }

        // Wait for any in-flight GPU work to complete before destroying
        // resources.  Don't submit the last recorded frame - it may reference
        // images whose layouts are stale after the quit interrupt, and the
        // frame would never be displayed anyway.
#ifdef KL_HAVE_VULKAN_BACKEND
        if (vkDisplay) {
            vkDeviceWaitIdle(vkDisplay->GetDevice());
        }
#endif

        // Shut down render backend and UI GPU resources before display backend
        // so Vulkan resources are destroyed while the VkDevice is still alive.
        engine.SetRenderBackend(nullptr);
#ifdef KL_HAVE_VULKAN_BACKEND
        if (UI* ui = engine.GetUI()) ui->ShutdownVulkanGPU();
#endif

        displayOwner->Shutdown();
        return 0;
    }

    void ParseArgs(int argc, char** argv) {
        for (int i = 1; i < argc; ++i) {
            if (std::strcmp(argv[i], "--software") == 0)         forceSoftware = true;
            else if (std::strcmp(argv[i], "--uncapped") == 0)    forceUncapped = true;
            else if (std::strcmp(argv[i], "--profiler") == 0)    enableProfiler = true;
            else if (std::strcmp(argv[i], "--no-profiler") == 0) enableProfiler = false;
            else if (std::strcmp(argv[i], "--vsync") == 0)       enableVSync = true;
            else if (std::strcmp(argv[i], "--script") == 0 && i + 1 < argc)
                scriptPath = argv[++i];
            else if (std::strcmp(argv[i], "--title") == 0 && i + 1 < argc)
                windowTitle = argv[++i];
            else if (std::strcmp(argv[i], "--modules-dir") == 0 && i + 1 < argc)
                modulesDir = argv[++i];
            else if (std::strcmp(argv[i], "--console") == 0)
                enableConsole = true;
            else if (std::strcmp(argv[i], "--vulkan") == 0)
                preferVulkan = true;
        }
    }
};

} // namespace koilo
