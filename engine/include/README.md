# Public Headers

This directory (`engine/include`) contains the public C++ headers for the PTX Engine. All exported engine API headers live under the `ptx/` prefix (i.e. this directory has a `ptx/` child) and are intended to be consumed via angle bracket includes of the form:

```cpp
#include <ptx/core/math/vector3d.hpp>
```

## Umbrella Header

An automatically generated umbrella header is produced at CMake configure/build time:

```
<build>/generated/ptx/ptxall.hpp
```

That header simply includes (via relative includes) every header discovered under `engine/include/ptx`. It is used to force translation units to see all reflection macros / static registration points during the reflection generation step.

You should not commit the generated umbrella; it lives only in the build tree. If you add or remove headers, rebuild (or touch CMakeLists / run the generation target) to regenerate `ptxall.hpp`.

## Include Style Policy

* Prefer `<ptx/...>` angle includes inside implementation files.
* Do not rely on relative `..` climbs between engine subsystems after the restructure.
* The build system adds both `engine/include` and `engine/include/ptx` to the public include path to allow legacy paths during migration, but new code should always use the `<ptx/...>` form.

## Reflection & Generation

The reflection code generation process scans headers here and produces:

* `generated/reflection_entry_gen.cpp` - translation unit that forces static registration.
* `generated/ptx/ptxall.hpp` - umbrella include list.
* `generated/.ptx_reflect_cache.json` - JSON cache with header fingerprints and metadata to skip unchanged headers on subsequent builds.

Flags passed to the generator (see `scripts/`):

* `--cache-file` points at the JSON cache in the build tree.
* `--reparse-all` forces ignoring the cache.
* `--no-cache` bypasses reading/writing the cache.

Headers that contain heavy templates or virtual members may be skipped according to filtering logic to keep generation fast.

## Adding New Headers

1. Place the header under `engine/include/ptx/...` in an appropriate subsystem directory (e.g. `core/math`).
2. Add its implementation (if any) under `engine/src/...` mirroring the structure.
3. Rebuild - the umbrella header and reflection pass will pick it up automatically.

## Future Work

* Add a script to validate that all public headers compile standalone with a trivial TU (header hygiene check).
* Provide an install target exporting only public headers and a CMake package config.

