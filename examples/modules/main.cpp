// SPDX-License-Identifier: GPL-3.0-or-later
// modules/main.cpp - ELF module demo
// Uses SDL3Host but pre-loads modules before script execution.
#include <koilo/platform/sdl3_host.hpp>
#include <koilo/kernel/module_loader.hpp>
#include <cstdio>
#include <cstring>

int main(int argc, char** argv) {
    // Parse --modules-dir and positional script path before host runs
    const char* scriptPath = "examples/modules/demo_all_modules.ks";
    const char* modulesDir = "build/modules";
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--modules-dir") == 0 && i + 1 < argc)
            modulesDir = argv[++i];
        else if (argv[i][0] != '-')
            scriptPath = argv[i];
    }

    // Pre-load modules via a custom engine so they're available to the script
    koilo::platform::PinToPerformanceCore();
    koilo::platform::DesktopFileReader reader;
    koilo::scripting::KoiloScriptEngine engine(&reader);

    int loaded = engine.GetModuleLoader().ScanAndLoad(modulesDir);
    printf("[modules] Loaded %d module(s) from %s\n", loaded, modulesDir);
    if (loaded == 0)
        printf("[modules] Warning: no modules found.\n");

    koilo::SDL3Host host;
    host.scriptPath  = scriptPath;
    host.windowTitle = "KoiloEngine - Modules";
    return host.RunWithEngine(argc, argv, engine);
}
