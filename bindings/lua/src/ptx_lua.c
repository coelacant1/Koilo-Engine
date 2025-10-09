/*
 * Minimal Lua binding for PTX reflection (prototype)
 * Exposes a tiny surface for desktop prototyping and MCU integration.
 *
 * API (Lua):
 *   ptx.list_classes()
 *   ptx.new(class_name) -> userdata (instance)
 *   instance:get(field_name)
 *   instance:set(field_name, value)
 *   instance:call(method_name, ...)
 *
 * This file links against the PTX reflection C API (reflect.h).
 */

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

/* Forward decl for method closure trampoline (needs lua_State) */
static int instance_method_trampoline(lua_State *Linner);

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "../../c_api/reflect.h"

typedef struct {
    void *class_desc;    /* ptx::ClassDesc* as void* */
    void *instance;      /* pointer to underlying instance */
    int   needs_destroy; /* 1 if owned via class destroy, 2 if method return needing method_destroy_return */
    void *origin_method; /* method descriptor for destroy_return when needs_destroy==2 */
} ptx_instance_t;

/* -------------------- Global Caching Layer -------------------- */
/* Build a lightweight cache per class the first time to avoid
 * repeated descriptor searches and type string parsing. */

typedef enum {
    PTX_TAG_UNKNOWN = 0,
    PTX_TAG_I8, PTX_TAG_U8,
    PTX_TAG_I16, PTX_TAG_U16,
    PTX_TAG_I32, PTX_TAG_U32,
    PTX_TAG_I64, PTX_TAG_U64,
    PTX_TAG_F32, PTX_TAG_F64,
    PTX_TAG_BOOL,
    PTX_TAG_CLASS /* object/class return or argument (not yet fully supported) */
} ptx_type_tag;

typedef struct {
    const void *decl; /* ptx_field_decl* */
    ptx_type_tag tag;
    size_t size;
} ptx_cached_field;

typedef struct {
    const void *decl; /* ptx_method_desc* */
    ptx_type_tag ret_tag;
    size_t argc;
    ptx_type_tag arg_tags[16]; /* cap for now */
    int is_static;
} ptx_cached_method;

typedef struct {
    const void *decl; /* ptx_constructor_desc* */
    size_t argc;
    ptx_type_tag arg_tags[16];
} ptx_cached_ctor;

typedef struct ptx_class_cache_s {
    const char *name;
    const void *class_desc;
    int fields_built;
    int methods_built;
    int ctors_built;
    size_t field_count;
    size_t method_count;
    size_t ctor_count;
    ptx_cached_field *fields;    /* allocated arrays */
    ptx_cached_method *methods;
    ptx_cached_ctor *ctors;
    struct ptx_class_cache_s *next;
} ptx_class_cache;

static ptx_class_cache *g_class_cache_head = NULL;

static ptx_type_tag map_type_tag(const void *type_info) {
    if (!type_info) return PTX_TAG_UNKNOWN;
    const char *n = ptx_type_info_name(type_info);
    if (!n) return PTX_TAG_UNKNOWN;
    /* Order matters: test longer substrings earlier if overlaps */
    if (strstr(n, "uint64") || strstr(n, "unsigned long long")) return PTX_TAG_U64;
    if (strstr(n, "int64") || strstr(n, "long long")) return PTX_TAG_I64;
    if (strcmp(n, "h") == 0) return PTX_TAG_U8; /* special abbrev */
    if (strstr(n, "uint32") || strstr(n, "unsigned")) return PTX_TAG_U32;
    if (strstr(n, "int32") || strstr(n, "int")) return PTX_TAG_I32;
    if (strstr(n, "uint16")) return PTX_TAG_U16;
    if (strstr(n, "int16")) return PTX_TAG_I16;
    if (strstr(n, "uint8") ) return PTX_TAG_U8;
    if (strstr(n, "int8") || strstr(n, "char")) return PTX_TAG_I8;
    if (strstr(n, "double")) return PTX_TAG_F64;
    if (strstr(n, "float")) return PTX_TAG_F32;
    if (strstr(n, "bool")) return PTX_TAG_BOOL;
    return PTX_TAG_UNKNOWN;
}

static ptx_class_cache* get_or_build_class_cache(const void *cls) {
    if (!cls) return NULL;
    const char *name = ptx_class_name(cls);
    for (ptx_class_cache *c = g_class_cache_head; c; c = c->next) {
        if (c->class_desc == cls) return c;
        if (c->name && name && strcmp(c->name, name)==0) return c; /* fallback by name */
    }
    /* allocate new */
    ptx_class_cache *cc = (ptx_class_cache*)malloc(sizeof(ptx_class_cache));
    memset(cc, 0, sizeof(*cc));
    cc->name = name; cc->class_desc = cls; cc->next = g_class_cache_head; g_class_cache_head = cc;
    return cc;
}

static void build_field_cache(ptx_class_cache *cc) {
    if (!cc || cc->fields_built) return;
    size_t count = ptx_class_field_count(cc->class_desc);
    cc->field_count = count;
    if (count) cc->fields = (ptx_cached_field*)malloc(sizeof(ptx_cached_field)*count);
    for (size_t i=0;i<count;++i) {
        const void *f = ptx_class_field_at(cc->class_desc, i);
        cc->fields[i].decl = f;
        cc->fields[i].size = f ? ptx_field_size(f) : 0;
        const void *ftype = f ? ptx_field_type(f) : NULL;
        cc->fields[i].tag = map_type_tag(ftype);
    }
    cc->fields_built = 1;
}

static void build_method_cache(ptx_class_cache *cc) {
    if (!cc || cc->methods_built) return;
    size_t count = ptx_class_method_count(cc->class_desc);
    cc->method_count = count;
    if (count) cc->methods = (ptx_cached_method*)malloc(sizeof(ptx_cached_method)*count);
    for (size_t i=0;i<count;++i) {
        const void *m = ptx_class_method_at(cc->class_desc, i);
        cc->methods[i].decl = m;
        cc->methods[i].argc = m ? ptx_method_arg_count(m) : 0;
        cc->methods[i].is_static = m ? (ptx_method_is_static(m)?1:0) : 0;
        /* return tag */
        const void *rt = m ? ptx_method_return_type(m) : NULL;
        cc->methods[i].ret_tag = map_type_tag(rt);
        size_t argc = cc->methods[i].argc;
        if (argc > 16) argc = 16; /* clamp */
        for (size_t a=0;a<argc;++a) {
            const void *at = ptx_method_arg_type(m, a);
            cc->methods[i].arg_tags[a] = map_type_tag(at);
        }
        for (size_t a=argc;a<16;++a) cc->methods[i].arg_tags[a] = PTX_TAG_UNKNOWN;
    }
    cc->methods_built = 1;
}

static void build_ctor_cache(ptx_class_cache *cc) {
    if (!cc || cc->ctors_built) return;
    size_t count = ptx_class_constructor_count(cc->class_desc);
    cc->ctor_count = count;
    if (count) cc->ctors = (ptx_cached_ctor*)malloc(sizeof(ptx_cached_ctor)*count);
    for (size_t i=0;i<count;++i) {
        const void *c = ptx_class_constructor_at(cc->class_desc, i);
        cc->ctors[i].decl = c;
        cc->ctors[i].argc = c ? ptx_constructor_arg_count(c) : 0;
        size_t argc = cc->ctors[i].argc;
        if (argc > 16) argc = 16;
        for (size_t a=0;a<argc;++a) {
            const void *at = ptx_constructor_arg_type(c, a);
            cc->ctors[i].arg_tags[a] = map_type_tag(at);
        }
        for (size_t a=argc;a<16;++a) cc->ctors[i].arg_tags[a] = PTX_TAG_UNKNOWN;
    }
    cc->ctors_built = 1;
}

/* Helpers to box a single argument according to tag. Returns 0 on success, lua_error on fail. */
static int box_arg_from_lua(lua_State *L, int lua_index, ptx_type_tag tag, void *storage_slot) {
    switch(tag) {
        case PTX_TAG_U8: *(uint8_t*)storage_slot = (uint8_t)luaL_checkinteger(L, lua_index); return 0;
        case PTX_TAG_I8: *(int8_t*)storage_slot = (int8_t)luaL_checkinteger(L, lua_index); return 0;
        case PTX_TAG_U16: *(uint16_t*)storage_slot = (uint16_t)luaL_checkinteger(L, lua_index); return 0;
        case PTX_TAG_I16: *(int16_t*)storage_slot = (int16_t)luaL_checkinteger(L, lua_index); return 0;
        case PTX_TAG_U32: *(uint32_t*)storage_slot = (uint32_t)luaL_checkinteger(L, lua_index); return 0;
        case PTX_TAG_I32: *(int32_t*)storage_slot = (int32_t)luaL_checkinteger(L, lua_index); return 0;
        case PTX_TAG_U64: *(uint64_t*)storage_slot = (uint64_t)luaL_checkinteger(L, lua_index); return 0;
        case PTX_TAG_I64: *(int64_t*)storage_slot = (int64_t)luaL_checkinteger(L, lua_index); return 0;
        case PTX_TAG_F32: *(double*)storage_slot = (double)luaL_checknumber(L, lua_index); return 0; /* store as double then reinterpret for float if needed */
        case PTX_TAG_F64: *(double*)storage_slot = (double)luaL_checknumber(L, lua_index); return 0;
        case PTX_TAG_BOOL: *(bool*)storage_slot = lua_toboolean(L, lua_index)?true:false; return 0;
        default: return luaL_error(L, "unsupported arg tag %d", (int)tag);
    }
}

/* Extract primitive return based on tag. Returns number of Lua return values pushed (0 or 1). */
static int push_return_for_tag(lua_State *L, ptx_type_tag tag, void *ret_mem, const void *method_desc) {
    if (!ret_mem) { lua_pushnil(L); return 1; }
    switch(tag) {
        case PTX_TAG_I8: lua_pushinteger(L, *(int8_t*)ret_mem); break;
        case PTX_TAG_U8: lua_pushinteger(L, *(uint8_t*)ret_mem); break;
        case PTX_TAG_I16: lua_pushinteger(L, *(int16_t*)ret_mem); break;
        case PTX_TAG_U16: lua_pushinteger(L, *(uint16_t*)ret_mem); break;
        case PTX_TAG_I32: lua_pushinteger(L, *(int32_t*)ret_mem); break;
        case PTX_TAG_U32: lua_pushinteger(L, *(uint32_t*)ret_mem); break;
        case PTX_TAG_I64: lua_pushinteger(L, (lua_Integer)*(int64_t*)ret_mem); break;
        case PTX_TAG_U64: lua_pushinteger(L, (lua_Integer)*(uint64_t*)ret_mem); break;
        case PTX_TAG_F32: lua_pushnumber(L, (lua_Number)*(float*)ret_mem); break; /* careful if ret_mem actually holds double; reflection returns pointer to real type */
        case PTX_TAG_F64: lua_pushnumber(L, (lua_Number)*(double*)ret_mem); break;
        case PTX_TAG_BOOL: lua_pushboolean(L, (*(bool*)ret_mem)?1:0); break;
        default:
            /* attempt class fallback as before */
            lua_pushnil(L);
            break;
    }
    if (method_desc) ptx_method_destroy_return(method_desc, ret_mem);
    return 1;
}

/* Ensure a per-userdata cache table exists as the userdata's uservalue.
 * Leaves the cache table on top of the stack. Returns 1 if created, 0 if already existed. */
static int ensure_instance_cache(lua_State *L, int ud_index) {
    /* normalize index */
    if (ud_index < 0) ud_index = lua_gettop(L) + 1 + ud_index;
    lua_getuservalue(L, ud_index); /* push uservalue */
    if (!lua_istable(L, -1)) {
        lua_pop(L, 1);
        lua_newtable(L);
        /* raw set as uservalue */
        lua_pushvalue(L, -1);
        lua_setuservalue(L, ud_index);
        /* leave the table on stack */
        return 1;
    }
    return 0;
}

/* ptx.list_classes() */
static int l_ptx_list_classes(lua_State *L) {
    size_t count = ptx_registry_class_count();
    lua_createtable(L, (int)count, 0);
    for (size_t i = 0; i < count; ++i) {
        const void *c = ptx_registry_class_at(i);
        const char *name = ptx_class_name((void*)c);
        lua_pushstring(L, name ? name : "");
        lua_rawseti(L, -2, (int)(i + 1));
    }
    return 1;
}

/* garbage collect instance */
static int instance_gc(lua_State *L) {
    ptx_instance_t *inst = (ptx_instance_t*)lua_touserdata(L, 1);
    if (inst && inst->instance) {
        if (inst->needs_destroy == 1) {
            ptx_class_destroy_instance(inst->class_desc, inst->instance);
        } else if (inst->needs_destroy == 2 && inst->origin_method) {
            ptx_method_destroy_return(inst->origin_method, inst->instance);
        }
        inst->instance = NULL;
    }
    return 0;
}

/* instance:get(field_name) */
static int instance_get(lua_State *L) {
    ptx_instance_t *inst = (ptx_instance_t*)luaL_checkudata(L, 1, "ptx.instance");
    build_field_cache(get_or_build_class_cache(inst->class_desc));
    /* Allow second arg to be either a string (field name) or lightuserdata (cached field pointer) */
    const void *field = NULL;
    int type2 = lua_type(L, 2);
    const char *fname = NULL;
    if (type2 == LUA_TLIGHTUSERDATA) {
        field = lua_touserdata(L, 2);
    } else {
        fname = luaL_checkstring(L, 2);
        field = ptx_class_find_field(inst->class_desc, (const char*)fname);
        if (!field) { return luaL_error(L, "field '%s' not found", fname); }
    }
    size_t fsize = ptx_field_size((void*)field);
    const void *ptr = ptx_field_get_const_ptr((void*)field, inst->instance);
    if (!ptr) { lua_pushnil(L); return 1; }
    /* support small scalar types common in reflection (int/float/bool/uint16) */
    const char *tname = ptx_type_info_name(ptx_field_type((void*)field));
    if (!tname) { lua_pushnil(L); return 1; }
    /* crude mapping */
    /* size-aware mapping to avoid reading beyond small fields */
    if (fsize == 1) {
        /* Treat as unsigned byte */
        uint8_t v = *(const uint8_t*)ptr;
        lua_pushinteger(L, (lua_Integer)v);
        return 1;
    }
    if (fsize == 2) {
        uint16_t v = *(const uint16_t*)ptr;
        lua_pushinteger(L, (lua_Integer)v);
        return 1;
    }
    if (fsize == 4) {
        /* could be float or 32-bit int */
        if (strstr(tname, "float")) {
            float v = *(const float*)ptr;
            lua_pushnumber(L, (lua_Number)v);
        } else if (strstr(tname, "uint") || strstr(tname, "unsigned")) {
            uint32_t v = *(const uint32_t*)ptr;
            lua_pushinteger(L, (lua_Integer)v);
        } else {
            int32_t v = *(const int32_t*)ptr;
            lua_pushinteger(L, (lua_Integer)v);
        }
        return 1;
    }
    if (fsize == 8) {
        if (strstr(tname, "double") || strstr(tname, "float")) {
            double v = *(const double*)ptr;
            lua_pushnumber(L, (lua_Number)v);
            return 1;
        }
    }
    /* fallback: push nil */
    lua_pushnil(L);
    return 1;
}

/* instance:set(field_name, value) */
static int instance_set(lua_State *L) {
    ptx_instance_t *inst = (ptx_instance_t*)luaL_checkudata(L, 1, "ptx.instance");
    build_field_cache(get_or_build_class_cache(inst->class_desc));
    /* Allow second arg to be lightuserdata (cached field pointer) or string (name) */
    const void *field = NULL;
    int type2 = lua_type(L, 2);
    const char *fname = NULL;
    if (type2 == LUA_TLIGHTUSERDATA) {
        field = lua_touserdata(L, 2);
    } else {
        fname = luaL_checkstring(L, 2);
        field = ptx_class_find_field(inst->class_desc, (const char*)fname);
        if (!field) { return luaL_error(L, "field '%s' not found", fname); }
    }
    void *ptr = ptx_field_get_ptr((void*)field, inst->instance);
    if (!ptr) { return luaL_error(L, "field pointer is null"); }
    const char *tname = ptx_type_info_name(ptx_field_type((void*)field));
    if (!tname) return luaL_error(L, "unknown field type");
    size_t fsize = ptx_field_size((void*)field);
    if (fsize == 1) {
        uint8_t v = (uint8_t)luaL_checkinteger(L, 3);
        *(uint8_t*)ptr = v;
        return 0;
    }
    if (fsize == 2) {
        uint16_t v = (uint16_t)luaL_checkinteger(L, 3);
        *(uint16_t*)ptr = v;
        return 0;
    }
    if (fsize == 4) {
        if (strstr(tname, "float")) {
            float v = (float)luaL_checknumber(L, 3);
            *(float*)ptr = v;
        } else if (strstr(tname, "uint") || strstr(tname, "unsigned")) {
            uint32_t v = (uint32_t)luaL_checkinteger(L, 3);
            *(uint32_t*)ptr = v;
        } else {
            int32_t v = (int32_t)luaL_checkinteger(L, 3);
            *(int32_t*)ptr = v;
        }
        return 0;
    }
    if (fsize == 8 && (strstr(tname, "double") || strstr(tname, "float"))) {
        double v = (double)luaL_checknumber(L, 3);
        *(double*)ptr = v;
        return 0;
    }
    return luaL_error(L, "unsupported field type");
}

/* instance:call(method_name, ...) - now supports scalar arguments & returns */
static int instance_call(lua_State *L) {
    ptx_instance_t *inst = (ptx_instance_t*)luaL_checkudata(L, 1, "ptx.instance");
    const char *mname = luaL_checkstring(L, 2);
    /* Use method cache */
    ptx_class_cache *cc = get_or_build_class_cache(inst->class_desc);
    build_method_cache(cc);
    const void *method = NULL; ptx_cached_method *mcached = NULL;
    for (size_t i=0;i<cc->method_count;++i) {
        const ptx_cached_method *cm = &cc->methods[i];
        const char *nm = ptx_method_name(cm->decl);
        if (nm && strcmp(nm, mname)==0 && !cm->is_static) { method = cm->decl; mcached = (ptx_cached_method*)cm; break; }
    }
    if (!method) { return luaL_error(L, "method '%s' not found", mname); }
    size_t argcount = mcached->argc;
    int provided = lua_gettop(L) - 2; /* exclude self + method_name */
    if ((int)argcount != provided) { return luaL_error(L, "method '%s' expects %d args, got %d", mname, (int)argcount, provided); }

    /* Box arguments */
    enum { MAX_ARGS = 16 };
    if (argcount > MAX_ARGS) return luaL_error(L, "too many args (%d)", (int)argcount);
    /* storage for primitive values */
    typedef union { int8_t i8; uint8_t u8; int16_t i16; uint16_t u16; int32_t i32; uint32_t u32; int64_t i64; uint64_t u64; double d; bool b; float f; } scalar_t;
    scalar_t storage[MAX_ARGS];
    void *argv[MAX_ARGS];
    for (size_t i=0;i<argcount;++i) {
        int lua_index = 3 + (int)i;
        if (box_arg_from_lua(L, lua_index, mcached->arg_tags[i], &storage[i]) != 0) return lua_error(L);
        /* map union member selection to pointer */
        switch(mcached->arg_tags[i]) {
            case PTX_TAG_I8: argv[i] = &storage[i].i8; break;
            case PTX_TAG_U8: argv[i] = &storage[i].u8; break;
            case PTX_TAG_I16: argv[i] = &storage[i].i16; break;
            case PTX_TAG_U16: argv[i] = &storage[i].u16; break;
            case PTX_TAG_I32: argv[i] = &storage[i].i32; break;
            case PTX_TAG_U32: argv[i] = &storage[i].u32; break;
            case PTX_TAG_I64: argv[i] = &storage[i].i64; break;
            case PTX_TAG_U64: argv[i] = &storage[i].u64; break;
            case PTX_TAG_F32: argv[i] = &storage[i].d; break; /* stored as double */
            case PTX_TAG_F64: argv[i] = &storage[i].d; break;
            case PTX_TAG_BOOL: argv[i] = &storage[i].b; break;
            default: return luaL_error(L, "unsupported arg tag %d", (int)mcached->arg_tags[i]);
        }
    }
    void *ret = ptx_method_invoke((void*)method, inst->instance, (argcount ? (void**)argv : NULL));
    if (!ret) { lua_pushnil(L); return 1; }
    /* Primitive return fast path */
    if (mcached->ret_tag != PTX_TAG_UNKNOWN && mcached->ret_tag != PTX_TAG_CLASS) {
        return push_return_for_tag(L, mcached->ret_tag, ret, method);
    }
    /* fallback: return userdata wrapper if class-like */
    const void *rtype = ptx_method_return_type(method);
    const char *pretty = rtype ? ptx_type_info_pretty_name(rtype) : NULL;
    if (pretty) {
        const void *retcls = ptx_registry_find_class(pretty);
        if (retcls) {
            ptx_instance_t *ud = (ptx_instance_t*)lua_newuserdata(L, sizeof(ptx_instance_t));
            ud->class_desc = (void*)retcls;
            ud->instance = ret;
            ud->needs_destroy = 2; /* method return */
            ud->origin_method = (void*)method;
            luaL_getmetatable(L, "ptx.instance");
            lua_setmetatable(L, -2);
            return 1;
        }
    }
    /* unknown return: push nil */
    ptx_method_destroy_return(method, ret);
    lua_pushnil(L);
    return 1;
}

/* ptx.new(class_name, ...) - construct via matching argcount */
static int l_ptx_new(lua_State *L) {
    const char *cname = luaL_checkstring(L, 1);
    int provided = lua_gettop(L) - 1; /* args after class name */
    const void *cls = ptx_registry_find_class(cname);
    if (!cls) { return luaL_error(L, "class '%s' not found", cname); }
    ptx_class_cache *cc = get_or_build_class_cache(cls);
    build_ctor_cache(cc);
    const ptx_cached_ctor *chosen = NULL;
    for (size_t i=0;i<cc->ctor_count;++i) {
        if ((int)cc->ctors[i].argc == provided) { chosen = &cc->ctors[i]; break; }
    }
    if (!chosen) { return luaL_error(L, "no constructor on '%s' accepts %d args", cname, provided); }
    size_t argc = chosen->argc;
    enum { MAX_CTOR_ARGS = 16 };
    if (argc > MAX_CTOR_ARGS) { lua_pushnil(L); return 1; }
    typedef union { int8_t i8; uint8_t u8; int16_t i16; uint16_t u16; int32_t i32; uint32_t u32; int64_t i64; uint64_t u64; double d; bool b; float f; } scalar_t;
    scalar_t storage[MAX_CTOR_ARGS];
    void *argv[MAX_CTOR_ARGS];
    for (size_t i=0;i<argc;++i) {
        int lua_index = 2 + (int)i;
        if (box_arg_from_lua(L, lua_index, chosen->arg_tags[i], &storage[i]) != 0) return lua_error(L);
        switch(chosen->arg_tags[i]) {
            case PTX_TAG_I8: argv[i] = &storage[i].i8; break; case PTX_TAG_U8: argv[i] = &storage[i].u8; break;
            case PTX_TAG_I16: argv[i] = &storage[i].i16; break; case PTX_TAG_U16: argv[i] = &storage[i].u16; break;
            case PTX_TAG_I32: argv[i] = &storage[i].i32; break; case PTX_TAG_U32: argv[i] = &storage[i].u32; break;
            case PTX_TAG_I64: argv[i] = &storage[i].i64; break; case PTX_TAG_U64: argv[i] = &storage[i].u64; break;
            case PTX_TAG_F32: argv[i] = &storage[i].d; break; case PTX_TAG_F64: argv[i] = &storage[i].d; break;
            case PTX_TAG_BOOL: argv[i] = &storage[i].b; break;
            default: return luaL_error(L, "unsupported ctor arg tag %d", (int)chosen->arg_tags[i]);
        }
    }
    void *inst_ptr = ptx_constructor_invoke((void*)chosen->decl, argc ? (void**)argv : NULL);
    if (!inst_ptr) { lua_pushnil(L); return 1; }
    ptx_instance_t *ud = (ptx_instance_t*)lua_newuserdata(L, sizeof(ptx_instance_t));
    ud->class_desc = (void*)cls;
    ud->instance = inst_ptr;
    ud->needs_destroy = 1;
    ud->origin_method = NULL;
    luaL_getmetatable(L, "ptx.instance");
    lua_setmetatable(L, -2);
    return 1;
}

/* metamethod dispatch: allow obj.field / obj.field = v and obj:MethodName(...) */
static int instance_method_trampoline(lua_State *Linner) {
    const char *mname = lua_tostring(Linner, lua_upvalueindex(1));
    /* stack: self, arg1, arg2, ...; need: self, mname, arg1, arg2,... */
    lua_pushstring(Linner, mname);      /* push method name */
    lua_insert(Linner, 2);              /* insert before first arg */
    return instance_call(Linner);
}

static int instance_index(lua_State *L) {
    ptx_instance_t *inst = (ptx_instance_t*)luaL_checkudata(L, 1, "ptx.instance");
    const char *key = luaL_checkstring(L, 2);
    /* First check per-instance cache (uservalue table) */
    ensure_instance_cache(L, 1); /* leaves cache table on stack */
    lua_pushstring(L, key);
    lua_rawget(L, -2); /* cache[key] */
    if (!lua_isnil(L, -1)) {
        /* cached value might be a closure (method) or lightuserdata (field descriptor)
         * If lightuserdata, call instance_get with it to obtain current field value. */
        if (lua_type(L, -1) == LUA_TLIGHTUSERDATA) {
            void *fld = lua_touserdata(L, -1);
            lua_pop(L, 1); /* pop cached descriptor */
            /* leave userdata at 1, then push field pointer and call instance_get */
            lua_settop(L, 1);
            lua_pushlightuserdata(L, fld);
            return instance_get(L);
        }
        /* otherwise return cached closure/function/value */
        return 1;
    }
    lua_pop(L, 1); /* remove nil */

    /* try field first */
    const void *field = ptx_class_find_field(inst->class_desc, key);
    if (field) {
        /* cache the field descriptor as lightuserdata */
        lua_pushstring(L, key);
        lua_pushlightuserdata(L, (void*)field);
        lua_rawset(L, -3);
        /* pop cache table, then call instance_get with field ptr */
        lua_pop(L, 1);
        lua_settop(L, 1);
        lua_pushlightuserdata(L, (void*)field);
        return instance_get(L);
    }

    /* if method exists, return a closure that calls instance_call and cache it */
    const void *method = ptx_class_find_method(inst->class_desc, key);
    if (method) {
        lua_pushstring(L, key);
        lua_pushstring(L, key); lua_pushcclosure(L, instance_method_trampoline, 1);
        /* store closure in cache */
        lua_rawset(L, -3);
        /* closure already stored; now retrieve it to return
         * (rawset popped key+closure, cache still on stack) */
        lua_pushstring(L, key); lua_rawget(L, -2);
        return 1;
    }
    /* fallback: look into original method table (methods like get/set/call) */
    luaL_getmetatable(L, "ptx.instance");
    lua_getfield(L, -1, key);
    if (!lua_isnil(L, -1)) {
        /* return that function */
        return 1;
    }
    lua_pop(L, 2);
    return luaL_error(L, "'%s' is neither field nor method", key);
}

static int instance_newindex(lua_State *L) {
    ptx_instance_t *inst = (ptx_instance_t*)luaL_checkudata(L, 1, "ptx.instance");
    const char *key = luaL_checkstring(L, 2);
    /* check cache for field descriptor */
    ensure_instance_cache(L, 1); /* leaves cache table on stack */
    lua_pushstring(L, key); lua_rawget(L, -2);
    if (lua_type(L, -1) == LUA_TLIGHTUSERDATA) {
        void *fld = lua_touserdata(L, -1);
        lua_pop(L, 2); /* pop descriptor and cache */
        /* call instance_set with lightuserdata field pointer */
        lua_settop(L, 3); /* ensure stack has self, key, value */
        lua_pushlightuserdata(L, fld);
        lua_replace(L, 2); /* replace key with field ptr */
        return instance_set(L);
    }
    lua_pop(L, 1); /* pop nil */
    lua_pop(L, 1); /* pop cache */
    /* Not cached: find field and cache it */
    const void *field = ptx_class_find_field(inst->class_desc, key);
    if (!field) return luaL_error(L, "cannot assign '%s' (no such field)", key);
    /* store into cache */
    ensure_instance_cache(L, 1);
    lua_pushstring(L, key); lua_pushlightuserdata(L, (void*)field); lua_rawset(L, -3);
    lua_pop(L, 1); /* pop cache */
    /* convert to call to set: replace key with lightuserdata then call instance_set */
    lua_settop(L, 3);
    lua_pushlightuserdata(L, (void*)field);
    lua_replace(L, 2);
    return instance_set(L);
}

static const luaL_Reg instance_methods[] = {
    {"get", instance_get},
    {"set", instance_set},
    {"call", instance_call},
    {"call_sig", NULL}, /* placeholder replaced at open */
    {NULL, NULL}
};

static const luaL_Reg ptx_funcs[] = {
    {"list_classes", l_ptx_list_classes},
    {"new", l_ptx_new},
    {"new_sig", NULL}, /* placeholder will set later */
    {"class_info", NULL},
    {"call_static", NULL},
    {"call_static_sig", NULL},
    {NULL, NULL}
};

/* Forward declarations for added functions */
static int l_ptx_class_info(lua_State *L);
static int l_ptx_new_sig(lua_State *L);
static int l_ptx_call_static(lua_State *L);
static int instance_call_sig(lua_State *L);
static int l_ptx_call_static_sig(lua_State *L);

/* instance:call_sig(method_name, signature, ...) */
static int instance_call_sig(lua_State *L) {
    ptx_instance_t *inst = (ptx_instance_t*)luaL_checkudata(L, 1, "ptx.instance");
    const char *mname = luaL_checkstring(L, 2);
    const char *sig = luaL_checkstring(L, 3);
    int provided = lua_gettop(L) - 3; /* exclude self, name, sig */
    /* find method by matching name+signature */
    size_t mcount = ptx_class_method_count(inst->class_desc);
    void *method = NULL;
    for (size_t i=0;i<mcount;++i) {
        const ptx_method_desc *m = ptx_class_method_at((const ptx_class_desc*)inst->class_desc, i);
        if (!m) continue;
        const char *nm = ptx_method_name(m);
        if (!nm || strcmp(nm, mname)!=0) continue;
        const char *msig = ptx_method_signature(m);
        if (msig && strcmp(msig, sig)==0) { method = (void*)m; break; }
    }
    if (!method) return luaL_error(L, "method '%s' with signature %s not found", mname, sig);
    size_t argcount = ptx_method_arg_count(method);
    if ((int)argcount != provided) return luaL_error(L, "signature arg count mismatch (%d expected, got %d)", (int)argcount, provided);
    enum { MAX_ARGS = 32 }; if (argcount > MAX_ARGS) return luaL_error(L, "too many args (%d)", (int)argcount);
    typedef union { int32_t i32; uint32_t u32; int64_t i64; uint64_t u64; double d; float f; int16_t i16; uint16_t u16; int8_t i8; uint8_t u8; bool b; } scalar_t;
    scalar_t storage[MAX_ARGS]; void *argv[MAX_ARGS];
    for (size_t i=0;i<argcount;++i) {
        const void *atype = ptx_method_arg_type(method, i);
        const char *tname = atype ? ptx_type_info_name(atype) : NULL;
        int lua_index = 4 + (int)i; /* after self,name,sig */
        if (!tname) return luaL_error(L, "unknown arg type index %d", (int)i);
        if (strcmp(tname, "h") == 0) { storage[i].u8 = (uint8_t)luaL_checkinteger(L, lua_index); argv[i] = &storage[i].u8; }
        else if (strstr(tname, "uint64") || strstr(tname, "unsigned long long")) { storage[i].u64 = (uint64_t)luaL_checkinteger(L, lua_index); argv[i] = &storage[i].u64; }
        else if (strstr(tname, "int64") || strstr(tname, "long long")) { storage[i].i64 = (int64_t)luaL_checkinteger(L, lua_index); argv[i] = &storage[i].i64; }
        else if (strstr(tname, "double") || strstr(tname, "float")) { storage[i].d = (double)luaL_checknumber(L, lua_index); argv[i] = &storage[i].d; }
        else if (strstr(tname, "uint32") || strstr(tname, "unsigned")) { storage[i].u32 = (uint32_t)luaL_checkinteger(L, lua_index); argv[i] = &storage[i].u32; }
        else if (strstr(tname, "int32") || strstr(tname, "int")) { storage[i].i32 = (int32_t)luaL_checkinteger(L, lua_index); argv[i] = &storage[i].i32; }
        else if (strstr(tname, "uint16_t")) { storage[i].u16 = (uint16_t)luaL_checkinteger(L, lua_index); argv[i] = &storage[i].u16; }
        else if (strstr(tname, "int16_t")) { storage[i].i16 = (int16_t)luaL_checkinteger(L, lua_index); argv[i] = &storage[i].i16; }
        else if (strstr(tname, "uint8_t")) { storage[i].u8 = (uint8_t)luaL_checkinteger(L, lua_index); argv[i] = &storage[i].u8; }
        else if (strstr(tname, "int8_t") || strstr(tname, "char")) { storage[i].i8 = (int8_t)luaL_checkinteger(L, lua_index); argv[i] = &storage[i].i8; }
        else if (strstr(tname, "bool")) { storage[i].b = lua_toboolean(L, lua_index)?1:0; argv[i] = &storage[i].b; }
        else return luaL_error(L, "unsupported arg type: %s", tname);
    }
    void *ret = ptx_method_invoke(method, inst->instance, (argcount? (void**)argv: NULL));
    if (!ret) { lua_pushnil(L); return 1; }
    const void *rtype = ptx_method_return_type(method);
    if (!rtype) { lua_pushnil(L); return 1; }
    const char *tname = ptx_type_info_name(rtype);
    if (!tname) { lua_pushnil(L); return 1; }
    if (strstr(tname, "int64") || strstr(tname, "uint64") || strstr(tname, "long long")) { int64_t v = *(int64_t*)ret; ptx_method_destroy_return(method, ret); lua_pushinteger(L, (lua_Integer)v); return 1; }
    if (strstr(tname, "int") || strstr(tname, "uint")) { int32_t v = *(int32_t*)ret; ptx_method_destroy_return(method, ret); lua_pushinteger(L, v); return 1; }
    if (strstr(tname, "float") || strstr(tname, "double")) { double v = *(double*)ret; ptx_method_destroy_return(method, ret); lua_pushnumber(L, v); return 1; }
    const char *pretty = ptx_type_info_pretty_name(rtype);
    if (pretty) {
        const void *retcls = ptx_registry_find_class(pretty);
        if (retcls) {
            ptx_instance_t *ud = (ptx_instance_t*)lua_newuserdata(L, sizeof(ptx_instance_t));
            ud->class_desc = (void*)retcls; ud->instance = ret; ud->needs_destroy = 2; ud->origin_method = method;
            luaL_getmetatable(L, "ptx.instance"); lua_setmetatable(L, -2); return 1;
        }
    }
    ptx_method_destroy_return(method, ret); lua_pushnil(L); return 1;
}

/* ptx.call_static_sig(class_name, method_name, signature, ...) */
static int l_ptx_call_static_sig(lua_State *L) {
    const char *cname = luaL_checkstring(L, 1);
    const char *mname = luaL_checkstring(L, 2);
    const char *sig = luaL_checkstring(L, 3);
    int provided = lua_gettop(L) - 3;
    const void *cls = ptx_registry_find_class(cname);
    if (!cls) { lua_pushnil(L); return 1; }
    size_t mcount = ptx_class_method_count((void*)cls);
    void *method = NULL;
    for (size_t i=0;i<mcount;++i) {
        const ptx_method_desc *m = ptx_class_method_at((const ptx_class_desc*)cls, i);
        if (!m) continue;
        if (!ptx_method_is_static(m)) continue;
        const char *nm = ptx_method_name(m);
        if (!nm || strcmp(nm,mname)!=0) continue;
        const char *msig = ptx_method_signature(m);
        if (msig && strcmp(msig, sig)==0) { method = (void*)m; break; }
    }
    if (!method) { lua_pushnil(L); return 1; }
    size_t argcount = ptx_method_arg_count(method);
    if ((int)argcount != provided) { lua_pushnil(L); return 1; }
    enum { MAX_ARGS = 32 }; if (argcount > MAX_ARGS) { lua_pushnil(L); return 1; }
    typedef union { int32_t i32; uint32_t u32; int64_t i64; uint64_t u64; double d; float f; int16_t i16; uint16_t u16; int8_t i8; uint8_t u8; bool b; } scalar_t;
    scalar_t storage[MAX_ARGS]; void *argv[MAX_ARGS];
    for (size_t i=0;i<argcount;++i) {
        const void *atype = ptx_method_arg_type(method, i);
        const char *tname = atype ? ptx_type_info_name(atype) : NULL;
        int lua_index = 4 + (int)i; /* after cname,mname,sig */
        if (!tname) { lua_pushnil(L); return 1; }
        if (strcmp(tname, "h") == 0) { storage[i].u8 = (uint8_t)luaL_checkinteger(L, lua_index); argv[i] = &storage[i].u8; }
        else if (strstr(tname, "uint64") || strstr(tname, "unsigned long long")) { storage[i].u64 = (uint64_t)luaL_checkinteger(L, lua_index); argv[i] = &storage[i].u64; }
        else if (strstr(tname, "int64") || strstr(tname, "long long")) { storage[i].i64 = (int64_t)luaL_checkinteger(L, lua_index); argv[i] = &storage[i].i64; }
        else if (strstr(tname, "double") || strstr(tname, "float")) { storage[i].d = (double)luaL_checknumber(L, lua_index); argv[i] = &storage[i].d; }
        else if (strstr(tname, "uint32") || strstr(tname, "unsigned")) { storage[i].u32 = (uint32_t)luaL_checkinteger(L, lua_index); argv[i] = &storage[i].u32; }
        else if (strstr(tname, "int32") || strstr(tname, "int")) { storage[i].i32 = (int32_t)luaL_checkinteger(L, lua_index); argv[i] = &storage[i].i32; }
        else if (strstr(tname, "uint16_t")) { storage[i].u16 = (uint16_t)luaL_checkinteger(L, lua_index); argv[i] = &storage[i].u16; }
        else if (strstr(tname, "int16_t")) { storage[i].i16 = (int16_t)luaL_checkinteger(L, lua_index); argv[i] = &storage[i].i16; }
        else if (strstr(tname, "uint8_t")) { storage[i].u8 = (uint8_t)luaL_checkinteger(L, lua_index); argv[i] = &storage[i].u8; }
        else if (strstr(tname, "int8_t") || strstr(tname, "char")) { storage[i].i8 = (int8_t)luaL_checkinteger(L, lua_index); argv[i] = &storage[i].i8; }
        else if (strstr(tname, "bool")) { storage[i].b = lua_toboolean(L, lua_index)?1:0; argv[i] = &storage[i].b; }
        else { lua_pushnil(L); return 1; }
    }
    void *ret = ptx_method_invoke(method, NULL, (argcount? (void**)argv: NULL));
    if (!ret) { lua_pushnil(L); return 1; }
    const void *rtype = ptx_method_return_type(method);
    if (!rtype) { lua_pushnil(L); return 1; }
    const char *tname = ptx_type_info_name(rtype);
    if (!tname) { lua_pushnil(L); return 1; }
    if (strstr(tname, "int64") || strstr(tname, "uint64") || strstr(tname, "long long")) { int64_t v = *(int64_t*)ret; ptx_method_destroy_return(method, ret); lua_pushinteger(L, (lua_Integer)v); return 1; }
    if (strstr(tname, "int") || strstr(tname, "uint")) { int32_t v = *(int32_t*)ret; ptx_method_destroy_return(method, ret); lua_pushinteger(L, v); return 1; }
    if (strstr(tname, "float") || strstr(tname, "double")) { double v = *(double*)ret; ptx_method_destroy_return(method, ret); lua_pushnumber(L, v); return 1; }
    const char *pretty = ptx_type_info_pretty_name(rtype);
    if (pretty) {
        const void *retcls = ptx_registry_find_class(pretty);
        if (retcls) {
            ptx_instance_t *ud = (ptx_instance_t*)lua_newuserdata(L, sizeof(ptx_instance_t));
            ud->class_desc = (void*)retcls; ud->instance = ret; ud->needs_destroy = 2; ud->origin_method = method;
            luaL_getmetatable(L, "ptx.instance"); lua_setmetatable(L, -2); return 1; }
    }
    ptx_method_destroy_return(method, ret); lua_pushnil(L); return 1;
}

/* Helper: build class_info table */
static int l_ptx_class_info(lua_State *L) {
    const char *cname = luaL_checkstring(L, 1);
    const void *cls = ptx_registry_find_class(cname);
    if (!cls) { lua_pushnil(L); return 1; }
    lua_newtable(L); /* result */
    /* fields */
    lua_newtable(L);
    size_t fcount = ptx_class_field_count((void*)cls);
    for (size_t i=0;i<fcount;++i) {
        const ptx_field_decl *f = ptx_class_field_at((const ptx_class_desc*)cls, i);
        if (!f) continue;
        const char *fname = ptx_field_name(f);
        lua_newtable(L); /* field entry */
        lua_pushstring(L, fname ? fname : ""); lua_setfield(L, -2, "name");
        lua_pushinteger(L, (lua_Integer)ptx_field_size(f)); lua_setfield(L, -2, "size");
        lua_pushnumber(L, ptx_field_min_value(f)); lua_setfield(L, -2, "min");
        lua_pushnumber(L, ptx_field_max_value(f)); lua_setfield(L, -2, "max");
        const char *fd = ptx_field_description(f); if (fd) { lua_pushstring(L, fd); lua_setfield(L, -2, "desc"); }
    const void *ftype = ptx_field_type(f);
        const char *tname = ftype ? ptx_type_info_pretty_name(ftype) : NULL;
        if (tname) { lua_pushstring(L, tname); lua_setfield(L, -2, "type"); }
        lua_rawseti(L, -2, (int)(i+1));
    }
    lua_setfield(L, -2, "fields");
    /* methods */
    lua_newtable(L);
    size_t mcount = ptx_class_method_count((void*)cls);
    for (size_t i=0;i<mcount;++i) {
        const ptx_method_desc *m = ptx_class_method_at((const ptx_class_desc*)cls, i);
        if (!m) continue;
        const char *mname = ptx_method_name(m);
        lua_newtable(L);
        lua_pushstring(L, mname?mname:""); lua_setfield(L, -2, "name");
        const char *sig = ptx_method_signature(m); if (sig) { lua_pushstring(L, sig); lua_setfield(L, -2, "signature"); }
        lua_pushboolean(L, ptx_method_is_static(m)); lua_setfield(L, -2, "static");
        const char *mdoc = ptx_method_doc(m); if (mdoc) { lua_pushstring(L, mdoc); lua_setfield(L, -2, "doc"); }
        size_t argc = ptx_method_arg_count(m); lua_pushinteger(L, (lua_Integer)argc); lua_setfield(L, -2, "argc");
        lua_rawseti(L, -2, (int)(i+1));
    }
    lua_setfield(L, -2, "methods");
    /* ctors */
    lua_newtable(L);
    size_t ccount = ptx_class_constructor_count((void*)cls);
    for (size_t i=0;i<ccount;++i) {
        const ptx_constructor_desc *c = ptx_class_constructor_at((const ptx_class_desc*)cls, i);
        if (!c) continue;
        lua_newtable(L);
        const char *sig = ptx_constructor_signature(c); if (sig) { lua_pushstring(L, sig); lua_setfield(L, -2, "signature"); }
        size_t argc = ptx_constructor_arg_count(c); lua_pushinteger(L, (lua_Integer)argc); lua_setfield(L, -2, "argc");
        lua_rawseti(L, -2, (int)(i+1));
    }
    lua_setfield(L, -2, "constructors");
    return 1;
}

/* ptx.new_sig(class_name, signature, ...) */
static int l_ptx_new_sig(lua_State *L) {
    const char *cname = luaL_checkstring(L, 1);
    const char *sig = luaL_checkstring(L, 2);
    int provided = lua_gettop(L) - 2;
    const ptx_class_desc *cls = ptx_registry_find_class(cname);
    if (!cls) { lua_pushnil(L); return 1; }
    size_t ccount = ptx_class_constructor_count(cls);
    const ptx_constructor_desc *ctor = NULL;
    for (size_t i=0;i<ccount;++i) {
        const ptx_constructor_desc *c = ptx_class_constructor_at(cls, i);
        const char *csig = ptx_constructor_signature(c);
        if (csig && strcmp(csig, sig) == 0) { ctor = c; break; }
    }
    if (!ctor) { lua_pushnil(L); return 1; }
    size_t argc = ptx_constructor_arg_count(ctor);
    if ((int)argc != provided) { lua_pushnil(L); return 1; }
    enum { MAX_ARGS = 32 }; if (argc > MAX_ARGS) { lua_pushnil(L); return 1; }
    typedef union { int32_t i32; uint32_t u32; int64_t i64; uint64_t u64; double d; float f; int16_t i16; uint16_t u16; int8_t i8; uint8_t u8; bool b; } scalar_t;
    scalar_t storage[MAX_ARGS]; void *argv[MAX_ARGS];
    for (size_t i=0;i<argc;++i) {
        const void *atype = ptx_constructor_arg_type(ctor, i);
        const char *tname = atype ? ptx_type_info_name(atype) : NULL;
        int lua_index = 3 + (int)i;
        if (!tname) { lua_pushnil(L); return 1; }
        if (strcmp(tname, "h") == 0) { storage[i].u8 = (uint8_t)luaL_checkinteger(L, lua_index); argv[i] = &storage[i].u8; }
        else if (strstr(tname, "uint64") || strstr(tname, "unsigned long long")) { storage[i].u64 = (uint64_t)luaL_checkinteger(L, lua_index); argv[i] = &storage[i].u64; }
        else if (strstr(tname, "int64") || strstr(tname, "long long")) { storage[i].i64 = (int64_t)luaL_checkinteger(L, lua_index); argv[i] = &storage[i].i64; }
        else if (strstr(tname, "double") || strstr(tname, "float")) { storage[i].d = (double)luaL_checknumber(L, lua_index); argv[i] = &storage[i].d; }
        else if (strstr(tname, "uint32") || strstr(tname, "unsigned")) { storage[i].u32 = (uint32_t)luaL_checkinteger(L, lua_index); argv[i] = &storage[i].u32; }
        else if (strstr(tname, "int32") || strstr(tname, "int")) { storage[i].i32 = (int32_t)luaL_checkinteger(L, lua_index); argv[i] = &storage[i].i32; }
        else if (strstr(tname, "uint16_t")) { storage[i].u16 = (uint16_t)luaL_checkinteger(L, lua_index); argv[i] = &storage[i].u16; }
        else if (strstr(tname, "int16_t")) { storage[i].i16 = (int16_t)luaL_checkinteger(L, lua_index); argv[i] = &storage[i].i16; }
        else if (strstr(tname, "uint8_t")) { storage[i].u8 = (uint8_t)luaL_checkinteger(L, lua_index); argv[i] = &storage[i].u8; }
        else if (strstr(tname, "int8_t") || strstr(tname, "char")) { storage[i].i8 = (int8_t)luaL_checkinteger(L, lua_index); argv[i] = &storage[i].i8; }
        else if (strstr(tname, "bool")) { storage[i].b = lua_toboolean(L, lua_index)?1:0; argv[i] = &storage[i].b; }
        else { lua_pushnil(L); return 1; }
    }
    void *inst_ptr = ptx_constructor_invoke(ctor, argc ? (void**)argv : NULL);
    if (!inst_ptr) { lua_pushnil(L); return 1; }
    ptx_instance_t *ud = (ptx_instance_t*)lua_newuserdata(L, sizeof(ptx_instance_t));
    ud->class_desc = (void*)cls; ud->instance = inst_ptr; ud->needs_destroy = 1; ud->origin_method = NULL;
    luaL_getmetatable(L, "ptx.instance"); lua_setmetatable(L, -2);
    return 1;
}

/* ptx.call_static(class_name, method_name, ...) */
static int l_ptx_call_static(lua_State *L) {
    const char *cname = luaL_checkstring(L, 1);
    const char *mname = luaL_checkstring(L, 2);
    int provided = lua_gettop(L) - 2;
    const void *cls = ptx_registry_find_class(cname);
    if (!cls) { lua_pushnil(L); return 1; }
    ptx_class_cache *cc = get_or_build_class_cache(cls); build_method_cache(cc);
    const ptx_cached_method *mcached = NULL; const void *method = NULL;
    for (size_t i=0;i<cc->method_count;++i) {
        const ptx_cached_method *cm = &cc->methods[i];
        if (!cm->is_static) continue;
        const char *nm = ptx_method_name(cm->decl);
        if (nm && strcmp(nm, mname)==0) { mcached = cm; method = cm->decl; break; }
    }
    if (!method) { lua_pushnil(L); return 1; }
    size_t argcount = mcached->argc;
    if ((int)argcount != provided) { lua_pushnil(L); return 1; }
    enum { MAX_ARGS = 32 }; if (argcount > MAX_ARGS) { lua_pushnil(L); return 1; }
    typedef union { int8_t i8; uint8_t u8; int16_t i16; uint16_t u16; int32_t i32; uint32_t u32; int64_t i64; uint64_t u64; double d; bool b; float f; } scalar_t;
    scalar_t storage[MAX_ARGS]; void *argv[MAX_ARGS];
    for (size_t i=0;i<argcount;++i) {
        int lua_index = 3 + (int)i;
        if (box_arg_from_lua(L, lua_index, mcached->arg_tags[i], &storage[i]) != 0) { lua_pushnil(L); return 1; }
        switch(mcached->arg_tags[i]) {
            case PTX_TAG_I8: argv[i] = &storage[i].i8; break; case PTX_TAG_U8: argv[i] = &storage[i].u8; break;
            case PTX_TAG_I16: argv[i] = &storage[i].i16; break; case PTX_TAG_U16: argv[i] = &storage[i].u16; break;
            case PTX_TAG_I32: argv[i] = &storage[i].i32; break; case PTX_TAG_U32: argv[i] = &storage[i].u32; break;
            case PTX_TAG_I64: argv[i] = &storage[i].i64; break; case PTX_TAG_U64: argv[i] = &storage[i].u64; break;
            case PTX_TAG_F32: argv[i] = &storage[i].d; break; case PTX_TAG_F64: argv[i] = &storage[i].d; break;
            case PTX_TAG_BOOL: argv[i] = &storage[i].b; break;
            default: lua_pushnil(L); return 1;
        }
    }
    void *ret = ptx_method_invoke(method, NULL, (argcount? (void**)argv: NULL));
    if (!ret) { lua_pushnil(L); return 1; }
    if (mcached->ret_tag != PTX_TAG_UNKNOWN && mcached->ret_tag != PTX_TAG_CLASS) {
        return push_return_for_tag(L, mcached->ret_tag, ret, method);
    }
    const void *rtype = ptx_method_return_type(method);
    const char *pretty = rtype ? ptx_type_info_pretty_name(rtype) : NULL;
    if (pretty) {
        const void *retcls = ptx_registry_find_class(pretty);
        if (retcls) {
            ptx_instance_t *ud = (ptx_instance_t*)lua_newuserdata(L, sizeof(ptx_instance_t));
            ud->class_desc = (void*)retcls; ud->instance = ret; ud->needs_destroy = 2; ud->origin_method = (void*)method;
            luaL_getmetatable(L, "ptx.instance"); lua_setmetatable(L, -2); return 1; }
    }
    ptx_method_destroy_return(method, ret); lua_pushnil(L); return 1;
}

int luaopen_ptx(lua_State *L) {
    /* create instance metatable */
    luaL_newmetatable(L, "ptx.instance");
    lua_pushcfunction(L, instance_index); lua_setfield(L, -2, "__index");
    lua_pushcfunction(L, instance_newindex); lua_setfield(L, -2, "__newindex");
    lua_pushcfunction(L, instance_gc);
    lua_setfield(L, -2, "__gc");
    luaL_setfuncs(L, instance_methods, 0);
    /* patch in call_sig now that function exists */
    lua_pushcfunction(L, instance_call_sig); lua_setfield(L, -2, "call_sig");

    lua_newtable(L);
    /* install base funcs then patch placeholders */
    luaL_setfuncs(L, ptx_funcs, 0);
    lua_pushcfunction(L, l_ptx_class_info); lua_setfield(L, -2, "class_info");
    lua_pushcfunction(L, l_ptx_new_sig); lua_setfield(L, -2, "new_sig");
    lua_pushcfunction(L, l_ptx_call_static); lua_setfield(L, -2, "call_static");
    lua_pushcfunction(L, l_ptx_call_static_sig); lua_setfield(L, -2, "call_static_sig");
    return 1;
}
