// SPDX-License-Identifier: GPL-3.0-or-later
// scene_3d/main.cpp - 3D scene example
#include <koilo/platform/sdl3_host.hpp>

int main(int argc, char** argv) {
    koilo::SDL3Host host;
    host.scriptPath  = "examples/scene_3d/scene_3d.ks";
    host.windowTitle = "KoiloEngine - 3D Scene";
    return host.Run(argc, argv);
}
