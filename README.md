# Koilo Engine

[![Build/Test Validation](https://github.com/coelacant1/KoiloEngine/actions/workflows/ci.yml/badge.svg)](https://github.com/coelacant1/KoiloEngine/actions/workflows/ci.yml)

C++17 game engine targeting desktop and embedded platforms. Scenes and game logic are written in KoiloScript (`.ks`), a custom bytecode-compiled scripting language with full access to engine internals through a runtime reflection system. No recompilation needed to change content.

**Platforms**: Linux, Windows (OpenGL 3.3 + software rasterizer), Teensy 4.x, ESP32-S3, STM32 (software rasterizer only).

**License**: GPL-3.0-or-later


![OBJ Scene](assets/screenshots/obj_scene_1.png)

> [!WARNING]
> This is heavily work-in-progress and still is ridden with bugs. I will be rolling out more features and changes and the backend is not yet stable. If you decide to use this to make a game you will find limitations in the scripting engine, I will be implementing a runtime testing suite to test more functionality.

---

## Building

```bash
./build.sh

# Or manually
cmake -S . -B build -G Ninja
cmake --build build --parallel $(nproc)

# Run tests
./build/koilo_tests
./build/koilo_script_language_tests
```

| Build Option | Default | Description |
|--------------|---------|-------------|
| `KOILO_ENABLE_AI` | ON | Pathfinding, behavior trees, FSM |
| `KOILO_ENABLE_AUDIO` | ON | SDL2 audio backend |
| `KOILO_ENABLE_PARTICLES` | ON | Particle system |
| `KL_BUILD_TESTS` | ON | Unit tests |
| `KL_BUILD_EXAMPLES` | ON | Example programs |

On hybrid GPU laptops (Linux NVIDIA PRIME):
```bash
__NV_PRIME_RENDER_OFFLOAD=1 __GLX_VENDOR_LIBRARY_NAME=nvidia ./build/examples/stress_test
```

---

## KoiloScript

KoiloScript is a dynamically-typed scripting language compiled to bytecode at load time. The VM uses NaN-boxing (8 bytes per value) with a 256-slot stack and 64-frame call depth.

```
DISPLAY { width: 800, height: 600 }

var scene = Scene()
var cam = Camera()
cam.SetPerspective(60.0, 0.1, 100.0)

fn Update(dt) {
    var t = scene.GetTime()
    cam.SetPosition(sin(t) * 5.0, 2.0, cos(t) * 5.0)
    cam.SetTarget(0.0, 0.0, 0.0)
    scene.Render(cam)
}
```

Language features: variables, functions, if/else/while/for, classes with fields and methods, operator overloading, signals and event broadcasting, coroutines with yield, multi-file imports, 35 built-in functions (math, string, debug, profiler). Scripts access C++ objects through the reflection bridge - any reflected class is callable from script code.

See `docs/scripting/koilo_script_spec.md` for the full language specification.

---

## Rendering

Two render backends behind a common `IRenderBackend` interface:

**GPU (OpenGL 3.3)** - Hardware-accelerated rendering with an uber-shader architecture. All KSL shaders compile into a single GL program, selecting variants via uniform ID. Dirty mesh tracking skips redundant uploads. Batch rendering sorts by material to minimize state changes.

**Software Rasterizer** - CPU scanline rasterizer for embedded and fallback. Edge-walking with incremental barycentric coordinates. Conditional attribute interpolation via per-triangle `attribMask`. Material sort for I-cache coherency. Runs on MCUs without a GPU.

Both backends produce a `Color888` framebuffer. Post-processing, particle compositing, and UI overlay run after the main render pass.

### KSL Shaders

Koilo Shader Language shaders are C++ headers (`.ksl.hpp`) that compile for both paths. A codegen step produces GLSL for GPU and ELF binaries for CPU from the same source. 19 shaders ship with the engine: PBR, Phong, procedural noise, gradients, image/texture sampling, audio-reactive visualizations, and more.

See `docs/shaders/ksl.md` for the shader system reference.

---

## Reflection

Any C++ class adds a `KL_BEGIN_DESCRIBE`/`KL_END_DESCRIBE` block to expose fields, methods, and constructors to scripts at runtime. No external codegen tools. Type-aware method overload resolution, operator overloading, nested field access (`obj.field.Method()`), and complex return types are all supported.

See `docs/reflection/reflection.md`.

---

## Modules

The engine supports dynamic modules - shared libraries (`.so`/`.dll`) loaded at runtime through `ModuleLoader`. Modules register with a phase that controls init/update order (Core, System, Render, Overlay). Hot-reload is supported on desktop. The C module SDK (`koilo_module_sdk.h`) provides lifecycle hooks and engine service access.

Engine subsystems like physics, audio, particles, and AI are compiled into the engine directly and toggled via CMake flags - they are not dynamic modules, but they expose the same script-accessible interface through the reflection system.

See `examples/modules/` for working module examples and `docs/modules/custom_modules.md` for the SDK reference.

---

## Scene Graph and ECS

The scene graph handles spatial hierarchy (parent/child transforms, culling). The ECS handles logic iteration with dense `ComponentArray<T>` storage and 64-bit component masks for bitwise queries. `EntityID` uses index + generation counter to detect stale references.

Components: Transform, Velocity, Tag, and any custom component registered through reflection.

---

## Math Library

Vectors (2D, 3D), quaternions, 4x4 matrices, transforms, Euler angles, axis-angle, yaw-pitch-roll. Geometric types: ray, AABB, circle, sphere, triangle, rectangle, ellipse, plane, cube. Spline paths (1D and 3D with Catmull-Rom interpolation). Spatial indexing via QuadTree.

Signal processing: Simplex noise, Kalman filters (vector and quaternion variants), FFT voice detection, running average filters.

Color types with lerp interpolation.

---

## Asset Pipeline

**KoiloMesh** (`.kmesh` / `.kmeshb`) - Custom mesh format with ASCII and binary representations. Supports morph targets (blend shapes) with sparse vertex deltas. OBJ and FBX files are converted to KoiloMesh offline via `tools/asset-conversion/convert_to_kmesh.py`. The engine loads `.kmesh` at runtime, not raw model formats. See `docs/assets/kmesh.md`.

**Textures** - Image loading into RGB888 format for both GPU and software paths.

---

## Animation

Skeleton animation playback, keyframe camera tracks with Catmull-Rom interpolation and shortest-path yaw handling, material parameter animation over time, morph target blending with independent per-target weights.

---

## Examples

| Example | Description |
|---------|-------------|
| `scene_3d` | 3D spinning cube with multiple KSL materials, orbiting camera, perspective projection |
| `space_shooter` | 2D top-down shooter with imports, signals, coroutines, collision, wave spawning |
| `stress_test` | 79 meshes with diverse KSL materials, GPU vs software comparison |
| `obj_scene` | OBJ model loading with textures, keyframe camera track, free-fly override |
| `syntax_demo` | KoiloScript language feature showcase |
| `module_demo` | Dynamic module loading with runtime reflection |

![Scene 3D](assets/screenshots/scene_3d.png)
![Stress Test](assets/screenshots/stress_test.png)
![OBJ Scene](assets/screenshots/obj_scene_2.png)
![Space Shooter](assets/screenshots/space_shooter.png)
![Syntax Demo](assets/screenshots/syntax_demo.png)
![Module Demo](assets/screenshots/module_demo.png)

---

## Tools

| Tool | Location | Description |
|------|----------|-------------|
| KSL Compiler | `tools/ksl/` | Shader codegen (C++ to GLSL) and ELF compilation |
| Asset Converter | `tools/asset-conversion/` | FBX/OBJ to KoiloMesh conversion |
| Module Compiler | `tools/koilo-compile-module/` | Module scaffold and build scripts |
| VS Code Extension | `tools/vscode-koiloscript/` | Syntax highlighting for `.ks` files |
| Blender Addon | `tools/addons/` | Blender integration |

---

## Documentation

| Document | Description |
|----------|-------------|
| `docs/architecture/overview.md` | Engine architecture and data flow |
| `docs/architecture/optimizations.md` | Performance techniques across all backends |
| `docs/scripting/koilo_script_spec.md` | KoiloScript language specification |
| `docs/shaders/ksl.md` | KSL shader system |
| `docs/reflection/reflection.md` | Reflection system |
| `docs/modules/custom_modules.md` | Module SDK and dynamic loading |
| `docs/assets/kmesh.md` | KoiloMesh file format |

---

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md) for coding standards and PR guidelines.
