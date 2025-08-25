# PTX Engine - C++17 Math & Render Core for Microcontrollers **and** Desktops

[![compilecheck](https://github.com/coelacant1/PTXEngine/actions/workflows/ci.yml/badge.svg)](https://github.com/coelacant1/PTXEngine/actions/workflows/ci.yml)

`PTX Engine` started as the math backbone of **ProtoTracer** - an embedded 3D rendering engine. It is now a stand-alone **header-first** library that runs on:
* 32-bit Cortex-M / ESP32 boards (STM32, Teensy, ESP32-S3)
* Linux / Windows / macOS desktops
* 8-bit AVR boards (Arduino UNO, Nano)

The same codebase powers hardware **and** a full-speed desktop simulator, so you prototype algorithms on the workstation and drop them onto a board unchanged.

> **Work in progress** - the APIs and structure may change without notice while working on the first major release.
>
> This code is currently being migrated from ProtoTracer to act as a submodule and generalize the functionality for more use cases.
> - **Project Goals**:
>     - Have a generic C++ library that can be used as a backbone to speed up development on microcontrollers as well as desktop computers
>     - Re-integrate this functionality as a sub-module into ProtoTracer
>     - Create a desktop GUI application that can utilize and simulate the same functionality as the embedded pipeline. 

---

## Features (2025-07 snapshot)

> WIP

| Domain                  | Capabilities (all header-first unless noted)                                                                                              |
| ----------------------- | ----------------------------------------------------------------------------------------------------------------------------------------- |
| **Math (core)**         | Vec2/3, Mat3/4, quaternions, axis<->angle, Euler orders, transforms, projection helpers, interpolation                                      |
| **Geometry (analytic)** | Circle, ellipse, rectangle, oriented triangle, sphere, cube, plane + SAT/AABB overlap tests                                               |
| **Spatial**             | Fixed-capacity quadtree (header-only), easy swap-in for legacy API                                                                        |
| **Signal / DSP**        | Radix-2 FFT, Kalman / running / peak / derivative filters, voice FFT detector, Simplex noise, function generator                          |
| **Control & Timing**    | PID controller, damped spring, bounce physics, timestep helper, non-blocking Wait, easy-ease animator                                     |
| **Physics systems**     | Boundary motion simulator, 2-D vector-field, generic physics simulator                                                                    |
| **Rendering (core)**    | Camera hierarchy, pixel buffer, tile CPU rasteriser, WIP path/ray tracer                                                                  |
| **Materials & Shaders** | Image / image-sequence, Phong, procedural-noise, combine masks, animated material graph; shader interface                                 |
| **Post-processing**     | Fixed-array compositor + built-in effects (fisheye, radial/gaussian blur, glitch, phase offset, etc.)                                     |
| **Scene graph**         | Entities, meshes, text builder, lights, animation tracks, timeline, volumetric light support                                              |
| **Mesh deform**         | Blend-shape controller, mesh aligner, triangle-group deformers                                                                            |
| **Assets / IO**         | Static & dynamic triangle groups, STL-style index groups, fonts, images, image sequences, voxel density fields                            |
| **Platform utils**      | Cross-platform Console helper (Serial / std::cout), Random helper (Arduino `random()` <-> `rand()`), Cast helper, µString, time abstraction |
| **Apps**                | Project wrapper + stub desktop GUI (simulation pipeline in progress)                                                                      |
| **Portability focus**   | No STL in hot loops, raw arrays, compile-time capacities; builds on AVR, ESP32, RP2040, STM32, Linux/Win/macOS                            |

*Everything above compiles with **-std=c++17** on MCU tool-chains and desktop GCC.*

---

## Planned / in-progress

* 4×4 matrix + projection utilities for graphics pipelines
* Additional procedural noise
* Doxygen-based website hosted on GitHub Pages
* BVH builder + on-device path tracer (`ray/`)
* More procedural noise
* Binary asset packer + CLI viewer

---

## Directory layout

```
lib/ptx/
|-- app/                     top-level runtime & project wrapper
|   |-- app.hpp
|   |-- project/             project-file I/O
|       |-- project.hpp
|       |-- project.cpp
|
|-- assets/                  load-once, immutable data
|   |-- font/                bitmap / vector font tables
|   |-- image/               stills & sequences
|   |-- model/               mesh + triangle-group containers
|   |-- volume/              density / voxel fields
|
|-- core/                    reusable engine subsystems
|   |-- color/               RGB565, gradients, color-space helpers
|   |-- control/             PID, springs, bounce
|   |-- geometry/            analytic shapes + spatial structures
|   |   |-- 2d/              circle, ellipse, rectangle, triangle, overlap
|   |   |-- 3d/              cube, sphere, plane, triangle
|   |   |-- spatial/         quadtree, KD-tree (WIP)
|   |-- math/                vectors, matrices, quaternions, transforms
|   |-- platform/            cross-platform console, random, ustring, time
|   |-- signal/              FFT & filter toolbox
|   |-- time/                timestep + non-blocking wait
|   |-- utils/               casthelper and misc header-only helpers
|
|-- systems/                 high-level engine systems
|   |-- physics/             boundary / vector-field simulators
|   |-- render/              full render stack
|       |-- core/            camera, pixel buffer
|       |-- engine/          renderer facade
|       |-- material/        material graph + PBR, image, noise
|       |-- post/            screen-space compositor & effects
|       |-- raster/          CPU rasteriser helpers
|       |-- ray/             path / ray tracer (WIP)
|       |-- shader/          shader interfaces & prototypes
|
|-- scene/                   run-time scene
    |-- entity.hpp
    |-- mesh / text / lighting sub-modules
    |-- animation/           keyframes, tracks, easy-ease animators
    |-- deform/              blend-shapes, mesh alignment & deformation

```

---

## Quick-start (desktop)

```bash
git clone https://github.com/CoelaCant/PTXEngine.git
cd PTXEngine
# compile
# run test program for native
````

>WIP - no test programs yet

---

## Quick-start (embedded | PlatformIO)

```bash
# build and run unit tests for native
pio test -e native

# compile example for native
pio run -e native
```

---

## Contributing & security

Please read **[CONTRIBUTING.md](CONTRIBUTING.md)** for style rules (lower-case file names, Upper CamelCase funcs, no STL on hot paths, etc).

---

## License

`PTXEngine` is released under the [AGPL-3.0](https://choosealicense.com/licenses/agpl-3.0/). If you ship modified versions (embedded or desktop) you must publish the modified source, ensuring improvements stay in the commons.
