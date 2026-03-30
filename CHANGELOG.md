# Change Log
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](http://keepachangelog.com/)
and this project adheres to [Semantic Versioning](http://semver.org/).

## [0.3.13] - 2026-3-30
Typed services, input events, ECS runtime components, and custom UI widgets.

### Added
- Typed service registry: `RegisterTyped<T>()`, `Get<T>()` with debug type-safety assertions, `ListByPrefix()` for service discovery (#27)
- `ICommandProvider` interface - modules declare console commands without coupling to ConsoleModule; discovered via `"commands.*"` prefix scan (#26)
- Input event system: `KeyEvent`, `MouseButtonEvent`, `MouseMoveEvent`, `ScrollEvent` structs with `IInputListener` priority-based dispatch and consumption (#28)
- `InputListenerRegistry` registered as `"input.listeners"` kernel service
- ECS component registry: `IComponentType` / `ComponentType<T>` descriptors, `ComponentRegistry` with runtime ID assignment after compile-time IDs (#29)
- Widget factory: `IWidgetFactory` / `CustomWidget` / `WidgetTypeRegistry` for module-defined UI widgets rendered via `CanvasDrawContext` (#30)
- `WidgetTag::Custom` enum value with `customWidget` field on Widget struct

### Changed
- `ComponentMask` expanded from `bitset<64>` to `bitset<128>`
- Audio, AI, Physics, and Asset modules migrated from direct `ConsoleModule::Instance()` to `ICommandProvider` pattern
- `InputManager::Update()` now generates events by diffing current vs previous frame state (polling API unchanged)

## [0.3.12] - 2026-3-30
Software RHI rendering fixes: text, scissor clipping, and draw ordering.

### Fixed
- SW font atlas text rendering (atlas texture data was valid but scissor clipped it away)
- Scissor Y-flip for SW backend - uses top-left origin like Vulkan, not bottom-left like OpenGL
- Degenerate scissor fallback drew everything instead of nothing, breaking scroll clipping
- VBO buffer overflow silently dropping late draw calls (floating panels not covering background)

### Changed
- `IsVulkanDevice()` - `UsesTopLeftOrigin()` on RenderPipeline (returns true for Vulkan and Software)
- UIRHIRenderer splits `isVulkan_` into `topLeftOrigin_` (scissor) and `deferredDraw_` (VBO batching)

## [0.3.11] - 2026-3-29
KSL decoupled from OpenGL, render graph infrastructure, and unified frame loop.

### Added
- Render graph (`engine/koilo/systems/render/graph/`) with pass DAG, stable topological sort, and resource lifetime tracking (issue #20)
- Standard pass library: `scene_pass`, `sky_pass`, `debug_pass`, `blit_pass`, `overlay_pass`
- Granular `RenderPipeline` methods: `BeginScenePass()`, `RenderSky()`, `RenderSceneMeshes()`, `RenderSceneDebugLines()`, `EndScenePass()`
- 11 render graph unit tests (DAG ordering, cycle detection, lifetimes, execution)

### Changed
- KSL shader registry fully decoupled from OpenGL -- zero GL references remain in any KSL file (issue #18)
  - `LoadGLSL()` retains source strings only; all GL compilation, uber-shader, and uniform APIs removed
  - `ScanDirectory()` uses unified path (removed `#ifdef KL_HAVE_OPENGL_BACKEND` guards)
- SDL3Host frame loop unified: simulation tick and UI animation shared above GPU/SW branch
  - GPU path now driven by render graph (7-8 passes per frame)
  - `RenderDirect()` refactored to delegate to granular methods (backward compatible)

### Removed
- Non-RHI GPU fallback path in SDL3Host (dead code since factory always returns `RenderPipeline`)
- All OpenGL API calls and preprocessor guards from KSL public headers

## [0.3.10] - 2026-3-29
Unified UI renderer interface, legacy renderer removal, SW/GPU consistency fixes, and RHI smoke tests.

### Added
- `IUIRenderer` abstract interface (`ui_renderer.hpp`) unifying GPU and software UI renderers behind a single polymorphic API (issue #21)
  - `SetFont()` / `SetBoldFont()` / `SyncFontAtlases()` for backend-agnostic font atlas management
  - `Render()` / `Pixels()` / `IsSoftware()` / `IsInitialized()` / `Shutdown()` virtual methods
- `PrepareAndRender()` unified render path in `ui.cpp`, replacing separate GPU and SW render logic
- RHI pipeline and renderer smoke tests
  - Factory returns software backend, UIRHIRenderer default/shutdown safety, UISWRenderer full lifecycle through IUIRenderer interface, pixel output verification, polymorphism checks

### Changed
- `UI` class now holds a single `unique_ptr<IUIRenderer>` instead of separate `UIRHIRenderer` + `UISWRenderer` members
- `UIRHIRenderer` and `UISWRenderer` both inherit from `IUIRenderer`
- Font atlas texture handles (`fontHandle_`, `boldHandle_`) managed by `UI` class, not individual renderers
- `RenderBackendFactory` reduced from 5 paths to 3 (Vulkan RHI, OpenGL RHI, Software) - all legacy non-RHI paths removed
- Removed `cvar_r_legacy_backend` CVar (no longer needed since RHI is the only GPU path)
- Reflection registry regenerated

### Fixed
- SW renderer scissor rectangles not intersecting with parent (GPU already did this), causing child widgets to draw outside parent bounds
- SW renderer coordinate truncation instead of rounding, causing 1px misalignment vs GPU
- SW renderer border widths collapsing to 0px at sub-pixel sizes (now `max(1, round(...))`)
- SW renderer glyph scaling using integer division, producing blocky text at non-native sizes
- SW renderer rounded rects, filled circles, and circle outlines using linear alpha falloff instead of smoothstep, causing harsh edges compared to GPU
- SW renderer circle outlines having no anti-aliasing at all (now has smoothstep on inner and outer edges)

### Removed
- Legacy (pre-RHI) UI renderers: `ui_gl_renderer.{cpp,hpp}`, `ui_vk_renderer.{cpp,hpp}` (deleted)
- Legacy (pre-RHI) render backends moved to `docs/.wip/legacy_render_backends/`: `opengl_render_backend.{cpp,hpp}`, `vulkan_render_backend.{cpp,hpp}`, `testopenglrenderbackend.{cpp,hpp}`
- `TryCreateGPURenderBackend()` factory function and all legacy CVar opt-in/fallback code

## [0.3.9] - 2026-3-27
OpenGL RHI rendering fixes -- depth buffer, material uniform bridging, and vertex attribute state.

### Fixed
- OpenGL RHI depth buffer never clearing between frames
  - Blit and overlay pipelines set `glDepthMask(GL_FALSE)`, which persisted into the next frame's `BeginRenderPass`. OpenGL's `glClear(GL_DEPTH_BUFFER_BIT)` respects the depth mask, so the clear was silently a no-op.
  - Symptoms: objects invisible when camera was still, partially visible only during motion, progressively disappearing over time
  - Fix: force `glDepthMask(GL_TRUE)` and `glColorMask(GL_TRUE,...)` before `glClear` in `BeginRenderPass`
- OpenGL RHI material uniforms receiving wrong values (black textures, wrong material colors)
  - `BridgeMaterialUniforms` enumerated uniforms via `glGetActiveUniform` and assumed they matched the std140 buffer's declaration-order packing. Mesa returns uniforms in alphabetical order, causing values like `u_frameW` to receive `u_hueAngle`'s data.
  - Fix: pipeline now carries KSL parameter metadata (name + type in declaration order). Bridge looks up each uniform by name (`"u_" + paramName`) and reads from the correct std140 offset. Fallback sort-by-location path retained for pipelines without metadata.
- OpenGL RHI vertex attribute state leaking between pipeline switches
  - `SetupVertexAttributes` enabled locations for the current pipeline but never disabled locations from the previous one. Stale attributes from a 3-attribute scene pipeline could bleed into a 2-attribute blit pipeline.
  - Fix: disable all attribute slots (0-7) before enabling the current pipeline's attributes

### Changed
- `RHIPipelineDesc` gains optional `materialParams[]` array for passing KSL parameter names and types to the RHI backend
- `GetOrCreateScenePipeline` populates material param metadata from the KSL registry when available
- OpenGL `PipelineSlot` stores material parameter names for name-based uniform bridging

## [0.3.8] - 2026-3-21
Vulkan shutdown crash fix, software display path fix, console improvements, and OpenGL performance.

### Added
- Interactive line editor for the console REPL (history, cursor movement, word/line delete, tab completion, screen clear)
- `--vulkan` and `--opengl` flags to prefer a specific render backend
- GPU selection hints in `--help` for hybrid laptops (DRI_PRIME, NVIDIA offload)

### Changed
- RenderPipeline sorts draw calls by pipeline and material to minimize GPU state changes (issue #7)
- Hoisted per-frame transform UBO upload out of per-mesh loop (was re-uploading identical data per draw)
- All `glGetUniformLocation` calls in OpenGL backend now use cached lookups
- Removed dead `SetUniform3c`/`SetUniform3v` helper functions

### Fixed
- Vulkan shutdown segfault caused by ODR violation in KSLRegistry/KSLModule (issue #4)
  - `KSLRegistry` and `KSLModule` had member variables behind `#ifdef KL_HAVE_OPENGL_BACKEND`, but `koiloengine_display` was compiled WITH the define while `koilo_engine` was compiled WITHOUT it, giving each translation unit a different class layout
  - Fix: member variables are now always present (plain int types); only GL API calls remain behind `#ifdef`
- Use-after-free in KSLMaterial::Unbind() during shutdown
  - KSLMaterial held raw pointer to KSLModule destroyed with the registry; late-running script engine cleanup called Unbind() on freed module
  - Fix: clear static registry pointer before destroying it; guard Unbind() with registry-alive check
- Null out Vulkan handles in slot after Destroy*() calls (defense-in-depth)
- Software display path pixel format mismatch (issue #5)
  - SDL3Backend mapped RGB888 (3 bpp) to SDL_PIXELFORMAT_XRGB8888 (4 bpp), causing stride mismatch and corrupted/interlaced output
  - Fix: use SDL_PIXELFORMAT_RGB24/BGR24 to match framebuffer byte width

## [0.3.7] - 2026-3-20
Unified render pipeline, shared caches, Vulkan validation fixes, and clean build fixes.

### Added
- Unified `RenderPipeline` using `IRHIDevice`, replacing duplicated backend scene traversal
- Shared `MeshCache`, `TextureCache`, and `MaterialBinder` for backend-agnostic resource management
- `ShaderResolver` for built-in shader name mapping (blit, overlay, debug line, pink error, sky)
- Deferred descriptor set write pattern for scene set 0 with automatic re-allocation on post-flush writes
- Per-frame deferred buffer and texture deletion queued behind GPU fence waits
- Per-frame descriptor pools (one per frame in flight)
- `sampleAfterPass` flag on `RHIColorAttachment` for render pass final layout control
- `r_backend` CVar for runtime render backend selection
- Swapchain render pass API on `IRHIDevice`
- `build.sh` flags: `--no-vulkan`, `--no-opengl`, `--software-only`, `--skip-ns`, `--debug`
- SDL3Backend fallback display for pure software-only builds (no GL/VK dependency)

### Changed
- Vulkan descriptor set layouts restructured into scene (3 sets) and blit (2 sets) pipelines
- `BindUniformBuffer` fully deferred for scene set 0, no immediate Vulkan calls
- `BindPipeline` uses effective-layout-aware cache invalidation
- `BeginFrame` reordered so fence wait precedes pool reset and deferred deletion flush
- `UploadLights` now runs before `RenderSky` so all set 0 bindings populate before first draw
- KSL modules expose SPIR-V accessors; registry gains `GetModuleByName`
- SDL3Host frame loop updated for RenderFrameGPU -> BlitToScreen -> Overlay -> UI -> Present sequence
- Removed `updatekoiloregistry.py` from CMake build (reflection registry is manual-only now)
- Reflection entry generator classifies GPU backend paths as platform-specific (software-only linker fix)

### Fixed
- Descriptor set 0 incomplete bindings from per-call allocation replaced with deferred accumulation
- Descriptor set updated after bind (UPDATE_AFTER_BIND violation) fixed with fresh-set replay
- Buffer and texture destruction while GPU in-flight via per-frame deferred deletion queues
- Render pipeline binding order causing partial set 0 flushes before lights were uploaded
- Descriptor type mismatch from stale pipeline layout carrying across frames
- Descriptor pool reset while other frame's command buffer still in-flight
- Offscreen-to-blit image layout transition missing proper subpass dependency
- Sky and pink error shader vertex format mismatches
- Overlay rendering outside active render pass
- Texture upload missing TransferDst usage flag
- Broken KL_* reflection macros in 18+ files that prevented clean builds
- Clean build from scratch now works for all backend configurations

## [0.3.6] - 2026-3-18
Render Hardware Interface (RHI) abstraction layer with Vulkan and OpenGL drivers.

### Added
- **RHI type system** (`engine/koilo/systems/render/rhi/rhi_types.hpp`)
 - 7 opaque handle types with null sentinels: Buffer, Texture, Shader, Pipeline, RenderPass, Framebuffer, DescriptorSet
 - 14 pixel/depth formats (R8 through D32F_S8), buffer/texture usage flags, shader stages
 - Complete pipeline state: topology, rasterizer, depth/stencil, blend, vertex attributes
 - Descriptor structs: `RHIBufferDesc`, `RHITextureDesc`, `RHIPipelineDesc`, `RHIRenderPassDesc`
 - Utility helpers: `RHIFormatBytesPerPixel`, `RHIFormatIsDepth`, `HasFlag` operators
- **RHI device interface** (`engine/koilo/systems/render/rhi/rhi_device.hpp`)
 - `IRHIDevice` pure virtual interface (~35 methods) covering lifecycle, capabilities, resource CRUD, data transfer, command recording, and presentation
 - Backend-agnostic contract enabling single render pipeline for all GPU APIs
- **RHI capability system** (`engine/koilo/systems/render/rhi/rhi_caps.hpp`)
 - `RHIFeature` enum (10 features): timestamp queries, compute, multi-draw indirect, bindless textures, push constants, storage buffers, geometry/tessellation shaders, async compute, depth clamp
 - `RHILimits` struct (14 hardware limits) queried from device at initialization
- **Vulkan RHI driver** (`engine/koilo/systems/render/rhi/vulkan/`)
 - Full `IRHIDevice` implementation using Vulkan API with SlotArray handle-VkObject mapping
 - Resource lifecycle: buffer/texture/shader/pipeline/renderpass/framebuffer create/destroy
 - Command recording: render pass begin/end, pipeline/buffer/texture binding, draw/draw-indexed
 - Descriptor pool with per-frame reset to prevent set exhaustion
 - Push constants, dynamic viewport/scissor, staging buffer transfers with layout transitions
 - Device limit/feature queries from `VkPhysicalDeviceProperties` and `VkPhysicalDeviceFeatures`
- **OpenGL RHI driver** (`engine/koilo/systems/render/rhi/opengl/`)
 - Full `IRHIDevice` implementation using OpenGL 3.3+ with immediate-mode GL calls
 - SlotArray pattern matching Vulkan driver for consistent handle management
 - Format conversion tables (RHIFormat <-> GL internal/pixel/type), topology/blend/compare mapping
 - Push constants emulated via reserved UBO at binding 7
 - Persistent VAO with lazy reconfiguration on vertex state change
 - Proper ordered shutdown: framebuffers - pipelines - shaders - textures - buffers
- **RHI smoke tests** (`tests/engine/systems/render/rhi/`)
 - Handle null sentinel and equality, format helpers, usage flag composition
 - Descriptor struct defaults, capability feature flags, limits defaults

### Changed
- **CMake** - RHI driver sources excluded from main glob and conditionally compiled via display CMakeLists (Vulkan section / OpenGL section), matching existing backend pattern
- **Vulkan RHI** - descriptor set layouts now checked for creation errors with proper error logging
- **Vulkan RHI** - `BindUniformBuffer` and `BindTexture` log errors on descriptor allocation failure

## [0.3.5] - 2026-3-18
Runtime inspection, console intelligence, state snapshots, and TCP event streaming.

### Added
- **Entity console commands** (`engine/koilo/kernel/console/entity_commands.cpp`)
 - `entity.list [tag]` - enumerate all live entities with optional tag filter
 - `entity.inspect <id|tag>` - show transform, velocity, scene node, and component details
 - `entity.set <id>.<field> <value>` - set entity position, scale, velocity, or tag at runtime
 - `entity.watch <id>.<field>` / `entity.unwatch` - pin entity fields to the debug overlay
 - `scene.hierarchy` - walk and print the scene graph tree with node names, meshes, and positions
 - `help --json` - structured JSON output of the full command registry for tooling
- **State save/load system** (`state.save`, `state.load`, `state.list`)
 - Saves entity transforms + velocities, CVars, and camera position to named snapshots
 - Tag-based entity matching on restore for cross-session state transfer
 - Snapshots stored as `states/<name>/` with `entities.json`, `cvars.cfg`, `camera.json`
- **TCP event subscription** (`engine/koilo/kernel/console/event_bridge.hpp`)
 - `EventBridge` service bridges MessageBus events to subscribed TCP console clients
 - `subscribe <event,...>` / `unsubscribe [event,...]` / `subscriptions` commands
 - Per-client subscription tracking with thread-local token per connection
 - Supports all 14 built-in event types (module, asset, scene, entity lifecycle)
 - JSON-line push format for real-time event streaming to external tools
- **`ScriptEntityManager::GetAllEntities()`** - public method to iterate all valid entities

### Changed
- **ConsoleModule** - now owns and registers `EventBridge` as kernel service `"events"`
- **ConsoleSocket** - per-client `EventBridge` token lifecycle (register on connect, unregister on disconnect)

## [0.3.4] - 2026-3-18
Service registration, CVar system, structured logging, module modularization, and critical bug fixes.

### Added
- **CVar system** (`engine/koilo/kernel/cvar/`)
 - `CVar<T>` template with int, float, bool, and string types
 - Automatic registration, console integration, and change callbacks
 - `sim_cvars` - simulation pause/step CVars for frame debugging
 - `render_cvars` - wireframe, culling, depth test, and max FPS CVars
 - Console commands: `cvar.list`, `cvar.get`, `cvar.set`, `cvar.reset`
- **Config system** (`engine/koilo/kernel/config/`)
 - `ConfigStore` - hierarchical key-value configuration with INI-style persistence
 - `IConfigProvider` interface for pluggable config backends
 - Console commands: `config.get`, `config.set`, `config.list`, `config.save`, `config.load`
- **Structured logging** (`engine/koilo/kernel/logging/`)
 - `KL_LOG`, `KL_WARN`, `KL_ERR` macros with channel-tagged output
 - `LogSystem` with per-channel verbosity, ring buffer history, and output sinks
 - Console commands: `log.level`, `log.history`, `log.channels`, `log.filter`
- **Thread pool** (`engine/koilo/kernel/thread_pool.*`)
 - Work-stealing thread pool with priority queues (Low, Normal, High)
 - Registered as kernel service; used by console socket and asset pipeline
- **Debug overlay** (`engine/koilo/kernel/debug_overlay.*`)
 - FPS counter, frame time graph, and memory stats overlay
 - Toggleable via CVar or console command
- **Module implementations** (IModule-based kernel modules)
 - `SceneModule` - scene and camera lifecycle as a kernel module
 - `InputModule` - input state management as a kernel module
 - `RenderModule` - render backend orchestration as a kernel module
 - `UIModule` - UI system lifecycle as a kernel module
 - `ParticleModule` - particle system as a kernel module
- **Console commands**
 - `utility_commands.cpp` - uptime, version, clear, echo, help improvements
 - `log_commands.cpp` - structured log querying and channel management
 - `message_commands.cpp` - message bus tap/untap/send/dispatch/flush
 - `service_commands.cpp` - service listing with type info and health checks

### Changed
- **Service registration** (`register_services.cpp`, `service_registry.*`)
 - Services now registered with string names and typed accessors
 - Thread pool, config store, CVar system, and log system registered as kernel services
- **Module manager** (`module_manager.*`)
 - Phase-ordered initialization (Core -> Simulation -> Render -> Overlay)
 - Capability checking integrated into module lifecycle
- **Kernel** (`kernel.cpp/hpp`)
 - Expanded with CVar system, config store, and log system ownership
 - `Shutdown()` dispatches `MSG_SHUTDOWN` signal before module teardown
- **Asset job queue** (`asset_job_queue.*`)
 - Migrated from internal thread to kernel thread pool
 - Priority-based job scheduling
- **Vulkan render backend** (`vulkan_render_backend.*`)
 - CVar-driven wireframe, culling, and depth test toggling
 - Pipeline cache rebuild on CVar change
- **Script engine module initialization** (`koiloscript_engine.cpp`)
 - `moduleLoader_.InitializeAll()` now called before `BuildCamera()` so scene/camera modules are available during setup
- **Render loop idle-skip** (`sdl3_host.hpp`)
 - Idle-skip optimization now requires at least 2 rendered frames before activating, ensuring the first frame is presented and the window becomes visible

### Fixed
- **Vulkan validation errors on shutdown** - Early break in render loop on quit signal prevents submitting command buffers with stale image layouts; `vkDeviceWaitIdle()` replaces final `SwapOnly()` during teardown
- **Shutdown hang requiring double Ctrl+C** - `ConsoleSocket::Stop()` now calls `shutdown(fd, SHUT_RDWR)` on server and client sockets before `close()`, reliably unblocking `accept()` and `recv()` on Linux
- **Scene never created / window not rendering** - Module initialization order fixed: `InitializeAll()` moved before `BuildCamera()` so `GetModule("scene")` finds the initialized `SceneModule`
- **Bytecode compiler `if`-statement stack leak** - `CompileIfStatement` now emits `POP` for the condition value on both true and false branches, matching the `while` loop pattern; prevents stack overflow in scripts with many `if` statements inside loops
- **Console session** - Command parsing edge cases and output formatting fixes

## [0.3.3] - 2026-3-17
Vulkan render backend, kernel architecture, module system, and compile-time backend selection. (Unshelving some unfinished content - Kernel + Vulkan)

### Added
- **Vulkan render backend** (`engine/koilo/systems/render/vk/`)
 - Full Vulkan rendering pipeline with SPIR-V shader compilation
- **Vulkan display backend** (`engine/koilo/systems/display/backends/gpu/vulkanbackend.*`)
 - SDL3 + Vulkan surface integration with multi-GPU enumeration and selection
 - Discrete GPU preference with automatic fallback
 - VSync support, fullscreen toggle, and dynamic window resizing
 - `IGPUDisplayBackend` interface shared with OpenGL backend
- **Kernel architecture** (`engine/koilo/kernel/`)
 - `KoiloKernel` - central engine kernel with service registry, message bus, and module manager
 - `ServiceRegistry` - name-based typed service discovery
 - `MessageBus` - ring-buffered pub/sub messaging with deferred and immediate delivery
 - `ModuleManager` - module lifecycle, phase-ordered initialization, dependency resolution
 - `IModule` - unified module interface with Initialize/Update/Render/Shutdown lifecycle
 - Capability-based permission system for module sandboxing
 - Message types for module lifecycle, assets, scene, and ECS events
- **Kernel console** (`engine/koilo/kernel/console/`)
 - `ConsoleModule` with `CommandRegistry` for extensible command system
 - `ConsoleSession` with history, tab completion, and output formatting
 - `ConsoleSocket` - TCP server for remote console access on port 9090
 - Built-in commands: reflection introspection, memory stats, message bus, service listing, GPU info
 - `ConsoleWidget` - in-engine UI console panel
- **Kernel memory** (`engine/koilo/kernel/memory/`)
 - Arena allocator (frame-temporary, 4MB default)
 - Linear allocator (scratch buffer, 1MB default)
 - Pool allocator for fixed-size block allocation
 - Allocation tagging for memory tracking
- **Kernel asset pipeline** (`engine/koilo/kernel/asset/`)
 - Asset registry with handle-based reference management
 - Async job queue with worker thread for background asset loading
 - Asset manifest and converter registry
- **Module implementations**
 - `AssetModule` - asset loading, file watching, hot-reload via message bus
 - `PhysicsModule` - physics world integration as kernel module
 - `AIModule` - behavior tree and pathfinding as kernel module
 - `AudioModule` - audio system as kernel module
- **SPIR-V shader sources** (`shaders/`)
 - `scene.vert`, `blit.vert/frag`, `ui.vert/frag` - core Vulkan shaders
- `KOILO_USE_OPENGL` CMake option for explicit OpenGL opt-in
- `koilo_runner.cpp` - standalone script runner entry point
- Run scripts for all example projects (`examples/*/run.sh`)

### Changed
- **Vulkan is now the default render backend** - `KOILO_USE_VULKAN` defaults to ON; `KOILO_USE_OPENGL` defaults to OFF
- OpenGL backend, render code, and tests conditionally compiled behind `KL_HAVE_OPENGL_BACKEND`
- `render_backend_factory` always compiled (needed for backend selection regardless of active backend)
- `KL_HAVE_FILESYSTEM` defined independently of OpenGL backend
- KSL uber-shader references guarded behind `KL_HAVE_OPENGL_BACKEND`
- Examples converted from custom `main.cpp` entry points to script-based execution via `koilo_runner`
- Engine modules (Asset, Physics, AI, Audio) converted to `IModule` interface with kernel lifecycle
- UI Vulkan renderer uses per-frame vertex buffers with flight-frame indexing
- Scene, ECS, and world systems updated for kernel service integration

### Fixed
- Vulkan UI root panel painting opaque background over 3D content
- Vulkan UI font atlas R8 swizzle returning constant 1.0 instead of glyph alpha
- Vulkan UI vertex buffer corruption from shared buffer across frames-in-flight
- Vulkan UI overflow flush silently discarding accumulated vertices
- Vulkan swapchain crash on window resize (stale image index after recreation)

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
