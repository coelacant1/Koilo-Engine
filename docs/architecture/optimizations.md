# Engine Optimizations

Performance techniques used across the KoiloEngine backend. The engine targets both embedded MCUs (Teensy 4.x, ESP32) and desktop (SDL2/OpenGL), so most optimizations focus on minimizing allocation, cache misses, and redundant computation.

---

## Software Rasterizer

The CPU rasterizer is the primary render path on embedded. All inner-loop work is designed around cache locality and branch elimination.

### Edge-Walking Scanline

Triangles are rasterized using incremental barycentric half-plane equations. Per-row x_start/x_end bounds are computed analytically from edge slopes, avoiding per-pixel half-plane tests. Barycentric deltas (dv_dx, dw_dx, dv_dy, dw_dy) are precomputed once per triangle and accumulated across scanlines.

### Flat POD Triangle Layout

`RasterTriangle2D` is a flat POD struct with all data inline - screen verts, world verts, UVs, normals, depth values, AABB bounds, and shader pointer. No inheritance, no vtable, no pointer chasing. A single triangle fits in a few cache lines.

### Integer Pixel AABB

During projection, each triangle's screen-space bounding box is computed once and stored as `int16_t` (pixMinX/Y, pixMaxX/Y). The scanline loop uses these directly, avoiding repeated float-to-int conversions.

### Conditional Attribute Interpolation

Each triangle carries an `attribMask` (uint8_t) indicating which shader inputs are actually needed (position, normal, UV, view direction). The inner loop only computes and increments attributes the shader reads. A flat-color shader skips all interpolation except depth.

### Perspective Correction

Attributes are pre-divided by Z once per triangle vertex. The inner loop accumulates 1/w incrementally and performs a single `1.0f / iw` division per pixel only when needed. This amortizes the expensive division across the triangle.

### Material Sort

Before rasterization, triangles are sorted by shader function pointer (primary) then by depth front-to-back (secondary). Grouping by shader pointer maximizes instruction cache hits. Front-to-back ordering enables early depth rejection.

### Sky Fill

Background gradient is filled using seed-and-double: write one pixel's RGB, then `memcpy` to double the filled region repeatedly. Fills a full row in ~log2(width) copies instead of per-pixel writes.

### Depth Buffer

Static depth buffer reused across frames (no per-frame allocation). Early Z-reject skips shading for occluded pixels. Quake III fast inverse square root (`0x5f3759df` magic constant) used for glow distance falloff calculations.

---

## Bytecode VM

### NaN-Boxing

All script values are 8 bytes. Numbers are raw IEEE 754 doubles. Non-number types (string, array, table, object, bool, nil, function, instance) encode a 3-bit type tag in the quiet-NaN bit pattern with a 48-bit payload. This reduces per-value memory from ~125 bytes (tagged union with std::string) to 8 bytes, making the 256-slot value stack fit in 2 KB.

### Method Dispatch Cache

A static `unordered_map<MethodCacheKey, const MethodDesc*>` caches resolved method lookups. The key combines class descriptor pointer, method name, and argument count. After the first call, subsequent dispatches to the same method on the same class type hit the cache and skip overload resolution entirely.

### ArgMarshaller

Method arguments are marshalled through a reusable static `ArgMarshaller` instance. It holds 16 pre-allocated slots per type (float, int, double, bool, string, pointer). For calls with 8 or fewer arguments, all marshalling happens on stack-allocated buffers with zero heap allocation. The instance persists across calls - only the slot counters reset via `Clear()`.

### String Atom Table

All identifier strings (variable names, method names, field names) are interned in an `AtomTable` at compile time. The table maps strings to `uint32_t` atom IDs. Name comparisons in the VM become integer equality checks instead of string hashing and comparison.

### Fixed Stack Layout

The VM uses statically-sized arrays for both the value stack (256 entries) and call frames (64 max depth). No dynamic allocation during execution. Local variable access is O(1) via `stackBase + slot`. Coroutine suspend/resume copies the entire stack/frame arrays with `memcpy`.

### Frame-Scoped Object Lifecycle

Temporary C++ objects created during a frame (method return values, constructor results) are tracked in a per-frame list and destroyed at end of `ExecuteUpdate()`. Objects assigned to script variables or fields are promoted to persistent and survive frame cleanup. When a persistent slot is overwritten, the old object is destroyed immediately to prevent leaks.

---

## GPU Backend (OpenGL)

### Uber-Shader

All KSL shaders compile into a single OpenGL program. A `u_shaderID` uniform selects the active shader variant at draw time. This avoids expensive `glUseProgram` switches between materials. Shader IDs are mapped from names at load time via `uber_ids.txt`.

### Dirty Mesh Upload

Each mesh carries a GPU version counter. The render backend caches the last-uploaded version per VBO. If the version matches, the upload is skipped entirely. Animated meshes use `glBufferSubData` for partial updates; static meshes use `GL_STATIC_DRAW`.

### Batch Rendering

Meshes are sorted by (program, material) to minimize GL state changes. Multiple meshes sharing the same material are merged into a single batch VBO for consolidated draw calls. The batch vertex buffer is a static vector reused across frames.

### Uniform Location Cache

KSL material uniform locations are cached per-program in `kslUniformCache_`. This avoids repeated `glGetUniformLocation` calls on every draw.

### Texture Sub-Upload

After the first frame, framebuffer texture updates use `glTexSubImage2D` instead of `glTexImage2D`. Full reallocation only happens on resize.

### Double-Buffered PBO

Pixel buffer objects are double-buffered for async CPU-to-GPU texture uploads, overlapping transfer with rendering.

---

## Platform

### Thread Affinity

`PinToPerformanceCore()` detects big.LITTLE topology on Linux (via `cpu_capacity` sysfs), identifies performance cores, and pins the render thread. On Windows, uses `SetThreadAffinityMask`. Reduces jitter from core migration and improves cache locality.

### Discrete GPU Hints

NvOptimus (`NvOptimusEnablement`) and AMD PowerXpress (`AmdPowerXpressRequestHighPerformance`) exports are set at link time to request the discrete GPU on laptops. On Linux, PRIME GPU selection is configured via a constructor-priority-101 initializer.

---

## Scene Graph

### Dirty-Flag Transforms

`SceneNode` tracks a `worldDirty_` flag. `GetWorldTransform()` recomputes the concatenated parent chain only when the flag is set. `MarkDirty()` propagates down the hierarchy. Untouched nodes skip all matrix math.

### Interleaved Vertex Format

GPU meshes use a packed interleaved layout: 3 floats position + 3 floats normal + 2 floats UV = 32 bytes per vertex. This matches GPU cache line expectations and avoids multiple buffer binds per draw.
