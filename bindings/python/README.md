# PTX Python Reflection

Python access to the PTX reflection registry via a thin `ctypes` wrapper. The
wrapper (`ptx.reflection`) dynamically loads the engine's reflection shared
library and exposes small helper classes for enumerating classes, fields,
methods, and constructing objects.

## Build (CMake)

Build the engine (shared reflection library is produced as part of the CMake
targets). From the repository root:

```bash
cmake -S . -B build -DPTX_BUILD_PYTHON=ON
cmake --build build -j
```

Result (Linux example): `build/libptx_reflect.so`

No PlatformIO step is required; previous `.pio/` paths are obsolete.

## Loader Overview (`ptx.reflection`)

Implemented in `bindings/python/ptx/reflection.py`.
Discovery order when you call `ptx.reflection.load_reflection()`:
1. `PTX_REFLECT_LIB` environment variable (explicit base name or full filename).
2. Platform `ctypes.util.find_library` lookup on a set of candidate base names.
3. Upward directory walk from the module location probing common build
   directories (`build/`, `build/lib/`).
4. Fallback legacy names (`ptx_c_api`, `libptx_c_api.so`).

Enable trace logging of each attempted path with:
```bash
export PTX_REFLECT_DEBUG=1
```

Key exported Python classes:
* `PTXReflection` - root handle / enumeration.
* `PTXClass`, `PTXField`, `PTXMethod`, `PTXConstructor`, `PTXObject`.

## Running the Demo

From the repository root (no explicit `--lib` needed if build tree is nearby):
```bash
python bindings/python/reflection_demo.py
```

Force a specific library (e.g. testing alt build):
```bash
PTX_REFLECT_LIB=ptx_reflect python bindings/python/reflection_demo.py
```

Verbose search trace:
```bash
PTX_REFLECT_DEBUG=1 python bindings/python/reflection_demo.py
```

## Troubleshooting

Empty registry:
* Ensure the generation step ran (CMake custom command should have emitted the
  umbrella header and registry translation unit). A clean rebuild may help.

Load error (OSError / cannot open shared object):
* Export `PTX_REFLECT_DEBUG=1` to see attempted paths.
* Verify the library exists: `ls build/libptx_reflect.so`.
* On Linux, inspect dependencies: `ldd build/libptx_reflect.so`.

Missing symbols:
* Confirm you're loading `libptx_reflect.so` and not a stripped core-only lib.

## Next Steps / TODO
* Optional wheel / package metadata for `pip install` flows.
* Type-hinted higher-level façade (auto-marshalling complex aggregates).
* Metadata caching to reduce per-call FFI overhead.
* Signature-based overload selection helper (parallel to Lua sugar layer).

## Minimal Example (Inline)
```python
from ptx.reflection import load_reflection
r = load_reflection()  # auto-discovery
print("Classes:", [c.name for c in r.iter_classes()])
rgb = r.class_by_name('RGBColor')
ctor = next(iter(rgb.constructors))
obj = ctor.construct(255, 0, 0)
print('R field value before:', obj['R'].read())
obj['R'].write(128)
print('R field value after:', obj['R'].read())
```