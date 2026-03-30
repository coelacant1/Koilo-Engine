// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file module_api.hpp
 * @brief C ABI for dynamically loaded modules.
 *
 * @date 01/22/2026
 * @author Coela
 */
#pragma once

#include <koilo/kernel/unified_module.hpp>

namespace koilo {

// --- C ABI for dynamically loaded modules (dlopen / MCU binary) ---

#define KL_MODULE_MAGIC   0x4D494F4B  // "KOIM"
#define KL_MODULE_ABI_VER 3

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

// ---- ABI v3 descriptor structs (C-compatible) ----

/// Describes a console command registered by a module.
struct KoiloCommandDesc {
    const char* name;                               ///< Command name (e.g., "spawn")
    const char* help;                               ///< One-line help text (nullable)
    int (*handler)(const char** args, int argc);    ///< Handler function
    uint32_t reserved;
};

/// Describes an input listener registered by a module.
struct KoiloInputListenerDesc {
    const char* name;                               ///< Listener name
    int priority;                                   ///< Higher = receives events first
    int (*on_key)(int key, int action, int mods);   ///< Return 1 to consume
    int (*on_mouse_button)(int button, int action, float x, float y);
    int (*on_mouse_move)(float x, float y, float dx, float dy);
    int (*on_scroll)(float dx, float dy);
    uint32_t reserved;
};

/// Describes a component type registered by a module.
struct KoiloComponentDesc {
    const char* name;                               ///< Component type name
    uint32_t size;                                  ///< sizeof(T)
    uint32_t alignment;                             ///< alignof(T)
    void (*construct)(void* dest);                  ///< Placement constructor
    void (*destruct)(void* ptr);                    ///< Destructor
    void (*copy)(void* dest, const void* src);      ///< Copy assignment
    uint32_t reserved;
};

/// Describes a custom widget type registered by a module.
struct KoiloWidgetTypeDesc {
    const char* name;                               ///< Widget type name
    void* (*create)(void);                          ///< Returns opaque widget state
    void  (*destroy)(void* state);                  ///< Destroy widget state
    void  (*render)(void* state, void* draw_ctx);   ///< Render via CanvasDrawContext
    void  (*on_click)(void* state);                 ///< Click handler (nullable)
    uint32_t reserved;
};

/// Describes a render pass registered by a module.
struct KoiloRenderPassDesc {
    const char* name;                               ///< Pass name (e.g., "fog")
    uint32_t order;                                 ///< Execution order hint
    void (*execute)(void* context);                 ///< Execute the pass
    uint32_t reserved;
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

    // --- Extension points (ABI v3) ---
    int   (*register_command)(void* engine, const KoiloCommandDesc* desc);
    int   (*register_input_listener)(void* engine, const KoiloInputListenerDesc* desc);
    int   (*register_component)(void* engine, const KoiloComponentDesc* desc);
    int   (*register_widget_type)(void* engine, const KoiloWidgetTypeDesc* desc);
    int   (*register_render_pass)(void* engine, const KoiloRenderPassDesc* desc);

    void* reserved_v3[4];
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
