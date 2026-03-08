// SPDX-License-Identifier: GPL-3.0-or-later
// space_shooter/main.cpp - KoiloEngine space shooter game
// 2D game using CPU software renderer with vsync.
#include <koilo/platform/sdl2_host.hpp>

int main(int argc, char** argv) {
    koilo::SDL2Host host;
    host.scriptPath    = "examples/space_shooter/space_shooter.ks";
    host.windowTitle   = "KoiloEngine - Space Shooter";
    host.forceSoftware = true;
    host.enableVSync   = true;
    return host.Run(argc, argv);
}
