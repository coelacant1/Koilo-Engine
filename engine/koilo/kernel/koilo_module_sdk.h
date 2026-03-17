// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file koilo_module_sdk.h
 * @brief Single-header SDK for authoring KoiloEngine dynamic modules.
 *
 * Include this file in your module source. It provides:
 * - All necessary type definitions from the engine ABI
 * - Convenience macros for declaring modules
 * - Helper macros for registering globals and methods
 *
 * Usage:
 * @code
 * #include "koilo_module_sdk.h"
 *
 * static KoiloEngineServices* svc = NULL;
 * static void* engine_ptr = NULL;
 *
 * // Your subsystem
 * typedef struct { float value; } MySystem;
 * static MySystem sys;
 *
 * KOILO_MODULE_HEADER("my_module", "1.0.0", KOILO_PHASE_SYSTEM)
 *
 * KOILO_MODULE_INIT {
 *     sys.value = 42.0f;
 *     svc->register_global(engine_ptr, "my_sys", "MySystem", &sys);
 *     return 1;
 * }
 *
 * KOILO_MODULE_UPDATE { (void)dt; }
 * KOILO_MODULE_RENDER { (void)buffer; (void)width; (void)height; }
 * KOILO_MODULE_SHUTDOWN { sys.value = 0; }
 * @endcode
 */
#ifndef KL_MODULE_SDK_H
#define KL_MODULE_SDK_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ---- ABI Constants ---- */

#define KL_MODULE_MAGIC   0x4D494F4B  /* "KOIM" */
#define KL_MODULE_ABI_VER 2

/* Module phases - controls initialization and rendering order. */
#define KOILO_PHASE_CORE     0
#define KOILO_PHASE_SYSTEM   1
#define KOILO_PHASE_RENDER   2
#define KOILO_PHASE_OVERLAY  3

/* Legacy aliases */
#define KL_PHASE_CORE     KOILO_PHASE_CORE
#define KL_PHASE_SYSTEM   KOILO_PHASE_SYSTEM
#define KL_PHASE_RENDER   KOILO_PHASE_RENDER
#define KL_PHASE_OVERLAY  KOILO_PHASE_OVERLAY

/* ---- ABI Type Tags ---- */

#define KL_KIND_NONE    0
#define KL_KIND_FLOAT   1
#define KL_KIND_INT     2
#define KL_KIND_BOOL    3
#define KL_KIND_STRING  4
#define KL_KIND_VEC3    5
#define KL_KIND_COLOR   6
#define KL_KIND_OBJECT  7
#define KL_KIND_VOID    8

/* ---- ABI Structs ---- */

typedef struct {
    uint32_t magic;
    uint32_t abi_version;
    char     name[32];
    char     version[16];
    uint32_t phase;
    uint32_t flags;
    uint32_t reserved[4];
} KoiloModuleHeader;

typedef struct {
    const char* name;
    const char* doc;
    void* (*invoker)(void* self, void** args);
    uint8_t  argc;
    uint8_t  is_static;
    uint32_t ret_kind;
    uint32_t arg_kinds[8];
} KoiloMethodExport;

typedef struct {
    const char* name;
    uint32_t    size;
    void (*destroy)(void*);
    const KoiloMethodExport* methods;
    uint32_t method_count;
    uint32_t reserved[2];
} KoiloClassDescExport;

typedef struct {
    const char* name;
    const char* class_name;
    void* instance;
    uint32_t flags;
} KoiloGlobalExport;

typedef struct {
    uint32_t api_size;

    /* Core */
    void  (*register_global)(void* engine, const char* name, const char* className, void* instance);
    void* (*get_global)(void* engine, const char* name);
    void  (*set_variable)(void* engine, const char* name, double value);
    double(*get_variable)(void* engine, const char* name);
    void  (*log_info)(const char* msg);
    void  (*log_error)(const char* msg);

    /* Memory (ABI v2) */
    void* (*alloc)(uint32_t size);
    void  (*dealloc)(void* ptr);

    /* Time (ABI v2) */
    float (*get_time)(void* engine);
    float (*get_delta_time)(void* engine);

    /* Pixel buffer (ABI v2, may be NULL) */
    void* (*get_pixel_buffer)(void* engine);
    int   (*get_buffer_width)(void* engine);
    int   (*get_buffer_height)(void* engine);

    /* Reflection (ABI v2) */
    int   (*register_class)(void* engine, const KoiloClassDescExport* desc);
    int   (*register_exports)(void* engine, const KoiloGlobalExport* exports, uint32_t count);

    void* reserved[4];
} KoiloEngineServices;

/* ---- Convenience Macros ---- */

/**
 * Declare the module header as a static global and export the getter.
 * @param _name    Module name string (max 31 chars)
 * @param _version Version string (max 15 chars)
 * @param _phase   One of KOILO_PHASE_CORE/SYSTEM/RENDER/OVERLAY
 */
#define KOILO_MODULE_HEADER(_name, _version, _phase)                       \
    static KoiloModuleHeader koilo_header_ = {                             \
        KL_MODULE_MAGIC, KL_MODULE_ABI_VER,                                \
        {0}, {0}, (_phase), 0, {0}                                         \
    };                                                                     \
    __attribute__((constructor)) static void koilo_init_header_(void) {    \
        size_t nlen = sizeof(_name) - 1;                                   \
        if (nlen > 31) nlen = 31;                                          \
        for (size_t i = 0; i < nlen; ++i) koilo_header_.name[i] = (_name)[i]; \
        size_t vlen = sizeof(_version) - 1;                                \
        if (vlen > 15) vlen = 15;                                          \
        for (size_t i = 0; i < vlen; ++i) koilo_header_.version[i] = (_version)[i]; \
    }                                                                      \
    KoiloModuleHeader* koilo_module_get_header(void) { return &koilo_header_; }

/**
 * Begin the init function. The body receives `svc_` and `engine_`.
 * Must return non-zero on success.
 */
#define KOILO_MODULE_INIT                                                  \
    int koilo_module_init(KoiloEngineServices* svc_, void* engine_)

/**
 * Begin the update function. Body receives `dt` (float).
 */
#define KOILO_MODULE_UPDATE                                                \
    void koilo_module_update(float dt)

/**
 * Begin the render function. Body receives `buffer`, `width`, `height`.
 */
#define KOILO_MODULE_RENDER                                                \
    void koilo_module_render(void* buffer, int width, int height)

/**
 * Begin the shutdown function.
 */
#define KOILO_MODULE_SHUTDOWN                                              \
    void koilo_module_shutdown(void)

/* Legacy macro aliases */
#define KL_MODULE_HEADER  KOILO_MODULE_HEADER
#define KL_MODULE_INIT    KOILO_MODULE_INIT
#define KL_MODULE_UPDATE  KOILO_MODULE_UPDATE
#define KL_MODULE_RENDER  KOILO_MODULE_RENDER
#define KL_MODULE_SHUTDOWN KOILO_MODULE_SHUTDOWN

/* ---- Utility: check if an API function is available ---- */

#define KOILO_HAS_API(svc, field)                                          \
    ((svc)->api_size >= (uint32_t)(offsetof(KoiloEngineServices, field) + sizeof((svc)->field)))

/* Legacy alias */
#define KL_HAS_API KOILO_HAS_API

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* KL_MODULE_SDK_H */
