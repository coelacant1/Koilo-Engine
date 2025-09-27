# PTX C API

This directory contains the C-compatible API layer for the PTX reflection/registry system.
The C API is a thin, extern "C" wrapper around the C++ reflection implementation and is
designed to make the runtime registry and basic reflection operations available to C and
foreign-language bindings (for example, Python's `ctypes`, or other FFI systems).

This README documents the purpose, build and usage patterns for the C API and provides a
minimal Python example showing how to load the generated `ptx_reflect.so` and query the
registry.

## Contents
- `reflect_capi.cpp` / `reflect.h` -- C wrapper implementation and header.

## Purpose
- Export a stable, C ABI friendly surface that exposes the global class registry,
	class enumeration, and simple helpers to query class/field metadata.
- Allow non-C++ runtimes (Python, Lua, other languages) to consume reflection metadata
	without linking against the C++ runtime directly.

## Building (CMake)
The reflection C API is produced automatically when you configure and build the project with CMake:

```bash
cmake -S . -B build
cmake --build build -j
```

Artifacts of interest:
* `build/libptx_reflect.so` - shared library exporting the C API (and internal registry symbols).
* `build/libptx_core.a` - static core library.
* `build/generated/ptx/ptxall.hpp` - auto-generated umbrella include (build tree only).

PlatformIO support previously existed (`nativereflectlib` environment); the canonical build is now CMake. PlatformIO scripts may still function but are considered secondary.

## Usage (C)
- Include the header `reflect.h` from this directory in your C program and link
	against `ptx_reflect.so` (or load it dynamically). The C API uses simple pointer
	and integer types to remain ABI-stable.

The header documents available functions and data structures. Typical usage pattern:

1. Load `ptx_reflect.so` (dlopen / LoadLibrary).
2. Obtain the registry handle and query the number of classes.
3. Iterate class descriptors and inspect fields/methods via the C API.

## Usage (Python ctypes)
The Python reflection wrapper (`bindings/python/ptx/reflection.py`) now auto-discovers `libptx_reflect.so` via:
1. `PTX_C_API_LIB` environment variable (explicit path override).
2. `ctypes.util.find_library` on common names (`ptx_reflect`, `libptx_reflect.so`, legacy names).
3. An upward directory walk scanning `build/` directories for `libptx_reflect.so`.

Example (from repo root):
```bash
python bindings/python/reflection_demo.py
```
Optionally:
```bash
PTX_REFLECT_DEBUG=1 python bindings/python/reflection_demo.py  # prints search paths
```

## Exported symbols
- See `reflect.h` for the authoritative list of exported functions. Typical functions
	you may find are:
	- `ptx_registry_count()` -- number of registered classes.
	- `ptx_registry_get(i)` -- get descriptor for ith class.
	- `ptx_class_name(desc)` -- C string class name.
	- `ptx_class_field_count(desc)` -- number of fields.
	- `ptx_class_field(desc, i)` -- get field descriptor.

These names are illustrative; please consult `reflect.h` for exact function and type names.

## Troubleshooting
* Empty registry:
  * Ensure generation ran (look for `generated/reflection_entry_gen.cpp` during build output). CMake custom command invokes the generator before building `ptx_reflect`.
* Missing symbols:
  * Verify you are loading `build/libptx_reflect.so` from the current build directory (not a stale file from an earlier layout).
* Python load error (`OSError`):
  * Run with `PTX_REFLECT_DEBUG=1` to see attempted paths or set `PTX_C_API_LIB=build/libptx_reflect.so` explicitly.
* Cache / stale data:
  * Remove `build/generated/.ptx_reflect_cache.json` and rebuild (or pass `--reparse-all` to the generator) to force a full parse.

## Extending or Embedding
- If you want to expose additional C-friendly functions, add them to `reflect_capi.cpp`
	and declare them in `reflect.h` with `extern "C"` linkage. Keep parameter types
	limited to integers, pointers and POD structs to preserve ABI stability.
