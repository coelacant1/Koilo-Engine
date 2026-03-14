// SPDX-License-Identifier: GPL-3.0-or-later
// ui_showcase/main.cpp - KoiloEngine UI Widget Showcase
// Displays all UI widget types in a comprehensive demo for visual debugging.
#include <koilo/platform/sdl3_host.hpp>

int main(int argc, char** argv) {
    koilo::SDL3Host host;
    host.scriptPath    = "examples/ui_showcase/ui_showcase.ks";
    host.windowTitle   = "KoiloEngine - UI Showcase";
    host.forceSoftware = false;
    host.enableVSync   = true;
    return host.Run(argc, argv);
}
