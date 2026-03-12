// SPDX-License-Identifier: GPL-3.0-or-later
// syntax_demo/main.cpp - KoiloScript syntax highlighting demo
#include <koilo/platform/sdl3_host.hpp>

int main(int argc, char** argv) {
    koilo::SDL3Host host;
    host.scriptPath  = "examples/syntax_demo/syntax_demo.ks";
    host.windowTitle = "KoiloEngine - Syntax Demo";
    return host.Run(argc, argv);
}
