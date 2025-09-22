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

## Building
- The C API is built as part of the `nativereflectlib` PlatformIO environment. The
	project builds a static `libptx` and links it into `ptx_reflect.so`, which contains
	the C API exports.

To build the reflect shared library from the repository root run:

```bash
platformio run --environment nativereflectlib
```

After a successful build the shared library is available at:

```
.pio/build/nativereflectlib/ptx_reflect.so
```

## Usage (C)
- Include the header `reflect.h` from this directory in your C program and link
	against `ptx_reflect.so` (or load it dynamically). The C API uses simple pointer
	and integer types to remain ABI-stable.

The header documents available functions and data structures. Typical usage pattern:

1. Load `ptx_reflect.so` (dlopen / LoadLibrary).
2. Obtain the registry handle and query the number of classes.
3. Iterate class descriptors and inspect fields/methods via the C API.

## Usage (Python ctypes)
- A small Python loader is provided in the repository (`src/ptx/reflection.py`) which
	demonstrates how to use `ctypes` to load `ptx_reflect.so`. See `lib/ptx_python/reflection_demo.py`
	for a working example. The important points are:

- Ensure the `src` directory is on `PYTHONPATH` so that the Python `ptx` package can
	be imported without installing.
- Pass the full path to `ptx_reflect.so` when running the demo, for example:

```bash
cd lib/ptx_python
PYTHONPATH=../../src python3 reflection_demo.py --lib ../../.pio/build/nativereflectlib/ptx_reflect.so
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
- Empty registry after loading the shared library:
	- This typically happens when the library was built without linking the translation units
		that contain the static registration objects (the code-generation step that forces
		`Class::Describe()` calls may be required). Use the `nativereflectlib` build environment
		which runs the generator and embeds the `reflection_entry_gen.cpp` TU that forces
		registration.

- Missing symbols at runtime:
	- Ensure you build `nativereflectlib` and are pointing to the `ptx_reflect.so` produced
		by that build. If you built a different target, exports may differ.

- Loading from Python fails with `ImportError` / `OSError`:
	- Verify the path to `ptx_reflect.so` is correct and the shared object dependencies are
		resolvable on your platform. Use `ldd .pio/build/nativereflectlib/ptx_reflect.so` to inspect
		runtime dependencies.

## Extending or Embedding
- If you want to expose additional C-friendly functions, add them to `reflect_capi.cpp`
	and declare them in `reflect.h` with `extern "C"` linkage. Keep parameter types
	limited to integers, pointers and POD structs to preserve ABI stability.
