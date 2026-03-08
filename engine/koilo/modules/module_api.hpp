// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <cstdint>

namespace koilo {

namespace scripting { class KoiloScriptEngine; }
class Color888;  // koilo::Color888

// Module lifecycle phases for ordered initialization.
enum class ModulePhase : uint8_t {
    Core     = 0,  // Math, reflection, scene  always present
    System   = 1,  // Physics, audio, AI  independent subsystems
    Render   = 2,  // Effects, particles  render pipeline stages
    Overlay  = 3   // UI  rendered last, on top of everything
};

// Metadata for a loaded module.
struct ModuleInfo {
    const char* name    = nullptr;
    const char* version = nullptr;
    ModulePhase phase   = ModulePhase::System;
};

// Base interface for engine modules.
///
// Each module owns its subsystem instances and registers them as script
// globals during Initialize(). The engine calls Update/Render/Shutdown
// at the appropriate points in the frame loop.
class IEngineModule {
public:
    virtual ~IEngineModule() = default;

    // Module identity.
    virtual ModuleInfo GetInfo() const = 0;

    // Called once after script loading. Register globals, wire callbacks.
    // @return true on success, false to mark module as failed.
    virtual bool Initialize(scripting::KoiloScriptEngine* engine) = 0;

    // Per-frame update (called from ExecuteUpdate, after script Update).
    virtual void Update(float dt) = 0;

    // Post-render pass (effects/particles/UI write into the frame buffer).
    virtual void Render(Color888* buffer, int width, int height) = 0;

    // Cleanup. Called from engine destructor.
    virtual void Shutdown() = 0;
};

// --- C ABI for dynamically loaded modules (dlopen / MCU binary) ---

#define KL_MODULE_MAGIC   0x4D494F4B  // "KOIM"
#define KL_MODULE_ABI_VER 2

// ---- Portable reflection descriptors (C ABI) ----

// Type tags for C ABI method arguments and return values.
enum KoiloFieldKind : uint32_t {
    KL_KIND_NONE    = 0,
    KL_KIND_FLOAT   = 1,
    KL_KIND_INT     = 2,
    KL_KIND_BOOL    = 3,
    KL_KIND_STRING  = 4,
    KL_KIND_VEC3    = 5,
    KL_KIND_COLOR   = 6,
    KL_KIND_OBJECT  = 7,  // opaque void*
    KL_KIND_VOID    = 8
};

// C ABI method descriptor  one per exported method.
struct KoiloMethodExport {
    const char* name;                           ///< Method name (e.g., "SetGravity")
    const char* doc;                            ///< Brief description (nullable)
    void* (*invoker)(void* self, void** args);  ///< Invocation function pointer
    uint8_t  argc;                              ///< Argument count
    uint8_t  is_static;                         ///< 1 if static method
    KoiloFieldKind ret_kind;                      ///< Return value type tag
    KoiloFieldKind arg_kinds[8];                  ///< Argument type tags (max 8)
};

// C ABI class descriptor  describes one reflected type.
// Modules fill these statically; engine registers them during init.
struct ClassDescExport {
    const char* name;                           ///< Class name (e.g., "MySystem")
    uint32_t    size;                           ///< sizeof(T)
    void (*destroy)(void*);                     ///< Destructor (nullable if engine-owned)
    const KoiloMethodExport* methods;             ///< Array of method descriptors
    uint32_t method_count;
    uint32_t reserved[2];
};

// C ABI global export  a named object instance with its class descriptor.
// Modules return an array of these from init; engine calls RegisterGlobal for each.
struct GlobalExport {
    const char* name;                           ///< Script global name (e.g., "sensor")
    const char* class_name;                     ///< Matching ClassDescExport::name
    void* instance;                             ///< Object pointer (module owns lifetime)
    uint32_t flags;                             ///< Reserved
};

// C-compatible module header (embedded at start of .kmod binary).
struct KoiloModuleHeader {
    uint32_t magic;
    uint32_t abi_version;
    char     name[32];
    char     version[16];
    uint32_t phase;        // ModulePhase cast to uint32_t
    uint32_t flags;
    uint32_t reserved[4];
};

// C function pointer table  engine services exposed to dynamic modules.
// Layout is append-only: new functions are added at the end, never reordered.
// Modules check api_size >= offsetof(EngineServices, needed_field) before use.
struct EngineServices {
    uint32_t api_size;     ///< sizeof(EngineServices)  for forward compatibility

    // --- Core (always available) ---
    void  (*register_global)(void* engine, const char* name, const char* className, void* instance);
    void* (*get_global)(void* engine, const char* name);
    void  (*set_variable)(void* engine, const char* name, double value);
    double(*get_variable)(void* engine, const char* name);
    void  (*log_info)(const char* msg);
    void  (*log_error)(const char* msg);

    // --- Memory (ABI v2) ---
    void* (*alloc)(uint32_t size);
    void  (*dealloc)(void* ptr);

    // --- Time (ABI v2) ---
    float (*get_time)(void* engine);
    float (*get_delta_time)(void* engine);

    // --- Pixel buffer (ABI v2, nullable  NULL if no render target) ---
    void* (*get_pixel_buffer)(void* engine);
    int   (*get_buffer_width)(void* engine);
    int   (*get_buffer_height)(void* engine);

    // --- Reflection (ABI v2) ---
    int   (*register_class)(void* engine, const ClassDescExport* desc);
    int   (*register_exports)(void* engine, const GlobalExport* exports, uint32_t count);

    void* reserved[4];
};

// C entry points that a dynamic module exports.
extern "C" {
    typedef KoiloModuleHeader* (*KoiloModuleGetHeaderFunc)();
    typedef int  (*KoiloModuleInitFunc)(EngineServices* services, void* engine);
    typedef void (*KoiloModuleUpdateFunc)(float dt);
    typedef void (*KoiloModuleRenderFunc)(void* buffer, int width, int height);
    typedef void (*KoiloModuleShutdownFunc)();
}

} // namespace koilo
