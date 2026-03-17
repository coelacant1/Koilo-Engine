// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file koilo_runner.cpp
 * @brief KoiloEngine universal entry point.
 *
 * The kernel always boots first with the console and TCP server.
 * CLI flags determine what happens next:
 *
 *   koilo                              Interactive console REPL
 *   koilo --script foo.ks [--vsync]    Boot kernel -> launch SDL3 display -> exit
 *   koilo --console-exec "help"        Boot kernel -> execute command -> exit
 *
 * From the REPL you can launch the display at any time:
 *   koilo> run examples/ui_showcase/ui_showcase.ks --vsync
 *   koilo> load examples/scene_3d/scene_3d.ks
 *   koilo> quit
 */
#include <koilo/platform/sdl3_host.hpp>
#include <koilo/kernel/kernel.hpp>
#include <koilo/kernel/console/console_module.hpp>
#include <koilo/kernel/register_services.hpp>
#include <koilo/platform/desktop_file_reader.hpp>
#include <koilo/scripting/koiloscript_engine.hpp>
#include <koilo/core/interfaces/iscript_bridge.hpp>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <string>
#include <iostream>
#include <csignal>
#include <atomic>
#include <unistd.h>
#include <SDL3/SDL.h>

// -- Signal handling -------------------------------------------------
static std::atomic<bool> g_running{true};
static std::atomic<int>  g_signalCount{0};
static koilo::KoiloKernel* g_kernel{nullptr};

static void SignalHandler(int) {
    g_running.store(false, std::memory_order_relaxed);
    int count = g_signalCount.fetch_add(1, std::memory_order_relaxed) + 1;
    // Push SDL_QUIT so the event loop exits promptly
    SDL_Event quitEvent{};
    quitEvent.type = SDL_EVENT_QUIT;
    SDL_PushEvent(&quitEvent);
    if (count >= 2) {
        const char msg[] = "\n[koilo] Forced exit.\n";
        (void)write(STDERR_FILENO, msg, sizeof(msg) - 1);
        _exit(1);
    }
    const char msg[] = "\n[koilo] Shutting down... (press again to force)\n";
    (void)write(STDERR_FILENO, msg, sizeof(msg) - 1);
}

// -- Kernel bootstrap (shared by all modes) --------------------------

struct KoiloInstance {
    koilo::KoiloKernel                  kernel;
    koilo::platform::DesktopFileReader  reader;
    koilo::scripting::KoiloScriptEngine engine{&reader};
    koilo::ConsoleModule*               console = nullptr;

    bool Boot() {
        engine.SetKernel(&kernel);
        kernel.Services().Register("script", &engine);
        kernel.Services().Register("script_bridge",
            static_cast<koilo::IScriptBridge*>(&engine));
        kernel.RegisterModule(koilo::ConsoleModule::GetModuleDesc());
        koilo::RegisterCoreServices(kernel);

        if (!kernel.InitializeModules()) {
            fprintf(stderr, "[koilo] Failed to initialize kernel modules\n");
            return false;
        }

        console = koilo::ConsoleModule::Instance();
        if (!console) {
            fprintf(stderr, "[koilo] Console module not available\n");
            return false;
        }

        console->StartSocket(9090);
        g_kernel = &kernel;
        return true;
    }

    void Shutdown() {
        g_kernel = nullptr;
        kernel.Shutdown();
    }
};

// -- Display launch (SDL3) -------------------------------------------

struct RunArgs {
    const char* script   = nullptr;
    const char* title    = nullptr;
    bool        software = false;
    bool        vsync    = false;
    bool        uncapped = false;
    const char* modules  = nullptr;
};

static RunArgs ParseRunArgs(const std::vector<std::string>& args) {
    RunArgs r;
    for (size_t i = 0; i < args.size(); ++i) {
        if (args[i] == "--software")       r.software = true;
        else if (args[i] == "--vsync")     r.vsync = true;
        else if (args[i] == "--uncapped")  r.uncapped = true;
        else if (args[i] == "--title" && i + 1 < args.size())
            r.title = args[++i].c_str();
        else if (args[i] == "--modules-dir" && i + 1 < args.size())
            r.modules = args[++i].c_str();
        else if (args[i][0] != '-' && !r.script)
            r.script = args[i].c_str();
    }
    return r;
}

static int LaunchDisplay(KoiloInstance& inst, int argc, char** argv) {
    koilo::SDL3Host host;
    host.externalStop = &g_running;
    return host.RunWithKernel(argc, argv, inst.engine, inst.kernel);
}

// -- REPL ------------------------------------------------------------

static void PrintBanner() {
    printf("|-------------------------------------|\n");
    printf("|         Koilo Engine Console         |\n");
    printf("|   Type 'help' for available commands |\n");
    printf("|   Type 'quit' to exit                |\n");
    printf("|--------------------------------------|\n");
}

static void RegisterREPLCommands(KoiloInstance& inst, int argc, char** argv) {
    auto* console = inst.console;

    console->Commands().Register({"quit", "quit", "Exit the console",
        [](koilo::KoiloKernel&, const std::vector<std::string>&)
            -> koilo::ConsoleResult {
            g_running = false;
            return koilo::ConsoleResult::Ok("Goodbye.");
        }, nullptr});

    console->Commands().Register({"exit", "exit", "Exit the console (alias for quit)",
        [](koilo::KoiloKernel&, const std::vector<std::string>&)
            -> koilo::ConsoleResult {
            g_running = false;
            return koilo::ConsoleResult::Ok("Goodbye.");
        }, nullptr});

    console->Commands().Register({"run",
        "run <script.ks> [--software] [--vsync] [--title name]",
        "Load a script and launch the SDL3 display window",
        [&inst, argc, argv](koilo::KoiloKernel&,
                            const std::vector<std::string>& args)
            -> koilo::ConsoleResult {
            if (args.empty()) {
                return koilo::ConsoleResult::Error(
                    "Usage: run <script.ks> [--software] [--vsync] [--title name]");
            }
            auto ra = ParseRunArgs(args);
            if (!ra.script) {
                return koilo::ConsoleResult::Error("No script path specified.");
            }
            printf("[koilo] Launching display: %s\n", ra.script);

            koilo::SDL3Host host;
            host.scriptPath    = ra.script;
            host.windowTitle   = ra.title ? ra.title : "KoiloEngine";
            host.forceSoftware = ra.software;
            host.enableVSync   = ra.vsync;
            host.forceUncapped = ra.uncapped;
            int rc = host.RunWithKernel(argc, argv, inst.engine, inst.kernel);

            char buf[128];
            snprintf(buf, sizeof(buf), "Display session ended (exit code: %d)", rc);
            return rc == 0 ? koilo::ConsoleResult::Ok(buf)
                           : koilo::ConsoleResult::Error(buf);
        }, nullptr});

    console->Commands().Register({"load", "load <script.ks>",
        "Load a KoiloScript file into the engine (headless, no display)",
        [&inst](koilo::KoiloKernel&, const std::vector<std::string>& args)
            -> koilo::ConsoleResult {
            if (args.empty()) {
                return koilo::ConsoleResult::Error("Usage: load <script.ks>");
            }
            if (!inst.engine.LoadScript(args[0].c_str())) {
                std::string err = "Failed to load: ";
                err += inst.engine.GetError();
                return koilo::ConsoleResult::Error(err);
            }
            if (!inst.engine.BuildScene()) {
                std::string err = "Failed to build scene: ";
                err += inst.engine.GetError();
                return koilo::ConsoleResult::Error(err);
            }
            return koilo::ConsoleResult::Ok(
                "Script loaded and scene built successfully.");
        }, nullptr});
}

// -- Help ------------------------------------------------------------

static void PrintUsage(const char* prog) {
    printf("KoiloEngine - modular game engine with microkernel architecture\n\n");
    printf("Usage:\n");
    printf("  %s                               Interactive console (headless)\n", prog);
    printf("  %s --script <path.ks> [options]   Launch with display\n", prog);
    printf("  %s --console-exec \"<command>\"     Execute command and exit\n\n", prog);
    printf("Display options:\n");
    printf("  --script <path>        Path to .ks script file\n");
    printf("  --title <name>         Window title (default: \"KoiloEngine\")\n");
    printf("  --software             Force software rendering\n");
    printf("  --vsync                Enable vertical sync\n");
    printf("  --uncapped             Disable FPS cap\n");
    printf("  --profiler             Enable performance profiler\n");
    printf("  --no-profiler          Disable performance profiler\n");
    printf("  --modules-dir <path>   Pre-load .so modules from directory\n");
    printf("  --console              Start TCP console alongside display\n");
    printf("  --help                 Show this help message\n");
}

// -- Main ------------------------------------------------------------

int main(int argc, char** argv) {
    const char* scriptPath    = nullptr;
    const char* consoleExec   = nullptr;
    bool        showHelp      = false;

    // Quick scan for mode-deciding flags
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--help") == 0 || std::strcmp(argv[i], "-h") == 0)
            showHelp = true;
        else if (std::strcmp(argv[i], "--script") == 0 && i + 1 < argc)
            scriptPath = argv[++i];
        else if (std::strcmp(argv[i], "--console-exec") == 0 && i + 1 < argc)
            consoleExec = argv[++i];
    }

    if (showHelp) {
        PrintUsage(argv[0]);
        return 0;
    }

    // Install signal handler
    // Install signal handler (no SA_RESTART so blocking calls return EINTR)
    struct sigaction sa{};
    sa.sa_handler = SignalHandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, nullptr);
    sigaction(SIGTERM, &sa, nullptr);

    // -- Always boot the kernel first --------------------------------
    KoiloInstance inst;
    if (!inst.Boot()) return 1;

    printf("[koilo] TCP console server on localhost:9090\n");

    // Mode 1: --console-exec "command" - run one command, exit
    if (consoleExec) {
        auto result = inst.console->Execute(consoleExec);
        std::string output = inst.console->Session().FormatResult(result);
        if (!output.empty()) printf("%s\n", output.c_str());
        inst.Shutdown();
        return result.ok() ? 0 : 1;
    }

    // Mode 2: --script - launch display then exit
    if (scriptPath) {
        int rc = LaunchDisplay(inst, argc, argv);
        inst.Shutdown();
        return rc;
    }

    // Mode 3: interactive REPL
    RegisterREPLCommands(inst, argc, argv);
    PrintBanner();
    printf("[koilo] TCP console server on localhost:9090\n\n");

    std::string line;
    while (g_running) {
        printf("koilo> ");
        fflush(stdout);

        if (!std::getline(std::cin, line)) {
            printf("\n");
            break;
        }

        if (line.empty()) continue;

        auto result = inst.console->Execute(line);
        std::string output = inst.console->Session().FormatResult(result);
        if (!output.empty()) {
            printf("%s\n", output.c_str());
        }

        if (!g_running) break;
    }

    inst.Shutdown();
    return 0;
}
