// SPDX-License-Identifier: GPL-3.0-or-later
// obj_scene/main.cpp - Artisans Hub + Cynder textured OBJ scene
#include <koilo/platform/sdl2_host.hpp>

int main(int argc, char** argv) {
    koilo::SDL2Host host;
    host.scriptPath  = "examples/obj_scene/obj_scene.ks";
    host.windowTitle = "KoiloEngine - OBJ Scene";
    return host.Run(argc, argv);
}
