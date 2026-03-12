// SPDX-License-Identifier: GPL-3.0-or-later
// stress_test/main.cpp - 3D stress test
#include <koilo/platform/sdl3_host.hpp>

int main(int argc, char** argv) {
    koilo::SDL3Host host;
    host.scriptPath  = "examples/stress_test/stress_test.ks";
    host.windowTitle = "KoiloEngine - Stress Test";
    return host.Run(argc, argv);
}
