#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# koilo-compile-module.sh  Compile a KoiloEngine dynamic module (.so/.dll)
#
# Usage:
#   koilo-compile-module.sh <source.c> -o <output> [--target desktop|teensy41] [options]
#
# Examples:
#   koilo-compile-module.sh my_plugin.c -o my_plugin.so
#   koilo-compile-module.sh my_plugin.c -o my_plugin.so --target desktop
#   koilo-compile-module.sh my_plugin.c -o my_plugin.bin --target teensy41
#
# The tool automatically locates the KoiloEngine SDK header and builds
# a shared library suitable for loading with ModuleLoader::LoadFromLibrary().

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ENGINE_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
SDK_INCLUDE="$ENGINE_ROOT/engine"

# Defaults
TARGET="desktop"
OUTPUT=""
SOURCE=""
EXTRA_FLAGS=""
VERBOSE=0

usage() {
    cat <<EOF
Usage: koilo-compile-module.sh <source.c> -o <output> [options]

Options:
  -o <file>           Output file path (required)
  --target <target>   Build target: desktop (default), teensy41
  --cflags <flags>    Additional compiler flags
  --verbose           Show compiler commands
  -h, --help          Show this help

Targets:
  desktop   Build as .so (Linux) or .dll (Windows) using host compiler
  teensy41  Build as .bin using arm-none-eabi-gcc (requires ARM toolchain)

The SDK header (koilo_module_sdk.h) is included automatically from:
  $SDK_INCLUDE/koilo/kernel/
EOF
    exit 0
}

# Parse arguments
while [[ $# -gt 0 ]]; do
    case "$1" in
        -o)
            OUTPUT="$2"; shift 2 ;;
        --target)
            TARGET="$2"; shift 2 ;;
        --cflags)
            EXTRA_FLAGS="$2"; shift 2 ;;
        --verbose)
            VERBOSE=1; shift ;;
        -h|--help)
            usage ;;
        -*)
            echo "Error: Unknown option $1" >&2; exit 1 ;;
        *)
            if [[ -z "$SOURCE" ]]; then
                SOURCE="$1"
            else
                echo "Error: Multiple source files not supported. Got: $SOURCE and $1" >&2
                exit 1
            fi
            shift ;;
    esac
done

if [[ -z "$SOURCE" ]]; then
    echo "Error: No source file specified" >&2
    echo "Usage: koilo-compile-module.sh <source.c> -o <output>" >&2
    exit 1
fi

if [[ -z "$OUTPUT" ]]; then
    echo "Error: No output file specified (-o)" >&2
    exit 1
fi

if [[ ! -f "$SOURCE" ]]; then
    echo "Error: Source file not found: $SOURCE" >&2
    exit 1
fi

if [[ ! -f "$SDK_INCLUDE/koilo/kernel/koilo_module_sdk.h" ]]; then
    echo "Error: SDK header not found at $SDK_INCLUDE/koilo/kernel/koilo_module_sdk.h" >&2
    echo "Make sure you're running from a KoiloEngine project directory" >&2
    exit 1
fi

run_cmd() {
    if [[ $VERBOSE -eq 1 ]]; then
        echo "+ $*" >&2
    fi
    "$@"
}

case "$TARGET" in
    desktop)
        # Detect compiler
        if command -v g++ &>/dev/null; then
            CXX="g++"
        elif command -v clang++ &>/dev/null; then
            CXX="clang++"
        else
            echo "Error: No C++ compiler found (need g++ or clang++)" >&2
            exit 1
        fi

        # Detect if source is C or C++
        case "$SOURCE" in
            *.cpp|*.cxx|*.cc)
                LANG_FLAGS="-std=c++17"
                COMPILER="$CXX"
                ;;
            *.c)
                LANG_FLAGS="-std=c11"
                if command -v gcc &>/dev/null; then
                    COMPILER="gcc"
                elif command -v clang &>/dev/null; then
                    COMPILER="clang"
                else
                    COMPILER="$CXX"
                    LANG_FLAGS="-std=c++17"
                fi
                ;;
            *)
                COMPILER="$CXX"
                LANG_FLAGS="-std=c++17"
                ;;
        esac

        run_cmd "$COMPILER" $LANG_FLAGS -shared -fPIC -fvisibility=hidden \
            -I "$SDK_INCLUDE" \
            -O2 \
            $EXTRA_FLAGS \
            "$SOURCE" \
            -o "$OUTPUT"

        echo "Built: $OUTPUT (desktop shared library)"
        ;;

    teensy41)
        ARM_GCC="arm-none-eabi-gcc"
        if ! command -v "$ARM_GCC" &>/dev/null; then
            echo "Error: $ARM_GCC not found in PATH" >&2
            echo "Install the ARM toolchain: apt install gcc-arm-none-eabi" >&2
            exit 1
        fi

        # Compile to position-independent object
        OBJFILE="${OUTPUT%.bin}.o"
        run_cmd "$ARM_GCC" -mcpu=cortex-m7 -mthumb -mfloat-abi=hard -mfpu=fpv5-d16 \
            -fPIC -fno-exceptions -fno-rtti \
            -I "$SDK_INCLUDE" \
            -Os \
            $EXTRA_FLAGS \
            -c "$SOURCE" \
            -o "$OBJFILE"

        # Link as relocatable binary
        run_cmd "$ARM_GCC" -mcpu=cortex-m7 -mthumb -mfloat-abi=hard -mfpu=fpv5-d16 \
            -nostdlib -shared -fPIC \
            "$OBJFILE" \
            -o "$OUTPUT"

        rm -f "$OBJFILE"
        echo "Built: $OUTPUT (Teensy 4.1 module binary)"
        ;;

    *)
        echo "Error: Unknown target '$TARGET'. Use 'desktop' or 'teensy41'" >&2
        exit 1
        ;;
esac
