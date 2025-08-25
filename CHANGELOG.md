# Change Log
All notable changes to this project will be documented in this file.
 
The format is based on [Keep a Changelog](http://keepachangelog.com/)
and this project adheres to [Semantic Versioning](http://semver.org/).


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
- render core colorspace
- Assets density field?
- make ray trace function

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
- render core colorspace
- render material shadermaterial
- mesh align -> use triangle 3d for rasterizing?
- material shaders
- make ray trace function to replace rasterize
- split effect from scene and add to compositor
  - render post


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
