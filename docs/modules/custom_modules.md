# Custom Modules

Dynamic modules extend KoiloEngine at runtime. Modules are C or C++ shared libraries (`.so` / `.dll`) that implement a fixed set of lifecycle hooks. The engine loads them, calls their hooks in phase order each frame, and exposes their objects to KoiloScript via the reflection bridge.

---

## Running Examples

Use the generic `koilo` runner with `--modules-dir` to pre-load modules:

```bash
# Build the engine and modules
./build.sh

# Run the modules demo
./build/koilo --script examples/modules/demo_all_modules.ks --modules-dir build/modules

# Or use the convenience wrapper
./examples/modules/run.sh
```

---

## Module Phases

Modules run in phase order after the script update:

| Phase | Constant | Purpose |
|-------|----------|---------|
| 0 | `KOILO_PHASE_CORE` | Math, reflection, scene (always present) |
| 1 | `KOILO_PHASE_SYSTEM` | Independent subsystems (physics, audio, AI, sensors) |
| 2 | `KOILO_PHASE_RENDER` | Pipeline stages (effects, particles) |
| 3 | `KOILO_PHASE_OVERLAY` | Drawn last (UI, debug overlays) |

---

## C API (Dynamic Modules)

Include the single-header SDK:

```c
#include <koilo/kernel/koilo_module_sdk.h>
```

### Header

Every module declares its identity with `KOILO_MODULE_HEADER`:

```c
KOILO_MODULE_HEADER("my_sensor", "1.0.0", KOILO_PHASE_SYSTEM)
```

Name is up to 31 characters. Version is informational. Phase controls init/update order.

### Lifecycle Hooks

```c
static KoiloEngineServices* svc = NULL;
static void* eng = NULL;

KOILO_MODULE_INIT {
    svc = svc_;
    eng = engine_;
    svc->register_global(eng, "my_sensor", "MySensorClass", &my_instance);
    return 1;  // non-zero = success
}

KOILO_MODULE_UPDATE {
    // dt is delta time in seconds
    my_instance.elapsed += dt;
}

KOILO_MODULE_RENDER {
    // buffer is Color888* (RGB, 3 bytes per pixel)
    // width and height are pixel dimensions
}

KOILO_MODULE_SHUTDOWN {
    svc = NULL;
    eng = NULL;
}
```

These expand to the C functions the loader resolves by symbol name: `koilo_module_init`, `koilo_module_update`, `koilo_module_render`, `koilo_module_shutdown`, and `koilo_module_get_header`.

### Engine Services

The `KoiloEngineServices` struct passed to init provides engine functionality:

```c
// Core (ABI v1)
svc->register_global(eng, name, className, instance);
svc->get_global(eng, name);
svc->set_variable(eng, "my_var", 42.0);
double val = svc->get_variable(eng, "my_var");
svc->log_info("message");
svc->log_error("error");

// Memory (ABI v2)
void* mem = svc->alloc(1024);
svc->dealloc(mem);

// Time (ABI v2)
float t  = svc->get_time(eng);
float dt = svc->get_delta_time(eng);

// Pixel buffer (ABI v2, may be NULL)
void* buf = svc->get_pixel_buffer(eng);
int w = svc->get_buffer_width(eng);
int h = svc->get_buffer_height(eng);

// Reflection (ABI v2)
svc->register_class(eng, &classDesc);
svc->register_exports(eng, exports, count);
```

Use `KOILO_HAS_API` to check availability before calling newer functions:

```c
if (KOILO_HAS_API(svc, get_pixel_buffer) && svc->get_pixel_buffer) {
    void* buf = svc->get_pixel_buffer(eng);
}
```

---

## C++ API (Static Modules)

C++ modules implement `IModule` from `koilo/kernel/unified_module.hpp`:

```cpp
class IModule {
public:
    virtual ModuleInfo GetInfo() const = 0;
    virtual bool Initialize(KoiloKernel& kernel) = 0;
    virtual void Update(float dt) = 0;
    virtual void Render(Color888* buf, int w, int h) {}   // optional
    virtual void OnMessage(const Message& msg) {}          // optional
    virtual void Shutdown() = 0;
};

struct ModuleInfo {
    const char* name;
    const char* version;
    ModulePhase phase;
};
```

Register with the module loader:

```cpp
loader.Register(std::make_unique<MyModule>());
```

The kernel reference passed to `Initialize` provides access to all subsystems via service registry:

```cpp
bool MyModule::Initialize(KoiloKernel& kernel) {
    kernel_ = &kernel;
    auto* engine = kernel.Services().Get<scripting::KoiloScriptEngine>("script");
    engine->RegisterGlobal("my_sensor", "MySensor", &sensor_instance);
    return true;
}
```

`RegisterGlobal` looks up the class name in the reflection registry and exposes the instance to scripts under the given name.

### Internal vs External Modules

| | Internal (`IModule`) | External (C ABI `.so`) |
|---|---|---|
| Language | C++ | C or C++ |
| Interface | `IModule` virtual class | `koilo_module_*` C symbols |
| Access | Full kernel via `KoiloKernel&` | Limited via `KoiloEngineServices` |
| Lifecycle | Compiled into engine | Loaded at runtime via `dlopen` |
| Hot-reload | No | Yes (file mtime check) |
| Use case | Engine subsystems | Plugins, mods, hardware drivers |

---

## Loading Modules

```cpp
ModuleLoader& loader = engine.GetModuleLoader();

// Load a single library
loader.LoadFromLibrary("./my_sensor.so");

// Scan a directory for all .so/.dll files
loader.ScanAndLoad("./plugins/");

// Lazy load by name (tries name.so, libkoilo_name.so, koilo_name.so)
loader.TryLoad("my_sensor");

// Hot-reload (desktop only, checks file mtime)
loader.ReloadModule("my_sensor");
loader.CheckAndReload();  // reloads all changed modules
```

The loader validates magic bytes (`0x4D494F4B`), ABI version, and resolves all five `koilo_module_*` symbols before accepting a module.

---

## Script Integration

```
fn Setup() {
    if (has_module("my_sensor")) {
        var temp = my_sensor.ReadTemp();
    }

    var mods = list_modules();
}
```

---

## Scaffold Tool

```bash
# Generate a new module skeleton
./tools/koilo-compile-module/koilo-new-module.sh my_sensor --dir my_sensor_module/

# Compile a module
./tools/koilo-compile-module/koilo-compile-module.sh source.c -o output.so
./tools/koilo-compile-module/koilo-compile-module.sh source.c -o output.so --target teensy41
./tools/koilo-compile-module/koilo-compile-module.sh source.c -o output.so --cflags "-O3 -DNDEBUG"
```

---

## Examples

Working examples in `examples/modules/`:

| Module | Description |
|--------|-------------|
| `fx_module.cpp` | Post-processing effects |
| `sensor_module.cpp` | Sensor data integration |
| `game_mode_module.cpp` | Game mode state management |

---

## Troubleshooting

Module won't load:
- Check file extension (`.so` on Linux, `.dll` on Windows)
- ABI version must match `KL_MODULE_ABI_VER` (currently 2)
- Header magic must be `0x4D494F4B`
- Run `nm -D module.so | grep koilo_module` - should show 5 entry points

Global not accessible from script:
- `register_global()` must be called in init
- Class name must match a reflection-registered class
- Init must return non-zero
