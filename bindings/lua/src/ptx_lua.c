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

#include <stdlib.h>
#include <string.h>

/* Correct relative path: current file is lib/ptx_lua/src/ptx_lua.c,
 * reflect.h lives at lib/ptx_c_api/reflect.h -> go up two levels to lib/, then ptx_c_api. */
// Adjusted include after directory restructure: ptx_c_api now at repo root
// Prefer angle bracket with explicit path root; ensure CMake adds ${CMAKE_SOURCE_DIR} if needed.
#include "reflect.h"  // updated path after restructure; reflect.h resides in bindings/c_api and that dir is in include dirs

typedef struct {
    void *class_desc;    /* ptx::ClassDesc* as void* */
    void *instance;      /* pointer to underlying instance */
    int   needs_destroy; /* 1 if owned via class destroy, 2 if method return needing method_destroy_return */
    void *origin_method; /* method descriptor for destroy_return when needs_destroy==2 */
} ptx_instance_t;

/* Helper: push a Lua string safe */
static void push_cstr(lua_State *L, const char *s) {
    if (!s) lua_pushnil(L);
    else lua_pushstring(L, s);
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
    const char *fname = luaL_checkstring(L, 2);
    const void *field = ptx_class_find_field(inst->class_desc, (const char*)fname);
    if (!field) { return luaL_error(L, "field '%s' not found", fname); }
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
    const char *fname = luaL_checkstring(L, 2);
    const void *field = ptx_class_find_field(inst->class_desc, (const char*)fname);
    if (!field) { return luaL_error(L, "field '%s' not found", fname); }
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
    const void *method = ptx_class_find_method(inst->class_desc, (const char*)mname);
    if (!method) { return luaL_error(L, "method '%s' not found", mname); }
    size_t argcount = ptx_method_arg_count((void*)method);
    int provided = lua_gettop(L) - 2; /* exclude self + method_name */
    if ((int)argcount != provided) { return luaL_error(L, "method '%s' expects %d args, got %d", mname, (int)argcount, provided); }

    /* Box arguments */
    enum { MAX_ARGS = 16 };
    if (argcount > MAX_ARGS) return luaL_error(L, "too many args (%d)", (int)argcount);
    /* storage for primitive values */
    typedef union { int32_t i32; uint32_t u32; int64_t i64; uint64_t u64; double d; float f; int16_t i16; uint16_t u16; int8_t i8; uint8_t u8; bool b; } scalar_t;
    scalar_t storage[MAX_ARGS];
    void *argv[MAX_ARGS];
    for (size_t i = 0; i < argcount; ++i) {
    const void *atype = ptx_method_arg_type((void*)method, i);
        const char *tname = ptx_type_info_name(atype);
        if (!tname) return luaL_error(L, "unknown arg type index %d", (int)i);
        int lua_index = 3 + (int)i; /* self, name, then args */
        if (strcmp(tname, "h") == 0) { /* unsigned char via typeid mangling/abbrev */
            storage[i].u8 = (uint8_t)luaL_checkinteger(L, lua_index);
            argv[i] = &storage[i].u8;
        } else if (strstr(tname, "double") || strstr(tname, "float")) {
            storage[i].d = (double)luaL_checknumber(L, lua_index);
            argv[i] = &storage[i].d;
        } else if (strstr(tname, "uint64") || strstr(tname, "unsigned long long")) {
            storage[i].u64 = (uint64_t)luaL_checkinteger(L, lua_index);
            argv[i] = &storage[i].u64;
        } else if (strstr(tname, "int64") || strstr(tname, "long long")) {
            storage[i].i64 = (int64_t)luaL_checkinteger(L, lua_index);
            argv[i] = &storage[i].i64;
        } else if (strstr(tname, "uint8_t")) {
            storage[i].u8 = (uint8_t)luaL_checkinteger(L, lua_index);
            argv[i] = &storage[i].u8;
        } else if (strstr(tname, "int8_t")) {
            storage[i].i8 = (int8_t)luaL_checkinteger(L, lua_index);
            argv[i] = &storage[i].i8;
        } else if (strstr(tname, "uint16_t")) {
            storage[i].u16 = (uint16_t)luaL_checkinteger(L, lua_index);
            argv[i] = &storage[i].u16;
        } else if (strstr(tname, "int16_t")) {
            storage[i].i16 = (int16_t)luaL_checkinteger(L, lua_index);
            argv[i] = &storage[i].i16;
        } else if (strstr(tname, "uint32") || strstr(tname, "unsigned")) {
            storage[i].u32 = (uint32_t)luaL_checkinteger(L, lua_index);
            argv[i] = &storage[i].u32;
        } else if (strstr(tname, "int32") || strstr(tname, "int")) {
            storage[i].i32 = (int32_t)luaL_checkinteger(L, lua_index);
            argv[i] = &storage[i].i32;
        } else if (strstr(tname, "bool")) {
            storage[i].b = lua_toboolean(L, lua_index) ? true : false;
            argv[i] = &storage[i].b;
        } else if (strstr(tname, "char")) {
            storage[i].i8 = (int8_t)luaL_checkinteger(L, lua_index);
            argv[i] = &storage[i].i8;
        } else {
            return luaL_error(L, "unsupported arg type: %s", tname);
        }
    }
    void *ret = ptx_method_invoke((void*)method, inst->instance, (argcount ? (void**)argv : NULL));
    if (!ret) { lua_pushnil(L); return 1; }
    /* try to interpret return as primitive */
    const void *rtype = ptx_method_return_type((void*)method);
    if (!rtype) { lua_pushnil(L); return 1; }
    const char *tname = ptx_type_info_name(rtype);
    if (!tname) { lua_pushnil(L); return 1; }
    if (strstr(tname, "int64") || strstr(tname, "uint64") || strstr(tname, "long long")) {
        /* treat as 64-bit */
        int64_t v = *(int64_t*)ret;
        ptx_method_destroy_return(method, ret);
        lua_pushinteger(L, (lua_Integer)v);
        return 1;
    }
    if (strstr(tname, "int") || strstr(tname, "uint")) {
        /* assume returned pointer to integer sized result */
        int32_t v = *(int32_t*)ret;
        ptx_method_destroy_return(method, ret);
        lua_pushinteger(L, v);
        return 1;
    }
    if (strstr(tname, "float") || strstr(tname, "double")) {
        double v = *(double*)ret;
        ptx_method_destroy_return(method, ret);
        lua_pushnumber(L, v);
        return 1;
    }
    /* fallback: return userdata wrapper if class-like */
    /* try to find class by name */
    const char *pretty = ptx_type_info_pretty_name(rtype);
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
    size_t ctor_count = ptx_class_constructor_count((void*)cls);
    const ptx_constructor_desc *ctor = NULL;
    for (size_t i = 0; i < ctor_count; ++i) {
    const void *c = ptx_class_constructor_at((void*)cls, i);
    if ((int)ptx_constructor_arg_count((const void*)c) == provided) { ctor = (void*)c; break; }
    }
    if (!ctor) { return luaL_error(L, "no constructor on '%s' accepts %d args", cname, provided); }
    size_t argc = ptx_constructor_arg_count((void*)ctor);
    enum { MAX_CTOR_ARGS = 16 };
    if (argc > MAX_CTOR_ARGS) { lua_pushnil(L); return 1; }
    typedef union { int32_t i32; uint32_t u32; int64_t i64; uint64_t u64; double d; float f; int16_t i16; uint16_t u16; int8_t i8; uint8_t u8; bool b; } scalar_t;
    scalar_t storage[MAX_CTOR_ARGS];
    void *argv[MAX_CTOR_ARGS];
    for (size_t i = 0; i < argc; ++i) {
    const void *atype = ptx_constructor_arg_type((void*)ctor, i);
        const char *tname = ptx_type_info_name(atype);
        int lua_index = 2 + (int)i; /* cname at 1 */
        if (!tname) { lua_pushnil(L); return 1; }
        if (strcmp(tname, "h") == 0) { storage[i].u8 = (uint8_t)luaL_checkinteger(L, lua_index); argv[i] = &storage[i].u8; }
    else if (strstr(tname, "double") || strstr(tname, "float")) { storage[i].d = (double)luaL_checknumber(L, lua_index); argv[i] = &storage[i].d; }
        else if (strstr(tname, "uint64") || strstr(tname, "unsigned long long")) { storage[i].u64 = (uint64_t)luaL_checkinteger(L, lua_index); argv[i] = &storage[i].u64; }
        else if (strstr(tname, "int64") || strstr(tname, "long long")) { storage[i].i64 = (int64_t)luaL_checkinteger(L, lua_index); argv[i] = &storage[i].i64; }
    else if (strstr(tname, "uint8_t")) { storage[i].u8 = (uint8_t)luaL_checkinteger(L, lua_index); argv[i] = &storage[i].u8; }
    else if (strstr(tname, "int8_t")) { storage[i].i8 = (int8_t)luaL_checkinteger(L, lua_index); argv[i] = &storage[i].i8; }
        else if (strstr(tname, "uint16_t")) { storage[i].u16 = (uint16_t)luaL_checkinteger(L, lua_index); argv[i] = &storage[i].u16; }
        else if (strstr(tname, "int16_t")) { storage[i].i16 = (int16_t)luaL_checkinteger(L, lua_index); argv[i] = &storage[i].i16; }
        else if (strstr(tname, "uint32") || strstr(tname, "unsigned")) { storage[i].u32 = (uint32_t)luaL_checkinteger(L, lua_index); argv[i] = &storage[i].u32; }
        else if (strstr(tname, "int32") || strstr(tname, "int")) { storage[i].i32 = (int32_t)luaL_checkinteger(L, lua_index); argv[i] = &storage[i].i32; }
        else if (strstr(tname, "bool")) { storage[i].b = lua_toboolean(L, lua_index) ? true : false; argv[i] = &storage[i].b; }
        else if (strstr(tname, "char")) { storage[i].i8 = (int8_t)luaL_checkinteger(L, lua_index); argv[i] = &storage[i].i8; }
        else { lua_pushnil(L); return 1; }
    }
    void *inst_ptr = ptx_constructor_invoke((void*)ctor, argc ? (void**)argv : NULL);
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
    /* try field first */
    const void *field = ptx_class_find_field(inst->class_desc, key);
    if (field) {
        /* push value via get logic (reuse stack by calling helper) */
        lua_settop(L, 1); /* keep userdata at 1 */
        lua_pushstring(L, key);
        return instance_get(L);
    }
    /* if method exists, return a closure that calls instance_call */
    const void *method = ptx_class_find_method(inst->class_desc, key);
    if (method) { lua_pushstring(L, key); lua_pushcclosure(L, instance_method_trampoline, 1); return 1; }
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
    const void *field = ptx_class_find_field(inst->class_desc, key);
    if (!field) return luaL_error(L, "cannot assign '%s' (no such field)", key);
    /* convert to call to set: stack currently self, key, value */
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
    const void *cls = ptx_registry_find_class(cname);
    if (!cls) { lua_pushnil(L); return 1; }
    size_t ccount = ptx_class_constructor_count((void*)cls);
    void *ctor = NULL;
    for (size_t i=0;i<ccount;++i) {
    const ptx_constructor_desc *c = ptx_class_constructor_at((const ptx_class_desc*)cls, i);
    const char *csig = ptx_constructor_signature(c);
    if (csig && strcmp(csig, sig) == 0) { ctor = c; break; }
    }
    if (!ctor) { lua_pushnil(L); return 1; }
    size_t argc = ptx_constructor_arg_count((const ptx_constructor_desc*)ctor);
    if ((int)argc != provided) { lua_pushnil(L); return 1; }
    enum { MAX_ARGS = 32 }; if (argc > MAX_ARGS) { lua_pushnil(L); return 1; }
    typedef union { int32_t i32; uint32_t u32; int64_t i64; uint64_t u64; double d; float f; int16_t i16; uint16_t u16; int8_t i8; uint8_t u8; bool b; } scalar_t;
    scalar_t storage[MAX_ARGS]; void *argv[MAX_ARGS];
    for (size_t i=0;i<argc;++i) {
    const void *atype = ptx_constructor_arg_type((const ptx_constructor_desc*)ctor, i);
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
    void *inst_ptr = ptx_constructor_invoke((const ptx_constructor_desc*)ctor, argc? (void**)argv: NULL);
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
    const ptx_method_desc *method = ptx_class_find_method((const ptx_class_desc*)cls, mname);
    if (!method) { lua_pushnil(L); return 1; }
    if (!ptx_method_is_static(method)) { lua_pushnil(L); return 1; }
    size_t argcount = ptx_method_arg_count(method);
    if ((int)argcount != provided) { lua_pushnil(L); return 1; }
    enum { MAX_ARGS = 32 }; if (argcount > MAX_ARGS) { lua_pushnil(L); return 1; }
    typedef union { int32_t i32; uint32_t u32; int64_t i64; uint64_t u64; double d; float f; int16_t i16; uint16_t u16; int8_t i8; uint8_t u8; bool b; } scalar_t;
    scalar_t storage[MAX_ARGS]; void *argv[MAX_ARGS];
    for (size_t i=0;i<argcount;++i) {
    const void *atype = ptx_method_arg_type(method, i);
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
    void *ret = ptx_method_invoke(method, NULL, (argcount? (void**)argv: NULL));
    if (!ret) { lua_pushnil(L); return 1; }
    const void *rtype = ptx_method_return_type(method);
    if (!rtype) { lua_pushnil(L); return 1; }
    const char *tname = ptx_type_info_name(rtype);
    if (!tname) { lua_pushnil(L); return 1; }
    if (strstr(tname, "int64") || strstr(tname, "uint64") || strstr(tname, "long long")) {
        int64_t v = *(int64_t*)ret; ptx_method_destroy_return(method, ret); lua_pushinteger(L, (lua_Integer)v); return 1; }
    if (strstr(tname, "int") || strstr(tname, "uint")) { int32_t v = *(int32_t*)ret; ptx_method_destroy_return(method, ret); lua_pushinteger(L, v); return 1; }
    if (strstr(tname, "float") || strstr(tname, "double")) { double v = *(double*)ret; ptx_method_destroy_return(method, ret); lua_pushnumber(L, v); return 1; }
    const char *pretty = ptx_type_info_pretty_name(rtype);
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
