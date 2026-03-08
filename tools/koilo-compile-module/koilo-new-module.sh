#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# koilo-new-module.sh  Generate a scaffolded KoiloEngine module from template
#
# Usage:
#   koilo-new-module.sh <module_name> [--dir <output_dir>]
#
# Creates:
#   <module_name>.c  Module source with all lifecycle hooks
#   Makefile         Build targets for desktop (.so)

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ENGINE_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

NAME=""
OUTPUT_DIR="."

usage() {
    echo "Usage: koilo-new-module.sh <module_name> [--dir <output_dir>]"
    echo ""
    echo "Creates a scaffolded KoiloEngine module with source and Makefile."
    exit 0
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --dir) OUTPUT_DIR="$2"; shift 2 ;;
        -h|--help) usage ;;
        -*) echo "Error: Unknown option $1" >&2; exit 1 ;;
        *) NAME="$1"; shift ;;
    esac
done

if [[ -z "$NAME" ]]; then
    echo "Error: Module name required" >&2
    echo "Usage: koilo-new-module.sh <module_name>" >&2
    exit 1
fi

mkdir -p "$OUTPUT_DIR"

# Generate module source
cat > "$OUTPUT_DIR/${NAME}.c" << TEMPLATE
/**
 * ${NAME}  KoiloEngine dynamic module
 *
 * Build:
 *   make desktop    # builds ${NAME}.so
 *
 * Load in engine:
 *   engine.GetModuleLoader().LoadFromLibrary("./${NAME}.so");
 *
 * Access from script:
 *   ${NAME}.YourMethod()
 */
#include <koilo/modules/koilo_module_sdk.h>

/* ---- Module state ---- */

static KoiloEngineServices* svc = NULL;
static void* eng = NULL;

/* ---- Your subsystem ---- */

typedef struct {
    float value;
} ${NAME^}System;

static ${NAME^}System sys_instance;

/* Example method: ${NAME}.GetValue() */
static void* method_get_value(void* self, void** args) {
    (void)args;
    ${NAME^}System* s = (${NAME^}System*)self;
    static float ret;
    ret = s->value;
    return &ret;
}

/* Example method: ${NAME}.SetValue(n) */
static void* method_set_value(void* self, void** args) {
    ${NAME^}System* s = (${NAME^}System*)self;
    s->value = *(float*)args[0];
    return NULL;
}

/* ---- Module lifecycle ---- */

KOILO_MODULE_HEADER("${NAME}", "1.0.0", KOILO_PHASE_SYSTEM)

KOILO_MODULE_INIT {
    svc = svc_;
    eng = engine_;

    sys_instance.value = 0.0f;

    /* Register as a script global */
    svc->register_global(eng, "${NAME}", "${NAME^}System", &sys_instance);

    if (KOILO_HAS_API(svc, log_info) && svc->log_info) {
        svc->log_info("${NAME} module initialized");
    }

    return 1; /* success */
}

KOILO_MODULE_UPDATE {
    (void)dt;
    /* Called every frame  add per-frame logic here */
}

KOILO_MODULE_RENDER {
    (void)buffer; (void)width; (void)height;
    /* Called after rasterization  modify pixel buffer here if needed */
}

KOILO_MODULE_SHUTDOWN {
    svc = NULL;
    eng = NULL;
}
TEMPLATE

# Generate Makefile
cat > "$OUTPUT_DIR/Makefile" << MAKEFILE
# Makefile for ${NAME} KoiloEngine module
ENGINE_ROOT ?= ${ENGINE_ROOT}
SDK_INCLUDE = \$(ENGINE_ROOT)/engine/include
COMPILE_TOOL = \$(ENGINE_ROOT)/tools/koilo-compile-module/koilo-compile-module.sh

.PHONY: desktop clean

desktop: ${NAME}.so

${NAME}.so: ${NAME}.c
	\$(COMPILE_TOOL) ${NAME}.c -o ${NAME}.so --target desktop

clean:
	rm -f ${NAME}.so ${NAME}.bin ${NAME}.o
MAKEFILE

echo "Created module scaffold:"
echo "  $OUTPUT_DIR/${NAME}.c"
echo "  $OUTPUT_DIR/Makefile"
echo ""
echo "Build with: cd $OUTPUT_DIR && make desktop"
