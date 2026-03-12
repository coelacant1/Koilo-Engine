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
 *   --software    Force software rendering
 *   --uncapped    Disable frame rate cap (ignore script SetTargetFPS)
 *   --no-profiler Disable the performance profiler
 *   --profiler    Enable profiler with periodic terminal reports
 *   --vsync       Enable vertical sync
 */

#include <koilo/scripting/koiloscript_engine.hpp>
#include <koilo/platform/sdl3_input_helper.hpp>
#include <koilo/platform/desktop_file_reader.hpp>
#include <koilo/core/platform/thread_affinity.hpp>
#include <koilo/core/time/timemanager.hpp>
#include <koilo/systems/display/backends/gpu/openglbackend.hpp>
#include <koilo/systems/display/framebuffer.hpp>
#include <koilo/systems/render/gl/render_backend_factory.hpp>
#include <koilo/systems/render/gl/opengl_render_backend.hpp>
#include <koilo/systems/profiling/performanceprofiler.hpp>
#include <SDL3/SDL.h>
#include <cstdio>
#include <cstring>
#include <chrono>
#include <thread>
#include <algorithm>

namespace koilo {

struct SDL3Host {
    // Required
    const char* scriptPath  = nullptr;
    const char* windowTitle = "KoiloEngine";

    // Defaults (overridable before Run)
    bool enableProfiler  = false;
    bool enableVSync     = false;
    bool forceSoftware   = false;
    bool forceUncapped   = false;
    int  profilerInterval = 120;  // Print report every N frames (0 = never)

    int Run(int argc, char** argv) {
        ParseArgs(argc, argv);

        platform::PinToPerformanceCore();
        platform::DesktopFileReader reader;
        scripting::KoiloScriptEngine engine(&reader);

        if (!LoadAndBuild(engine)) return 1;
        return RunLoop(argc, argv, engine);
    }

    // For hosts that need to configure the engine before loading (e.g. modules)
    int RunWithEngine(int argc, char** argv,
                      scripting::KoiloScriptEngine& engine) {
        ParseArgs(argc, argv);
        if (!LoadAndBuild(engine)) return 1;
        return RunLoop(argc, argv, engine);
    }

private:
    bool LoadAndBuild(scripting::KoiloScriptEngine& engine) {
        if (!scriptPath || !engine.LoadScript(scriptPath)) {
            printf("Failed to load script: %s\nError: %s\n",
                   scriptPath ? scriptPath : "(null)", engine.GetError());
            return false;
        }
        if (!engine.BuildScene()) {
            printf("Failed to build scene: %s\n", engine.GetError());
            return false;
        }
        return true;
    }

    int RunLoop(int /*argc*/, char** /*argv*/,
                scripting::KoiloScriptEngine& engine) {
        auto info = engine.GetDisplayInfo();
        int winW = info.width      > 0 ? info.width      : 960;
        int winH = info.height     > 0 ? info.height     : 540;
        int pixW = info.pixelWidth > 0 ? info.pixelWidth : 192;
        int pixH = info.pixelHeight> 0 ? info.pixelHeight: 108;

        // Frame cap
        bool capFPS = info.capFPS && !forceUncapped;
        float targetFrameTime = 0.0f;
        if (capFPS && info.targetFPS > 0.0f)
            targetFrameTime = 1.0f / info.targetFPS;

        // Create display
        OpenGLBackend display(winW, winH, windowTitle, false, true);
        if (!display.Initialize()) {
            printf("Failed to initialize OpenGL display backend\n");
            return 1;
        }
        display.SetVSyncEnabled(enableVSync);
        if (forceSoftware) display.SetNearestFiltering(true);

        // Select render backend
        if (!forceSoftware)
            engine.SetRenderBackend(CreateBestRenderBackend());
        else
            engine.SetRenderBackend(CreateBestSoftwareBackend());

        auto* gpuBackend = dynamic_cast<OpenGLRenderBackend*>(engine.GetRenderBackend());

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
            if (enableProfiler) profiler.BeginFrame();

            uint64_t now = SDL_GetPerformanceCounter();
            float  dt  = static_cast<float>(now - lastTick) / static_cast<float>(freq);
            lastTick   = now;
            TimeManager::GetInstance().Tick(dt);

            // Events
            SDL_Event e;
            while (SDL_PollEvent(&e)) {
                if (e.type == SDL_EVENT_QUIT) { running = false; break; }
                if (e.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED)
                    { running = false; break; }
                if (e.type == SDL_EVENT_WINDOW_RESIZED) {
                    winW = e.window.data1;
                    winH = e.window.data2;
                    // Re-render immediately during resize for smooth feedback
                    if (gpuBackend) {
                        engine.RenderFrameGPU();
                        glBindFramebuffer(GL_FRAMEBUFFER, 0);
                        glViewport(0, 0, winW, winH);
                        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
                        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                        gpuBackend->BlitToScreen(winW, winH);
                        gpuBackend->CompositeCanvasOverlays(winW, winH);
                        engine.RenderUIOverlay(winW, winH, dt);
                        display.SwapOnly();
                    }
                }
                if (!HandleSDL3Event(engine, e))
                    running = false;
            }

            uint64_t workStart = SDL_GetPerformanceCounter();

            if (gpuBackend) {
                if (!firstFrame) display.SwapOnly();
                firstFrame = false;

                engine.ExecuteUpdate();
                if (engine.HasError()) {
                    printf("Script error: %s\n", engine.GetError());
                    running = false; break;
                }

                engine.RenderFrameGPU();
                // Ensure default framebuffer is bound and cleared before
                // compositing the scene, canvas overlays, and UI.
                glBindFramebuffer(GL_FRAMEBUFFER, 0);
                glViewport(0, 0, winW, winH);
                glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                gpuBackend->BlitToScreen(winW, winH);
                gpuBackend->CompositeCanvasOverlays(winW, winH);
                engine.RenderUIOverlay(winW, winH, dt);
            } else {
                // Software render path
                engine.ExecuteUpdate();
                if (engine.HasError()) {
                    printf("Script error: %s\n", engine.GetError());
                    running = false; break;
                }

                engine.RenderFrame(fb.data(), pixW, pixH);
                engine.UpdateUIAnimations(dt);
                Framebuffer framebuffer(
                    reinterpret_cast<uint8_t*>(fb.data()),
                    pixW, pixH, PixelFormat::RGB888);
                display.PresentNoSwap(framebuffer);
                display.SwapOnly();
            }

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

            // Frame cap
            if (capFPS && targetFrameTime > 0.0f) {
                uint64_t frameEnd = SDL_GetPerformanceCounter();
                float elapsed = static_cast<float>(frameEnd - now) /
                                static_cast<float>(freq);
                float remaining = targetFrameTime - elapsed;
                if (remaining > 0.001f) {
                    std::this_thread::sleep_for(
                        std::chrono::microseconds(
                            static_cast<long long>(remaining * 1000000.0f)));
                }
            }

            // Estimated max FPS console report (every 300 frames when capped)
            if (capFPS && frameNum % 300 == 0 && estMaxFPS > 0.0) {
                printf("[Host] Est. max FPS: %.0f  (capped at %.0f)\n",
                       estMaxFPS, info.targetFPS);
            }
        }

        if (!firstFrame) display.SwapOnly();
        display.Shutdown();
        return 0;
    }

    void ParseArgs(int argc, char** argv) {
        for (int i = 1; i < argc; ++i) {
            if (std::strcmp(argv[i], "--software") == 0)    forceSoftware = true;
            else if (std::strcmp(argv[i], "--uncapped") == 0)    forceUncapped = true;
            else if (std::strcmp(argv[i], "--profiler") == 0)    enableProfiler = true;
            else if (std::strcmp(argv[i], "--no-profiler") == 0) enableProfiler = false;
            else if (std::strcmp(argv[i], "--vsync") == 0)       enableVSync = true;
        }
    }
};

} // namespace koilo
