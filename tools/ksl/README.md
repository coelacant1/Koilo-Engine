# KSL Tools

KoiloEngine Shader Language toolchain.

## Tools

| Script | Purpose |
|--------|---------|
| `ksl_codegen.py` | Transpile `.ksl.hpp` -> `.glsl` (GPU rendering) |
| `ksl_compiler.py` | Compile `.ksl.hpp` -> `.kso` ELF (CPU rasterizer) |

Both are called automatically by CMake during the build (`engine/src/systems/display/CMakeLists.txt`).

## Manual Usage

```bash
# Generate GLSL for GPU
python3 tools/ksl/ksl_codegen.py engine/shaders/phong.ksl.hpp -o build/shaders/phong.glsl

# Compile ELF for CPU rasterizer
python3 tools/ksl/ksl_compiler.py engine/shaders/phong.ksl.hpp -o build/shaders/phong.kso
```
