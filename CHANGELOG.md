# Change Log
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](http://keepachangelog.com/)
and this project adheres to [Semantic Versioning](http://semver.org/).

## [0.2.0] - 2025-10-11
Core gameplay systems (ECS, Particles, AI, World Management, Profiling)

### Added
- **Entity Component System (ECS)** (`engine/include/ptx/systems/ecs/`)
  - Entity handles with generation counters for stale handle detection
  - Dense component storage for cache-friendly iteration
  - Template-based type-safe component API
  - Component masks and query system for efficient entity filtering
  - Core components: `TransformComponent` (wraps existing Transform), `VelocityComponent`, `TagComponent`
- **Particle System** (`engine/include/ptx/systems/particles/`)
  - Particle pooling (pre-allocated, zero runtime allocations)
  - Multiple emission shapes: Point, Sphere, Box, Cone, Circle
  - Lifetime-based interpolation for size, color, and alpha
  - Physics simulation (gravity, velocity, acceleration)
  - Burst emission and custom update callbacks
- **AI System** (`engine/include/ptx/systems/ai/`)
  - **Behavior Trees**: Composites (Sequence, Selector, Parallel), Decorators (Inverter, Repeater, Succeeder), Leaves (Action, Condition, Wait)
  - **State Machines**: State callbacks (Enter/Update/Exit), automatic transition checking with conditions
  - **Pathfinding**: A* algorithm on grid, multiple heuristics (Manhattan, Euclidean, Diagonal), terrain costs, diagonal movement support
- **World Management** (`engine/include/ptx/systems/world/`)
  - Level loading/unloading with state machine (Unloaded/Loading/Loaded/Active/Unloading)
  - Level streaming based on viewer position with configurable radius
  - Level metadata and callbacks (OnLoad/OnUnload)
  - `LevelSerializer` for JSON/Binary/XML serialization formats
  - Level/Scene integration (Level can reference Scene for rendering)
- **Profiling Tools** (`engine/include/ptx/systems/profiling/`)
  - **PerformanceProfiler**: Frame timing, FPS tracking, code section profiling with RAII helpers (`PTX_PROFILE_SCOPE`, `PTX_PROFILE_FUNCTION`)
  - **MemoryProfiler**: Allocation tracking by tag, leak detection, peak usage tracking, formatted output (`PTX_TRACK_ALLOC`, `PTX_TRACK_FREE`)
  - Historical statistics (min/max/avg over configurable frame history)
  - Performance and memory reports with percentage breakdowns

### Changed
- Renamed `SceneSerializer` to `LevelSerializer` for architectural clarity (distinguishes Level serialization from Scene rendering)
- `TransformComponent` now wraps existing `Transform` class instead of duplicating fields (maintains single source of truth)
- New files include PTX reflection support (`PTX_BEGIN_FIELDS`, `PTX_BEGIN_METHODS`, `PTX_BEGIN_DESCRIBE`)
- Updated ci.yml to run using build.sh script with individual execution 

### Testing
- Test skeleton generation script available in `scripts/generatetestskeletons.py`
- All systems designed with testable APIs and clear interfaces

### Next Tasks
- Update CMakeLists.txt with new source files in mind
- Fix include path in `level.hpp:16` for `entity.hpp`
- Write unit tests for new systems
- Integration testing: ECS + Particles + AI + World Management
- Performance profiling of existing systems using new profiling tools

## [0.1.7] - 2025-10-03
Runtime filter migrations, container clean-up across scene/physics systems, and updated tooling defaults for the new engine layout.

### Added
- `VectorKalmanFilter::Reset` to reinitialise all component filters with a shared covariance.

### Changed
- Converted Kalman- and running-average-based filters to runtime implementations (`KalmanFilter`, `VectorKalmanFilter`, `QuaternionKalmanFilter`, `VectorRunningAverageFilter`) using STL containers and safer parameter handling.
- Reworked scene and deformation utilities (`Scene`, `MeshDeformer`, `TriangleGroupDeformer`, `MeshAlign`, `BoundaryMotionSimulator`, `VectorField2D`) to use `std::vector` or `std::array` instead of raw allocations, reducing manual memory management.
- Updated rasterizer acceleration structure setup to match the runtime `QuadTree` API and leverage `std::vector` storage for projected triangles.
- Adjusted physics helpers (`BouncePhysics`) for the new velocity filter signature.
- Enhanced build scripts (`BuildSharedReflectLib.py`, `GeneratePTXAll.py`, `UpdatePTXRegistry.py`) to default to `engine/include/ptx`, add defensive include-path discovery, and handle relocated bindings directories.

### Fixed
- Eliminated numerous raw `new`/`delete` paths in filters, physics, and scene helpers that could leak or double-free under error conditions.

### Removed
- Legacy `scripts/migrate_layout.py` helper now that the new engine layout is the default.

## [0.1.6] - 2025-09-27
Build & tooling modernization: migration off PlatformIO for workflows, addition of signature-oriented Lua helpers, hardened Python auto-discovery, reflection parse caching, and documentation updates.

### Added
- CMake-based targets: `ptx_core` (static), `ptx_reflect` (shared), `ptx_lua` (module), `ptx_tests` (unit tests executable)
- Reflection generation custom command producing umbrella header (`ptxall.hpp`) and registry translation unit in build `generated/` directory.
- JSON reflection parse cache (size, mtime, partial hash) with flags to force reparse.
- Lua binding signature helpers (`call_sig`/constructor helpers), embedded RPATH `$ORIGIN`, and post-build copy of `libptx_reflect.so` beside `ptx.so`.
- Python loader upward directory search, extended candidate library names, and `PTX_REFLECT_DEBUG` verbose tracing.
- New documentation: `engine/include/README.md`, `test/README.md`
- Reflection generation now skips heavy/templated or excluded headers and leverages fingerprint cache to avoid redundant clang parses

### Changed
- All public includes now referenced via `<ptx/...>` angle form; relative includes replaced to stabilize modular layout.
- Restructured repository layout (engine/include, engine/src, bindings/{c_api,lua,python}, tests, scripts, generated, external)
- Updated C API README to reflect CMake build, removed PlatformIO assumptions.
- Lua & Python READMEs refactored for new build/discovery model
- Overhauled C API, Lua, Python READMEs

### Fixed
- Lua runtime loading issues by setting module OUTPUT_NAME to `ptx` and copying reflection shared library for co-location discovery.
- Python reflection failures when run outside original build dir via robust upward search and multiple filename candidates.
- Header include failures after restructuring by normalizing include paths.

### Removed
- Reliance on PlatformIO build output for desktop reflection workflows (still will be available for MCU targets where needed)

### Next Tasks
- Finalize descriptor caching layer for Lua & Python hot paths.
- Add string/UString bridging & unified overload signature dispatch.
- Package Python module (wheel / optional pip distribution) and provide install targets.
- Introduce feature toggles for MCU footprint trimming

## [0.1.5] - 2025-09-22
Integration and hardening of the reflection build pipeline, a ctypes-based
Python wrapper for runtime reflection, and a number of safety/formatting fixes.

### Added
- `lib/ptx_python/ptx/reflection.py`
  - A self-contained `ctypes`-based wrapper that loads the reflection shared
    library and provides `PTXReflection`/`PTXClass`/`PTXField` helpers for
    Python usage (demo in `lib/ptx_python/reflection_demo.py`).
- `lib/ptx_c_api/README.md`
  - Documentation for C ABI layer exposing registry functions
- `lib/ptx_python/README.md`
  - Documentation for using the Python ctypes wrapper and demo commands

### Changed
- `.scripts/BuildSharedReflectLib.py` (finalized)
  - Runs `GeneratePTXAll.py`, scans `lib/ptx` headers for reflection describes,
    generates `src/reflection_entry_gen.cpp` that forces `Class::Describe()` calls,
    builds a static `libptx` and links it into `ptx_reflect.so`.
- Header scanner in `.scripts/BuildSharedReflectLib.py` hardened
  - Skips macro-definition lines and comment/span constructs, filters placeholder
    tokens (e.g., `CLASS`), avoids nested/unqualified type names, and attempts
    to qualify names with enclosing namespaces when detected

### Fixed
- Ensure `ptx_reflect.so` contains the full registry at runtime by generating
  and compiling `reflection_entry_gen.cpp` so all `Class::Describe()` statics are
  emitted and initialised when the shared library is loaded

### Notes / Next Tasks
- The header scanner is heuristic-based; consider a C++ parser
- Optionally add a standalone CLI generator to emit `src/reflection_entry_gen.cpp`
  without invoking the full PlatformIO build pipeline


## [0.1.4] - 2025-09-21
Added to .hpp automatic macro configuration to handle additional edge cases

### Added
- reflect_capi.cpp
  - Backend link between C ABI -> C++ PTX library
- reflect.h
  - Header for C ABI/API
- tasks.json
  - Task file for setting up header smoke test (unfinished)
  - Task for initializing/setting up the python workspace/venv
  - Task for updating the PTX registry macros in all of the .hpp files
- BuildSharedReflectLib.py
  - Working on build script for C ABI
- PTXEngine virtual environment with automatic configuration when called via task

### Changed
- Added detection for span comments to ignore "class" in comments
- Added detection for macro calls to avoid recursive calls
- Handles pure virtual/template classes now
- Added macro block validation to ensure no duplication/bad formatting
- Added overload macro automation
- Added function to remove current macro blocks
- Added subtree exclusions for registry and platform
- Changed .gitignore to include new .scripts/ __pycache__ excludes

### Next Tasks
- Modify template classes and convert to runtime versions
- Finish C api and implement test
- Validate overload calls


## [0.1.3] - 2025-09-14
Automatic .hpp macro configuration

### Added
- UpdatePTXRegistry.py
  - Automatically generate and link PTX macro functions to initialize registry

### Changed
- All .hpp files
  - Now include base macro calls for registry initialization
  - Overloaded functions not fixed, require manual tweaking
  - Added reflect_macros.hpp include to all .hpp files

### Next Tasks
- Fix all overloaded functions, macro calls will fail due to overlap


## [0.1.2] - 2025-09-13
Updated reflection for C++

### Added 
- global_registry.hpp
  - list of class description objects

### Changed
- demangle.hpp
  - Simplified
- reflect_helpers.hpp
  - Improved invokers
  - Streamlined field and method searching
  - Added Pretty printing for demangled strings for types/signatures/constructors
- reflect_macros.hpp
  - Streamlined
  - Implemented via global registry
  - Implemented constructor creation
- reflect_make.hpp
  - Major restructure
  - Implemented custom type span creation
  - Changed invokers to use box return and new apply tuple functions
  - Simplfied method/static methods/const methods

### Removed
- reflectable.hpp
  - No longer necessary with new implementation

### Next Tasks
- Implement UpdatePTXRegistry.py to automatically configure C++ headers for new registry macros 
  - Pay attention to ignoring operators/etc
- Implmement ptx_lua
- Implement ptx_capi/reflect_capi (cpp mapping side)
- Implement ptx_platform for library static/library linking for runtime initialization via capi on mcus and posix


## [0.1.1] - 2025-09-08
First working end-to-end reflection pipeline: discover -> access -> invoke.

### Added
- registry.hpp
  - Define FieldDecl, MethodDesc, FieldList, MethodList
  - Add FieldAccess helpers for safe get/set
  - Provide simple demangle wrapper for type names
- reflectable.hpp
  - Base CRTP template Reflectable<T> with static Fields() and Methods() hooks
- reflect_make.hpp
  - Generic helpers (make_field, make_method, make_smethod)
  - Type-safe argument unpacking & return boxing (works for void + value returns)
  - Removes need for N-arg boilerplate macros
- reflect_macros.hpp
  - Macros PTX_FIELD, PTX_METHOD, PTX_SMETHOD as thin sugar over make_*
  - PTX_BEGIN_FIELDS/METHODS & PTX_END_FIELDS/METHODS for static arrays
  - Simplified and future-proof (handles const/non-const, static, multiple args)

### Changed
- Updated RGBColor to declare reflected fields (R, G, B) and methods (Scale, Add, HueShift, ToString, InterpolateColors)
- Runtime registry now enumerates properties & methods dynamically

### Testing
- registry_test.cpp:
  - Enumerates fields with demangled type names
  - Dynamically reads/writes fields (R, G, B)
  - Dynamically invokes methods (Add, Scale, HueShift, ToString, InterpolateColors)
  - Prints expected round-trip values


## [0.1.0] - 2025-08-18

### Changed
- Renamed to PTX (ProtoTracer eXtended)


## [0.0.7] - 2025-08-18
 
Implementing frontend shared memory and virtual controller
 
### Added
- Added numerous shaders and materials
- Added virtual controller
- Added shm ipc for frontend
- Added basic project for testing shm virtual controller
- Began converting post effects/compositor
 
### Changed
- Fixed ishader/imaterial implementation
- Fixed post processor/effect
- Fixed commenting
- Removed unnecessary files

### Next Tasks
- Import Phong information from Scene
- Implement hasUV under rasterizer
- Render core colorspace
- Assets density field?
- Write ray trace function
- Finish FFT implementation
- Extend print options in console
- Implement timeline
- Implement animator
- Implement entity


## [0.0.6] - 2025-07-26
 
Building out and decoupling functionality
 
### Added
- Extended RGBColor functionality
- Added Reflect function to Vector3D
- Added Phong, ProceduralNoise, Gradient, Normal Materials/Shaders
 
### Changed
- Removed hardcoded prints and replaced with helper class calls
- Fixed AnimationTrack/KeyFrameTrack
- Fixed MaterialT errors
- All headers compile except for final materials/shaders

### Next Tasks
- Implement post effects/compositors
- Implement rest of shaders/materials
- Import Phong information from Scene
- Implement hasUV under rasterizer
- Render core colorspace
- Assets density field?
- Write ray trace function
- Finish FFT implementation


## [0.0.5] - 2025-07-09
 
Building out and decoupling functionality
 
### Added
- Added interface layer for images/image sequences -> materials
- Script for building header containing all headers for test builds
- Script to compile each header individually as a an individual translation unit
- Added CorrectPaths.py to validate include paths in header files
- Added CI for running a smoke test on header compilation for all header files
- Added inclomplete shader implementations
 
### Changed
- Renamed from um3d to ptx for more generic end functionality

### Next Tasks
- Implement post effects/compositors
- Implement rest of shaders/materials
- Implement hasUV under rasterizer
- Render core colorspace
- Assets density field?
- Write ray trace function


## [0.0.4] - 2025-07-02
 
Building out and decoupling functionality
 
### Added
- Added interface layer for images/image sequences -> materials
 
### Changed
- Implemented interface for materials + shaders (needs tweaked)
- Restructed to App/Assets/Core/Systems
- Added raster triangle for optimized layer ontop of the geometric triangle
- Split effect from scene

### Next Tasks
- Implement post effects/compositors
- Implement rest of shaders/materials
- Implement hasUV under rasterizer
- Render core colorspace
- Assets density field?
- Make ray trace function


## [0.0.3] - 2025-06-25
 
Building out and decoupling functionality
 
### Added
- Added interface layer for images/image sequences -> materials
 
### Changed
- Migrated .h -> .hpp
- Finalized shapes
- Updated paths
- Changed pipeline for rendering -> engine to renderer -> ray/rasterize
- Removed virtual function from animated material to extend material more generically
- Object3D -> Mesh (mesh deformers, etc)
- Morph -> blendshape
- MorphTransform -> blendshape controller
- 

### Next Tasks
- Verify if GetMaterial under animated material needs to exist
- Assets density field?
- Render core colorspace
- Render material shadermaterial
- Mesh align -> use triangle 3d for rasterizing?
- Material shaders
- Make ray trace function to replace rasterize
- Split effect from scene and add to compositor
  - Render post


## [0.0.2] - 2025-06-18
 
Building out and decoupling functionality
 
### Added
- GitHub markdowns for issues/funding/pr templates/contributing/etc
- Importing rendering and rasterizing backend from ProtoTracer
    - Upper Camel Case files are unconverted
    - Empty files are in-progress
- Change Log
 
### Changed
- Decoupled functionality for Triangle 2D/3D geometry classes
- PlatformIO ini for support on embedded/non-embedded systems


## [0.0.1] - 2025-06-17
 
Building out and decoupling functionality
 
### Added
- Imported base mathematics, signaling, geometry, control, time classes


## [X.X.X] - 2025-06-18

This is a template
 
### Added

### Changed
 
### Fixed

### Removed

### Testing

### Next Tasks
