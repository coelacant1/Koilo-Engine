# Change Log
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](http://keepachangelog.com/)
and this project adheres to [Semantic Versioning](http://semver.org/).

## [0.3.2] - 2026-3-14
UI code quality: hpp/cpp splits, coding standards fixes, dirty-flag rendering (attempt, not finished), and widget bug fixes. End of the staged changes - working to keep the upstream repository up-to-date (even if broken).

### Changed
- **hpp/cpp splits** - Separated header-only UI files into proper hpp/cpp pairs for faster incremental builds and cleaner interfaces:
  - `ui_context.hpp/cpp` (core context, event handling, widget management)
  - `draw_list.hpp/cpp` (draw command generation and widget emitters)
  - `kml_builder.hpp/cpp` (KML markup parser and widget construction)
  - `kss_parser.hpp/cpp` (KSS stylesheet parser)
  - `layout.hpp/cpp` (flexbox-style layout engine)
  - `event.hpp/cpp` (hit testing and event dispatch)
  - `theme.hpp/cpp` (style resolution and theme management)
  - `canvas2d.hpp/cpp` (Canvas2D immediate-mode draw context)
  - `color_picker.hpp/cpp` (HSV color picker panel)
- Aligned all UI source files to project coding standards
- Dropdown popup rendering deferred to overlay pass so menus draw above sibling widgets

### Fixed
- Text input corruption (character reordering and dropped characters)
- Cursor position drift in text fields growing with text length
- Slider and toggle switch sluggish response from per-pixel dirty marking
- Shadow opacity too dark from additive overlap of multi-step blur rects
- `rgba()` alpha values in KSS parsed as 0-1 float now correctly mapped to 0-255
- Number spinner vertical sizing asymmetry
- Tree node showing expand icon on leaf nodes with no children
- Dropdown items not clickable (hit test returning child labels instead of dropdown)

### Added
- **Dirty-flag rendering** - UI only redraws when widget state changes
- `SetRenderDirty()` / `IsRenderDirty()` API on UIContext for explicit redraw requests
- `MarkDirty()` propagation through widget hierarchy on state changes
- ColorField hex value parsing from KML `value` attribute
- `SetColorHex()` scripting API for programmatic color field updates
- `onChange` callback support for text fields

## [0.3.1] - 2026-3-12
UI system, rendering fixes, and naming cleanup.

### Added
- **UI system** (`engine/koilo/systems/ui/`)
  - KML (Koilo Markup Language) and KSS (Koilo Style Sheet) for declarative UI
  - Custom TTF renderer for UI text; upgrade from characters used in previous Canvas2D examples
  - 23 widget types: panel, button, label, slider, checkbox, dropdown, treenode, splitpane, floatingpanel, and others
  - GPU-accelerated and software rendering paths
  - CSS-like selectors, pseudo-classes, variables, media queries, and transitions
  - Hierarchy tree view with expand/collapse and connector lines (not finished)
  - Floating panel system with minimize, close, and bottom tray
  - UI demo example (`examples/ui_demo/`)
  - Reference documentation (`docs/ui/kml_kss.md`)

### Changed
- Software render path now swaps immediately after present for reliable display on Wayland

## [0.3.0] - 2026-3-8
Major architectural restructure: PTX -> Koilo rename, unified engine layout, KoiloScript Language shader system, and new platform backends.

### Added
- **KoiloScript Language (KSL)** (`engine/koilo/ksl/`)
  - Compile-time shader system replacing the old material/shader class hierarchy
  - ELF-based module loader, symbol resolution, and shader registry
  - 19 built-in KSL shaders (`shaders/*.ksl.hpp`): PBR, Phong, gradient, procedural noise, audio reactive, spectrum analyzer, and others
  - KSL compiler and codegen tooling (`tools/ksl/`)
  - VSCode syntax extension (`tools/vscode-koiloscript/`)
- **SplinePath system** (`engine/koilo/core/math/`)
  - 1D and 3D spline path interpolation with Catmull-Rom support
- **SDL2 platform backend** (`engine/koilo/platform/sdl2_host.hpp`, `engine/koilo/systems/display/backends/gpu/sdl2backend.*`)
  - SDL2 host with input helper for windowed rendering
- **OpenGL render backend** (`engine/koilo/systems/render/gl/`)
  - Render backend abstraction with OpenGL and software implementations
  - Factory pattern for backend selection
- **KoiloMesh asset pipeline** (`tools/asset-conversion/`)
  - Offline OBJ/FBX -> `.kmesh` converter with UV and texture support
  - Binary mesh format with KOIM header, vertex/index/UV/material chunks
- **Example projects** (`examples/`)
  - Module demos, OBJ scene loading, 3D scene, space shooter, stress test, and syntax demo
- **Documentation** (`docs/`)
  - Architecture overview and optimization guide
  - KSL specification, reflection system, KoiloMesh format, custom modules, KoiloScript spec

### Changed
- Unified engine layout: merged `engine/include/` and `engine/src/` into `engine/koilo/`
- Camera system moved from `render/core/` to `scene/camera/`
- Render backend files reorganized into `render/gl/` subdirectory
- Tests restructured under `tests/` with scripts moved from `scripts/tests/`
- Build system updated for new directory structure and optional feature flags

### Removed
- Old material/shader class hierarchy (19 material implementations, 30 shader implementations, 6 post-processing effects) - replaced by KSL
- C API, Lua, and Python bindings (`bindings/`)
- PlatformIO support (`platformio.ini`, `src/` entry points)
- Legacy classes: `RGBColor`, `DensityField`, `PTX_SHM`, `Project`, `Controller`, `VirtualController`, `PhysicsSimulator`, `TextBuilder`, `Animator`, `BlendShapeMesh`


## [0.2.0] - 2025-10-11
Core gameplay systems (ECS, Particles, AI, World Management, Profiling)

### Added
- **Entity Component System (ECS)** (`engine/include/koilo/systems/ecs/`)
  - Entity handles with generation counters for stale handle detection
  - Dense component storage for cache-friendly iteration
  - Template-based type-safe component API
  - Component masks and query system for efficient entity filtering
  - Core components: `TransformComponent` (wraps existing Transform), `VelocityComponent`, `TagComponent`
- **Particle System** (`engine/include/koilo/systems/particles/`)
  - Particle pooling (pre-allocated, zero runtime allocations)
  - Multiple emission shapes: Point, Sphere, Box, Cone, Circle
  - Lifetime-based interpolation for size, color, and alpha
  - Physics simulation (gravity, velocity, acceleration)
  - Burst emission and custom update callbacks
- **AI System** (`engine/include/koilo/systems/ai/`)
  - **Behavior Trees**: Composites (Sequence, Selector, Parallel), Decorators (Inverter, Repeater, Succeeder), Leaves (Action, Condition, Wait)
  - **State Machines**: State callbacks (Enter/Update/Exit), automatic transition checking with conditions
  - **Pathfinding**: A* algorithm on grid, multiple heuristics (Manhattan, Euclidean, Diagonal), terrain costs, diagonal movement support
- **World Management** (`engine/include/koilo/systems/world/`)
  - Level loading/unloading with state machine (Unloaded/Loading/Loaded/Active/Unloading)
  - Level streaming based on viewer position with configurable radius
  - Level metadata and callbacks (OnLoad/OnUnload)
  - `LevelSerializer` for JSON/Binary/XML serialization formats
  - Level/Scene integration (Level can reference Scene for rendering)
- **Profiling Tools** (`engine/include/koilo/systems/profiling/`)
  - **PerformanceProfiler**: Frame timing, FPS tracking, code section profiling with RAII helpers (`KL_PROFILE_SCOPE`, `KL_PROFILE_FUNCTION`)
  - **MemoryProfiler**: Allocation tracking by tag, leak detection, peak usage tracking, formatted output (`KL_TRACK_ALLOC`, `KL_TRACK_FREE`)
  - Historical statistics (min/max/avg over configurable frame history)
  - Performance and memory reports with percentage breakdowns

### Changed
- Renamed `SceneSerializer` to `LevelSerializer` for architectural clarity (distinguishes Level serialization from Scene rendering)
- `TransformComponent` now wraps existing `Transform` class instead of duplicating fields (maintains single source of truth)
- New files include Koilo reflection support (`KL_BEGIN_FIELDS`, `KL_BEGIN_METHODS`, `KL_BEGIN_DESCRIBE`)
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
- Enhanced build scripts (`BuildSharedReflectLib.py`, `GenerateKoiloAll.py`, `UpdateKoiloRegistry.py`) to default to `engine/include/koilo`, add defensive include-path discovery, and handle relocated bindings directories.

### Fixed
- Eliminated numerous raw `new`/`delete` paths in filters, physics, and scene helpers that could leak or double-free under error conditions.

### Removed
- Legacy `scripts/migrate_layout.py` helper now that the new engine layout is the default.

## [0.1.6] - 2025-09-27
Build & tooling modernization: migration off PlatformIO for workflows, addition of signature-oriented Lua helpers, hardened Python auto-discovery, reflection parse caching, and documentation updates.

### Added
- CMake-based targets: `koilo_core` (static), `koilo_reflect` (shared), `koilo_lua` (module), `koilo_tests` (unit tests executable)
- Reflection generation custom command producing umbrella header (`koiloall.hpp`) and registry translation unit in build `generated/` directory.
- JSON reflection parse cache (size, mtime, partial hash) with flags to force reparse.
- Lua binding signature helpers (`call_sig`/constructor helpers), embedded RPATH `$ORIGIN`, and post-build copy of `libkoilo_reflect.so` beside `koilo.so`.
- Python loader upward directory search, extended candidate library names, and `KL_REFLECT_DEBUG` verbose tracing.
- New documentation: `engine/include/README.md`, `test/README.md`
- Reflection generation now skips heavy/templated or excluded headers and leverages fingerprint cache to avoid redundant clang parses

### Changed
- All public includes now referenced via `<koilo/...>` angle form; relative includes replaced to stabilize modular layout.
- Restructured repository layout (engine/include, engine/src, bindings/{c_api,lua,python}, tests, scripts, generated, external)
- Updated C API README to reflect CMake build, removed PlatformIO assumptions.
- Lua & Python READMEs refactored for new build/discovery model
- Overhauled C API, Lua, Python READMEs

### Fixed
- Lua runtime loading issues by setting module OUTPUT_NAME to `koilo` and copying reflection shared library for co-location discovery.
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
- `lib/koilo_python/koilo/reflection.py`
  - A self-contained `ctypes`-based wrapper that loads the reflection shared
    library and provides `KoiloReflection`/`KoiloClass`/`KoiloField` helpers for
    Python usage (demo in `lib/koilo_python/reflection_demo.py`).
- `lib/koilo_c_api/README.md`
  - Documentation for C ABI layer exposing registry functions
- `lib/koilo_python/README.md`
  - Documentation for using the Python ctypes wrapper and demo commands

### Changed
- `.scripts/BuildSharedReflectLib.py` (finalized)
  - Runs `GenerateKoiloAll.py`, scans `lib/koilo` headers for reflection describes,
    generates `src/reflection_entry_gen.cpp` that forces `Class::Describe()` calls,
    builds a static `libkoilo` and links it into `koilo_reflect.so`.
- Header scanner in `.scripts/BuildSharedReflectLib.py` hardened
  - Skips macro-definition lines and comment/span constructs, filters placeholder
    tokens (e.g., `CLASS`), avoids nested/unqualified type names, and attempts
    to qualify names with enclosing namespaces when detected

### Fixed
- Ensure `koilo_reflect.so` contains the full registry at runtime by generating
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
  - Backend link between C ABI -> C++ Koilo library
- reflect.h
  - Header for C ABI/API
- tasks.json
  - Task file for setting up header smoke test (unfinished)
  - Task for initializing/setting up the python workspace/venv
  - Task for updating the Koilo registry macros in all of the .hpp files
- BuildSharedReflectLib.py
  - Working on build script for C ABI
- KoiloEngine virtual environment with automatic configuration when called via task

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
- UpdateKoiloRegistry.py
  - Automatically generate and link Koilo macro functions to initialize registry

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
- Implement UpdateKoiloRegistry.py to automatically configure C++ headers for new registry macros 
  - Pay attention to ignoring operators/etc
- Implmement koilo_lua
- Implement koilo_capi/reflect_capi (cpp mapping side)
- Implement koilo_platform for library static/library linking for runtime initialization via capi on mcus and posix


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
  - Macros KL_FIELD, KL_METHOD, KL_SMETHOD as thin sugar over make_*
  - KL_BEGIN_FIELDS/METHODS & KL_END_FIELDS/METHODS for static arrays
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
- Renamed to Koilo (ProtoTracer eXtended)


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
- Renamed from um3d to koilo for more generic end functionality

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
