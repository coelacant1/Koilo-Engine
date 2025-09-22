# PTX Python reflection demo

This folder contains a small Python demo that exercises the PTX reflection
registry through a ctypes-based wrapper. The wrapper lives in the Python package
as `ptx.reflection` and uses the C ABI exposed by the project's reflection
shared library (built by the `nativereflectlib` PlatformIO environment).

This README documents how to build the shared library, how the Python wrapper
finds and loads it, and how to run the demo. The Python wrapper is self-contained
and does not require the separate `ptx_c_api` directory to be installed -- the
wrapper will load whatever reflection-compatible shared library you point it at.

## Build the shared library

From the repository root run:

```bash
platformio run --environment nativereflectlib
```

The built shared object will be placed at:
```
.pio/build/nativereflectlib/ptx_reflect.so
```

## ctypes wrapper (`ptx.reflection`)

- The Python helper is implemented in `lib/ptx_python/ptx/reflection.py`.
- It locates the reflection shared library using this order:
  1. The `PTX_C_API_LIB` environment variable (explicit path),
  2. Platform-specific library name lookup via `ctypes.util.find_library`,
  3. A set of repository-relative fallback locations (the PlatformIO build output).
- Once loaded, the wrapper exposes `PTXReflection`, `PTXClass`, `PTXField`,
  `PTXMethod`, `PTXConstructor`, and `PTXObject` classes which map the C API
  metadata into Python-friendly helpers.

## Running the demo

From this folder run the reflection demo with the built shared library path:

```bash
cd lib/ptx_python
PYTHONPATH=../../src python3 reflection_demo.py --lib ../../.pio/build/nativereflectlib/ptx_reflect.so
```

Or export `PTX_C_API_LIB` with the full path and run the demo without the `--lib` flag:

```bash
export PTX_C_API_LIB=/full/path/to/ptx_reflect.so
PYTHONPATH=../../src python3 reflection_demo.py
```

## Notes and troubleshooting

- If the demo reports an empty registry, ensure you built the `nativereflectlib`
  environment which runs the generator that forces all `Class::Describe()` calls
  into the final shared library. The `ptx_reflect.so` produced by that environment
  includes the backend and registration hook.
- If Python fails to load the shared object (`OSError`), confirm the path and run
  `ldd .pio/build/nativereflectlib/ptx_reflect.so` to see unresolved dependencies.
- The Python wrapper expects the shared library to export the functions declared in
  the registry C header; if you build a different target that doesn't include the
  registry exports the demo will fail to find symbols.

## Examples and exploration

- The demo constructs an `RGBColor` instance through reflection, reads and writes
  fields, and invokes a method to mutate the instance. The wrapper is intentionally
  minimal -- it boxes Python scalars to C values for method calls and returns simple
  Python values where possible.

## Notes:
- The reflect build produces a shared object in the PlatformIO build directory for the `nativereflectlib` env.
- The test script constructs an `RGBColor` and manipulates fields through the C API.

```bash
# Exact call for reference
cd /home/coela/Documents/GitHub/PTXEngine/lib/ptx_python
PYTHONPATH=../../src python3 reflection_demo.py --lib ../../.pio/build/nativereflectlib/ptx_reflect.so
```