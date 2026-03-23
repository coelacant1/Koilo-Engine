#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""
ksl_compiler  Compile KSL shader sources into .kso (ELF) modules + .glsl.

All targets produce .kso files (ELF shared objects) loaded by KSLElfLoader:
  - linux:     g++ -shared -fpic -> .kso (ELF x86_64)
  - windows:   clang++ --target=x86_64-linux-elf -> .kso (ELF x86_64)
  - teensy41:  arm-none-eabi-g++ -shared -fpic -> .kso (ELF ARM32)

Also generates .glsl via ksl_codegen.py for GPU rendering (desktop only).

Usage:
    ksl_compiler shader.ksl.hpp                        # Desktop .kso + .glsl
    ksl_compiler shader.ksl.hpp --target=teensy41      # MCU .kso only
    ksl_compiler shader.ksl.hpp -o out/                # Output to directory
    ksl_compiler engine/shaders/*.ksl.hpp              # Batch compile

Requirements:
    linux:     g++ (or c++)
    windows:   clang++ (produces ELF, not PE)
    teensy41:  arm-none-eabi-g++
    glsl:      Python3 + ksl_codegen.py
"""

import argparse
import os
import re
import subprocess
import sys
import shutil
import tempfile

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
# Engine include path (relative to this script)
ENGINE_INCLUDE = os.path.normpath(os.path.join(SCRIPT_DIR, "..", "..", "engine"))
KSL_CODEGEN = os.path.join(SCRIPT_DIR, "ksl_codegen.py")


def detect_target() -> str:
    """Auto-detect compilation target from the host platform."""
    if sys.platform.startswith("linux"):
        return "linux"
    elif sys.platform == "win32":
        return "windows"
    return "linux"


def find_compiler(target: str) -> str:
    """Find the appropriate C++ compiler for the target."""
    if target == "teensy41":
        compiler = shutil.which("arm-none-eabi-g++")
        if not compiler:
            print("Error: arm-none-eabi-g++ not found.", file=sys.stderr)
            print("Install: sudo apt install gcc-arm-none-eabi", file=sys.stderr)
            sys.exit(1)
        return compiler
    elif target == "windows":
        compiler = shutil.which("clang++")
        if not compiler:
            print("Error: clang++ not found (required for Windows ELF output).", file=sys.stderr)
            sys.exit(1)
        return compiler
    else:  # linux
        for cc in ["g++", "c++"]:
            compiler = shutil.which(cc)
            if compiler:
                return compiler
        print("Error: No C++ compiler found.", file=sys.stderr)
        sys.exit(1)


def extract_shader_class(source_path: str) -> str:
    """Extract the shader class name from a .ksl.hpp file."""
    with open(source_path, "r") as f:
        content = f.read()
    match = re.search(r'struct\s+(\w+)\s*:\s*public\s+ksl::Shader', content)
    if not match:
        print(f"Error: Could not find 'struct XXX : public ksl::Shader' in {source_path}",
              file=sys.stderr)
        sys.exit(1)
    return match.group(1)


def shader_display_name(source_path: str) -> str:
    """Derive a display name from the source filename."""
    base = os.path.basename(source_path)
    # Remove .ksl.hpp extension
    name = re.sub(r'\.ksl\.hpp$', '', base)
    return name


def detect_required_attribs(source_path: str) -> int:
    """Scan shader source to detect which ShadeInput fields it reads."""
    with open(source_path, "r") as f:
        content = f.read()
    flags = 0
    if re.search(r'\bin\.position\b', content):
        flags |= 1   # SHADE_ATTRIB_POS
    if re.search(r'\bin\.uv\b', content):
        flags |= 2   # SHADE_ATTRIB_UV
    if re.search(r'\bin\.viewDir\b', content):
        flags |= 4   # SHADE_ATTRIB_VIEWDIR
    if re.search(r'\bin\.normal\b', content):
        flags |= 8   # SHADE_ATTRIB_NORMAL
    return flags


def detect_has_metadata(source_path: str) -> bool:
    """Check if the shader declares KSL_META_BEGIN."""
    with open(source_path, "r") as f:
        content = f.read()
    return bool(re.search(r'KSL_META_BEGIN', content))


def generate_wrapper(shader_include: str, class_name: str, display_name: str,
                     required_attribs: int = 0xFF, has_metadata: bool = False) -> str:
    """Generate the C ABI wrapper source code."""
    metadata_fn = ""
    if has_metadata:
        metadata_fn = """
    const ksl::MetaEntry* ksl_metadata(int* count) {
        auto m = ShaderType::Meta();
        *count = m.count;
        return m.entries;
    }"""
    return f'''// Auto-generated KSL wrapper - do not edit
#include <cstdlib>
#include <cstring>
#include <new>

#include "koilo/ksl/ksl.hpp"
#include "{shader_include}"

using ShaderType = {class_name};

static ksl::KSLShaderInfo _shader_info;
static bool _info_ready = false;

extern "C" {{
    void* ksl_create() {{
        void* p = malloc(sizeof(ShaderType));
        if (!p) return nullptr;
        return ::new(p) ShaderType();
    }}

    void ksl_destroy(void* p) {{
        if (p) {{
            static_cast<ShaderType*>(p)->~ShaderType();
            free(p);
        }}
    }}

    ksl::vec4 ksl_shade(void* inst, const ksl::ShadeInput* input) {{
        return static_cast<ShaderType*>(inst)->ShaderType::shade(*input);
    }}

    void ksl_set_param(void* inst, const char* name, const void* data, int type, int count) {{
        auto* s = static_cast<ShaderType*>(inst);
        auto params = ShaderType::Params();
        for (int i = 0; i < params.count; i++) {{
            if (strcmp(params.decls[i].name, name) == 0) {{
                size_t sz = 0;
                switch (static_cast<ksl::ParamType>(type)) {{
                    case ksl::ParamType::Float: sz = sizeof(float); break;
                    case ksl::ParamType::Int:   sz = sizeof(int); break;
                    case ksl::ParamType::Bool:  sz = sizeof(bool); break;
                    case ksl::ParamType::Vec2:  sz = sizeof(ksl::vec2); break;
                    case ksl::ParamType::Vec3:  sz = sizeof(ksl::vec3); break;
                    case ksl::ParamType::Vec4:  sz = sizeof(ksl::vec4); break;
                }}
                memcpy(reinterpret_cast<char*>(s) + params.decls[i].offset, data, sz * count);
                return;
            }}
        }}
    }}

    const ksl::KSLShaderInfo* ksl_info() {{
        if (!_info_ready) {{
            memset(&_shader_info, 0, sizeof(_shader_info));
            strncpy(_shader_info.name, "{display_name}", 63);
            _shader_info.version = 1;
            _shader_info.paramCount = ShaderType::Params().count;
            _shader_info.requiredAttribs = {required_attribs};
            _info_ready = true;
        }}
        return &_shader_info;
    }}

    const ksl::ParamDecl* ksl_params(int* count) {{
        auto p = ShaderType::Params();
        *count = p.count;
        return p.decls;
    }}
{metadata_fn}
}}
'''


def compile_kso(source: str, output: str, target: str, compiler: str,
                engine_include: str, extra_flags: list, verbose: bool) -> bool:
    """Compile a .ksl.hpp to a .kso (ELF shared object)."""
    class_name = extract_shader_class(source)
    display_name = shader_display_name(source)
    source_dir = os.path.dirname(os.path.abspath(source))

    if verbose:
        print(f"  Class: {class_name}, Name: {display_name}")

    with tempfile.TemporaryDirectory() as tmpdir:
        wrapper_path = os.path.join(tmpdir, "ksl_wrapper.cpp")
        attribs = detect_required_attribs(source)
        has_meta = detect_has_metadata(source)
        wrapper_src = generate_wrapper(os.path.basename(source), class_name,
                                       display_name, attribs, has_meta)

        with open(wrapper_path, "w") as f:
            f.write(wrapper_src)

        # Build compiler command
        base_flags = ["-shared", "-fpic", "-fno-rtti", "-fno-exceptions",
                      "-fno-stack-protector", "-Wno-invalid-offsetof",
                      "-Wno-template-body"]

        if target == "teensy41":
            cmd = [
                compiler,
                "-mcpu=cortex-m7", "-mthumb", "-mfloat-abi=hard", "-mfpu=fpv5-d16",
                "-Os", "-nostdlib",
            ] + base_flags
        elif target == "windows":
            cmd = [
                compiler,
                "--target=x86_64-linux-elf",
                "-O2",
            ] + base_flags
        else:  # linux
            cmd = [compiler, "-O2"] + base_flags

        cmd += [
            f"-I{engine_include}",
            f"-I{source_dir}",
            "-std=c++17",
            "-o", output,
            wrapper_path,
        ] + extra_flags

        if verbose:
            print(f"  Command: {' '.join(cmd)}")

        result = subprocess.run(cmd, capture_output=True, text=True)

        if result.returncode != 0:
            print(f"  FAILED: {source}", file=sys.stderr)
            if result.stderr:
                print(result.stderr, file=sys.stderr)
            return False

    size = os.path.getsize(output)
    print(f"  OK: {output} ({size} bytes)")
    return True


def generate_glsl(source: str, output: str, verbose: bool) -> bool:
    """Generate .glsl from .ksl.hpp via ksl_codegen.py."""
    if not os.path.isfile(KSL_CODEGEN):
        if verbose:
            print(f"  Skipping GLSL: {KSL_CODEGEN} not found")
        return False

    cmd = [sys.executable, KSL_CODEGEN, source, "-o", output]
    if verbose:
        print(f"  GLSL: {' '.join(cmd)}")

    result = subprocess.run(cmd, capture_output=True, text=True)
    if result.returncode != 0:
        print(f"  GLSL FAILED: {source}", file=sys.stderr)
        if result.stderr:
            print(result.stderr, file=sys.stderr)
        return False

    print(f"  GLSL: {output}")
    return True


def main():
    parser = argparse.ArgumentParser(
        prog="ksl-compiler",
        description="Compile KSL shaders to .kso (ELF) + .glsl.",
    )
    parser.add_argument("sources", nargs="+", metavar="SOURCE",
                        help=".ksl.hpp source file(s)")
    parser.add_argument("-o", "--output", default=None,
                        help="Output file or directory")
    parser.add_argument("--target", default=None,
                        choices=["linux", "windows", "teensy41"],
                        help="Compilation target (default: auto-detect)")
    parser.add_argument("--no-glsl", action="store_true",
                        help="Skip GLSL generation")
    parser.add_argument("--cc", default=None,
                        help="Override C++ compiler path")
    parser.add_argument("--engine-include", default=ENGINE_INCLUDE,
                        help="Engine include directory")
    parser.add_argument("-v", "--verbose", action="store_true")
    parser.add_argument("--cflags", default="",
                        help="Extra compiler flags")

    args = parser.parse_args()

    target = args.target or detect_target()
    compiler = args.cc or find_compiler(target)
    extra_flags = args.cflags.split() if args.cflags else []

    # Determine output directory
    out_dir = None
    if args.output and len(args.sources) > 1:
        out_dir = args.output
        os.makedirs(out_dir, exist_ok=True)
    elif args.output and os.path.isdir(args.output):
        out_dir = args.output

    success = 0
    failed = 0

    for source in args.sources:
        if not os.path.isfile(source):
            print(f"Error: {source} not found", file=sys.stderr)
            failed += 1
            continue

        stem = shader_display_name(source)

        if out_dir:
            kso_path = os.path.join(out_dir, stem + ".kso")
            glsl_path = os.path.join(out_dir, stem + ".glsl")
        elif args.output and len(args.sources) == 1:
            kso_path = args.output
            glsl_path = os.path.splitext(args.output)[0] + ".glsl"
        else:
            base_dir = os.path.dirname(source)
            kso_path = os.path.join(base_dir, stem + ".kso")
            glsl_path = os.path.join(base_dir, stem + ".glsl")

        print(f"[{target}] {source}")

        ok = compile_kso(source, kso_path, target, compiler,
                         args.engine_include, extra_flags, args.verbose)

        if ok and not args.no_glsl and target != "teensy41":
            generate_glsl(source, glsl_path, args.verbose)

        if ok:
            success += 1
        else:
            failed += 1

    if len(args.sources) > 1:
        print(f"\nResults: {success} succeeded, {failed} failed")

    sys.exit(1 if failed > 0 else 0)


if __name__ == "__main__":
    main()
