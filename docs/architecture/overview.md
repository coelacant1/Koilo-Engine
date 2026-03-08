# KoiloEngine Architecture

C++17 game engine targeting both embedded (Teensy 4.x, ESP32-S3) and desktop (SDL2/OpenGL). Scene and game logic lives in `.ks` script files loaded at runtime - no recompilation needed to change content.

---

## Data Flow

Script file (.ks) is lexed, parsed into an AST, and compiled to bytecode once at startup. The bytecode VM runs `fn Update(dt)` every frame, calling C++ methods through the reflection bridge. The reflection registry maps script-visible names to C++ function pointers and object instances. The VM mutates ECS components, scene objects, and module state. Modules (physics, audio, particles, effects, UI, AI) run in phase order after the script update. The render backend (software CPU rasterizer or OpenGL) produces a Color888 framebuffer that goes to the display driver (SDL2, HUB75, WS2812).

---

## Scripting Pipeline

Source goes through four stages:

1. **Lexer** - tokenizes into a flat stream of keywords, literals, operators
2. **Parser** - recursive-descent, builds a typed AST (statements, expressions, class decls, loops)
3. **Compiler** - walks AST, emits opcodes into a `CompiledScript` with a main chunk, per-function chunks, and a constant pool of numbers and interned string atoms
4. **VM** - stack-based interpreter with a 256-slot NaN-boxed value stack. NaN-boxing encodes all values (numbers, object refs, booleans, nil) as IEEE 754 doubles. `CALL_METHOD` opcodes resolve through the reflection bridge to C++ function pointers. Max call depth is 64 frames.

Compilation happens once at `BuildScene()`. The VM re-executes compiled bytecode every frame.

---

## Reflection

Any C++ class adds a `KL_BEGIN_DESCRIBE`/`KL_END_DESCRIBE` block to become script-accessible. No external codegen tools required.

Each reflected class produces:
- `Fields()` - name, type, byte offset, accessor lambdas, min/max bounds
- `Methods()` - name, arg types, return type, function pointer
- `Describe()` - bundles fields + methods + constructors, auto-registers in `GlobalRegistry`

Script call dispatch: VM looks up the object name in `GlobalRegistry`, finds the `ClassDesc`, locates the matching `MethodDesc` by name, marshals arguments, and invokes the C++ function pointer. Return values are marshalled back onto the VM stack.

---

## ECS

Dense `ComponentArray<T>` per component type (swap-with-last on removal). 64-bit `ComponentMask` per entity for O(n) bitwise-AND queries. `EntityID` is index + generation counter - stale references to recycled slots are detected at lookup.

Components that affect rendering (mesh, transform, material) mirror into the scene graph so the render backend iterates them directly.

---

## Scene Graph and Rendering

The scene graph handles spatial traversal (transforms, culling). The ECS handles logic iteration. They overlap but serve different consumers.

- `SceneNode` - transform + parent/child hierarchy
- `Mesh` - vertex/index arrays, material binding. Triangles hold pointers into the vertex array; `UpdateTransform()` modifies vertices in-place.
- `Camera` - position, rotation, projection. Default is orthographic (for LED matrix targets).

Per-frame render: project triangles, rasterize, depth test, shade, write pixels. Then post-FX, particle composite, UI overlay. Output is a Color888 framebuffer.

`IRenderBackend` decouples the pipeline. Software backend for embedded, OpenGL backend for desktop.

---

## Modules

Optional subsystems register through `ModuleLoader` with a phase that controls init/update order:

| Phase | Purpose | Examples |
|-------|---------|---------|
| 0 Core | Always present | Math, reflection, scene |
| 1 System | Independent subsystems | Physics, audio, AI |
| 2 Render | Pipeline stages | Particles, effects |
| 3 Overlay | Drawn last | UI |

Each module's `Initialize()` receives the engine pointer and calls `RegisterGlobal()` to expose objects to scripts. Modules can be statically linked or dynamically loaded (.so/.dll).

---

## Entry Point

```cpp
koilo::platform::DesktopFileReader reader;
koilo::scripting::KoiloScriptEngine engine(&reader);

engine.LoadScript("scene.ks");
engine.BuildScene();

while (running) {
    engine.ExecuteUpdate(dt);
    engine.RenderFrame(fb, w, h);
}
```

`LoadScript` and `BuildScene` are separated so the host can validate the script before allocating hardware resources. On embedded, swap `DesktopFileReader` for a flash-backed reader - the loop is identical.

---

## Constraints

| Rule | Why |
|------|-----|
| No heap allocation after `BuildScene()` on embedded | MCUs without MMU fragment and crash |
| No STL exceptions in engine core | Stack unwinding disabled on most MCUs |
| Reflection macros must reference real members | Wrong names silently break script dispatch |
| `Triangle3D` stores vertex pointers, not copies | In-place `UpdateTransform()` requires shared data |
| `SetRotation()` takes degrees | API consistency; internal conversion in `EulerAnglesToQuaternion()` |
| Module init order is by phase, not registration | Overlay depends on Render depends on System |
