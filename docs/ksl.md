# KSL Shader System

KSL (Koilo Shader Language) is a unified shader system. Each shader is a single `.ksl.hpp` C++ header that defines both GPU and CPU rendering paths. A compile step produces a `.kso` (ELF binary for CPU) and a `.glsl` (fragment shader for GPU) from the same source.

---

## Pipeline

```
.ksl.hpp source
    |
    +- ksl_codegen.py  -> .glsl (GLSL fragment shader)
    +- ksl_compiler.py -> .kso  (ELF shared object)
```

`ksl_codegen.py` extracts `KSL_PARAM` macros into uniform declarations and translates the `shade()` body from C++ to GLSL via regex substitution (`in.position` to `v_position`, `ksl::vec3` to `vec3`, `ksl::sample()` to `texture()`, etc.). It also emits a stdlib of helper functions (noise, hash, map, rotate2d).

`ksl_compiler.py` generates a C ABI wrapper around the shader struct (`ksl_create`, `ksl_destroy`, `ksl_set_param`, `ksl_shade`, `ksl_info`) and compiles it with `g++` or `clang++` as a position-independent shared object. For MCU targets, uses `arm-none-eabi-g++`.

Both tools are called by CMake during the build. Outputs go to `build/shaders/`.

---

## Writing a Shader

Create a `.ksl.hpp` file in `shaders/`:

```cpp
#pragma once
#include "koilo/ksl/ksl.hpp"

struct KSL_MyShader : public ksl::Shader {
    float intensity = 1.0f;
    ksl::vec3 color;

    KSL_PARAMS_BEGIN(KSL_MyShader)
        KSL_PARAM(float,     intensity, "Effect intensity", 0.0f, 10.0f)
        KSL_PARAM(ksl::vec3, color,     "Base color",       0.0f, 1.0f)
    KSL_PARAMS_END

    ksl::vec4 shade(const ksl::ShadeInput& in) const override {
        return ksl::vec4(color * intensity, 1.0f);
    }
};
```

For array parameters use `KSL_PARAM_ARRAY(type, name, maxSize, description)`.

### ShadeInput

| Field | Type | Description |
|-------|------|-------------|
| `position` | `vec3` | World-space position |
| `normal` | `vec3` | Surface normal |
| `uv` | `vec2` | Texture coordinates |
| `viewDir` | `vec3` | View direction |
| `depth` | `float` | Depth value |
| `time` | `float` | Elapsed time (seconds) |
| `dt` | `float` | Delta time |
| `frameCount` | `int` | Current frame number |
| `lights` | `LightData*` | Active lights array |
| `lightCount` | `int` | Number of active lights |

---

## Runtime Loading

`KSLRegistry::ScanDirectory()` scans for matching `.kso` + `.glsl` pairs. The ELF loader (`ksl_elf_loader.hpp`) maps the `.kso` into executable memory, processes relocations, and resolves symbols against the engine's symbol table. Five C-linkage functions are extracted per shader: `ksl_info`, `ksl_create`, `ksl_destroy`, `ksl_set_param`, `ksl_shade`.

Each loaded shader becomes a `KSLModule` that provides both a CPU path (calling the ELF-loaded `shade()`) and a GPU path (the compiled GL program from the `.glsl`).

If an uber-shader (`uber.glsl`) is present, all shaders share a single GL program and select variants via a `u_shaderID` uniform.

---

## KSLMaterial

`KSLMaterial` is the runtime material class. It wraps a `KSLModule` behind the `IMaterial` interface and is reflected for KoiloScript access.

### C++

```cpp
KSLMaterial mat("pbr");
mat.SetVec3("albedo", 0.8f, 0.2f, 0.1f);
mat.SetFloat("roughness", 0.5f);
mat.AddLight();
mat.SetLightPosition(0, 0.0f, 10.0f, 0.0f);
mat.SetLightColor(0, 1.0f, 1.0f, 1.0f);
mat.SetLightIntensity(0, 1.0f);
mesh.SetMaterial(&mat);
```

### KoiloScript

```
var mat = KSLMaterial("phong");
mat.SetFloat("shininess", 32.0);
mat.SetVec3("diffuseColor", 0.6, 0.3, 0.1);
mat.AddLight();
mat.SetLightPosition(0, 0.0, 10.0, 5.0);
mesh.SetMaterial(mat);
```

### Parameter Methods

| Method | Description |
|--------|-------------|
| `SetFloat(name, value)` | Float parameter |
| `SetInt(name, value)` | Integer parameter |
| `SetVec2(name, x, y)` | 2D vector |
| `SetVec3(name, x, y, z)` | 3D vector |
| `SetColor(name, Color888)` | Color (auto-normalized to 0-1) |
| `SetVec3Array(name, index, x, y, z)` | Element of a vec3 array |
| `SetColorAt(name, index, Color888)` | Array element from Color888 |
| `SetRainbowPalette(name, count)` | Fill array with up to 6 rainbow colors |

Materials support deferred parameter setting - parameters set before the module binds are buffered and replayed once the shader resolves.

Up to 16 lights per material with per-light position, color, intensity, falloff, and curve.

---

## Render Backends

**GPU (OpenGL)** - `OpenGLRenderBackend` compiles the `.glsl` into GL programs via `KSLModule::LoadGLSL()`. At draw time, `BindKSLMaterialUniforms()` pushes each parameter as a GL uniform. Light data is uploaded as uniform arrays.

**CPU (Software Rasterizer)** - `KSLShaderBridge` implements `IShader`. It populates a `ksl::ShadeInput` from the rasterizer's `SurfaceProperties` and the material's light/camera state, then calls `KSLModule::Shade()` on the ELF-loaded instance.

**MCU (Embedded)** - Same `.kso` format compiled with `arm-none-eabi-g++`. Identical shader code runs on microcontrollers without a GPU.

---

## Included Shaders

| Shader | Description |
|--------|-------------|
| `audio_reactive` | Audio sample data as rings/bars with gradient coloring |
| `depth` | Maps world position along an axis to a color gradient |
| `gradient` | Linear or radial gradient with rotation and phase |
| `horizontal_rainbow` | Scrolling rainbow spectrum pattern |
| `image` | Texture sampling with optional hue shift |
| `image_sequence` | Texture sequence with hue shift |
| `normal` | Surface normals as RGB |
| `oscilloscope` | Audio waveform line visualization |
| `pbr` | GGX distribution, Fresnel-Schlick, Smith geometry |
| `phong` | Ambient, diffuse, specular lighting |
| `procedural_noise` | Simplex noise mapped to color spectrum |
| `sky` | Vertical gradient between two colors |
| `spectrum_analyzer` | Frequency spectrum as vertical bars |
| `spiral` | Procedural spiral with configurable palette |
| `texture` | Texture with frame regions and diffuse lighting |
| `tv_static` | Simplex noise, scanlines, and colored bars |
| `uniform_color` | Single flat color (default/fallback) |
| `uvmap` | Texture with U/V axis flip and hue shift |
| `vector_field_2d` | 2D vector field as density gradient |
