// SPDX-License-Identifier: GPL-3.0-or-later
// ui_demo/main.cpp - KoiloEngine UI system demonstration
// Shows the retained-mode UI widget system with GPU-accelerated rendering.
#include <koilo/platform/sdl3_host.hpp>

int main(int argc, char** argv) {
    koilo::SDL3Host host;
    host.scriptPath    = "examples/ui_demo/ui_demo_kml.ks";
    host.windowTitle   = "KoiloEngine - UI Demo";
    host.forceSoftware = false;
    host.enableVSync   = true;
    return host.Run(argc, argv);
}
