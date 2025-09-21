# Change Log
All notable changes to this project will be documented in this file.
 
The format is based on [Keep a Changelog](http://keepachangelog.com/)
and this project adheres to [Semantic Versioning](http://semver.org/).

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
