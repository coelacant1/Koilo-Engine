# PTX Engine - C++17 Math & Render Core for Microcontrollers **and** Desktops

[![compilecheck](https://github.com/coelacant1/PTXEngine/actions/workflows/ci.yml/badge.svg)](https://github.com/coelacant1/PTXEngine/actions/workflows/ci.yml)

`PTX Engine` is a header-first C++17 library providing a compact math, geometry
and rendering core designed to run on both microcontrollers and desktop
platforms. The project focuses on portability, low-overhead primitives, and a
small footprint suitable for embedded use while maintaining a full-featured
desktop simulator for development.

This project started as the backbone of `ProtoTracer` - an embedded 3D rendering engine. It is now a stand-alone **header-first** library that runs on:
* 32-bit Cortex-M / ESP32 boards (STM32, Teensy, ESP32-S3)
* Linux / Windows desktops
* 8-bit AVR boards (Arduino UNO, Nano)

> **Work in progress** - the APIs and structure may change without notice while working on the first major release.
>
> This repository is actively being developed. Recent work focuses on a runtime
reflection system, a C ABI layer for bindings, and a ctypes-based Python helper
to explore reflection metadata from Python.

## Key focuses
- Header-first, portable C++17 code usable on MCUs and desktops
- Reflection system for runtime discovery/invocation of types and members
- Tooling to build a self-contained `ptx_reflect.so` that embeds the core
    library and reflection registration
- A small Python `ctypes` wrapper to load and exercise reflection from Python

## Status
- Core library: in active development, most subsystems compile header-first
- Reflection: working - generator and build scripts produce `ptx_reflect.so`
- Python: ctypes wrapper + demo available in `lib/ptx_python`

## Quick start (build reflect shared library)

From repository root:

```bash
# Build the reflect shared library which links a static libptx into ptx_reflect.so
platformio run --environment nativereflectlib
```

The produced shared library will be at:

```
.pio/build/nativereflectlib/ptx_reflect.so
```

Using the Python ctypes wrapper (quick demo)

1. Change to the Python demo folder and run the demo with the built library:

```bash
cd lib/ptx_python
PYTHONPATH=../../src python3 reflection_demo.py --lib ../../.pio/build/nativereflectlib/ptx_reflect.so
```

2. Or export `PTX_C_API_LIB` with the full path and run without `--lib`:

```bash
export PTX_C_API_LIB=/full/path/to/ptx_reflect.so
PYTHONPATH=../../src python3 reflection_demo.py
```

## Developer notes
- The build script `.scripts/BuildSharedReflectLib.py` runs the header
    umbrella generator and creates `src/reflection_entry_gen.cpp` which forces
    `Class::Describe()` calls to ensure registration objects are emitted in the
    final shared object.
- The CMake target `ptx_update_registry` regenerates the reflection umbrella
    header (`generated/ptx/ptxall.hpp`) and updates the PTX registry cache. It
    is built automatically, but you can run it manually with:

    ```bash
    cmake --build build --target ptx_update_registry
    ```
- Static utility namespaces (for example `ptx::Console`, `ptx::Random`,
    `ptx::Time`) expose a nested `Reflection` struct that forwards to their
    static functions so they appear in the registry without requiring
    instances. Follow this pattern for other static-only modules that need
    runtime access.
- The Python wrapper is at `lib/ptx_python/ptx/reflection.py` and exposes
    `PTXReflection` to enumerate classes, create instances, read/write fields,
    and invoke methods.
- For debug: use `ldd .pio/build/nativereflectlib/ptx_reflect.so` (Linux) to
    inspect shared dependencies and `PYTHONPATH=src` when running the demo.

## Contributing
- See `CONTRIBUTING.md` for coding style and platform guidance.

## License
- PTX Engine is released under the AGPL-3.0 license. See `LICENSE` in the
    repository root for details.
