#!/usr/bin/env python3
"""One-shot migration helper to new repository layout.

This script copies (does NOT delete by default) the existing source tree:
  lib/ptx               -> engine/include/ptx (headers *.hpp, *.tpp)
  lib/ptx (sources)     -> engine/src (mirroring relative subpaths *.cpp)
  lib/ptx_c_api         -> bindings/c_api
  lib/ptx_lua           -> bindings/lua
  test/*.cpp|*.hpp      -> tests/engine/

It is idempotent: existing destination files are left untouched unless
--overwrite is specified.

After running:
  1. Update CMakeLists.txt to point generation root at engine/include/ptx.
  2. Remove obsolete lib/ptx directories once builds pass.
  3. Delete lib/ptx/ptxall.hpp from source (generated in build instead).

Usage:
  python scripts/migrate_layout.py --dry-run
  python scripts/migrate_layout.py --apply
  python scripts/migrate_layout.py --apply --overwrite
"""

from __future__ import annotations
import argparse
import shutil
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]

OLD_CORE = ROOT / "lib" / "ptx"
OLD_CAPI = ROOT / "lib" / "ptx_c_api"
OLD_LUA  = ROOT / "lib" / "ptx_lua"
OLD_TEST = ROOT / "test"

NEW_ENGINE_INCLUDE = ROOT / "engine" / "include" / "ptx"
NEW_ENGINE_SRC     = ROOT / "engine" / "src"
NEW_CAPI           = ROOT / "bindings" / "c_api"
NEW_LUA            = ROOT / "bindings" / "lua"
NEW_TESTS_ENGINE   = ROOT / "tests" / "engine"

HEADER_EXTS = {".hpp", ".h", ".tpp"}
SOURCE_EXTS = {".cpp", ".cxx"}

def rel_to(old_root: Path, p: Path) -> Path:
    return p.relative_to(old_root)

def collect_files(base: Path):
    headers, sources, others = [], [], []
    for p in base.rglob('*'):
        if p.is_dir():
            continue
        ext = p.suffix.lower()
        if ext in HEADER_EXTS:
            headers.append(p)
        elif ext in SOURCE_EXTS:
            sources.append(p)
        else:
            others.append(p)
    return headers, sources, others

def copy_file(src: Path, dst: Path, overwrite: bool, dry_run: bool, actions: list[str]):
    if dst.exists() and not overwrite:
        actions.append(f"SKIP exists {dst}")
        return
    if not dry_run:
        dst.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(src, dst)
    actions.append(f"COPY {src} -> {dst}")

def migrate(apply: bool, overwrite: bool) -> int:
    actions: list[str] = []
    dry = not apply

    if not OLD_CORE.exists():
        print("[migrate] Old core path not found; nothing to do.")
        return 0

    # Core headers & sources
    core_headers, core_sources, _ = collect_files(OLD_CORE)
    for h in core_headers:
        # Skip legacy generated umbrella if present
        if h.name == 'ptxall.hpp':
            continue
        dst = NEW_ENGINE_INCLUDE / rel_to(OLD_CORE, h)
        copy_file(h, dst, overwrite, dry, actions)
    for s in core_sources:
        dst = NEW_ENGINE_SRC / rel_to(OLD_CORE, s)
        copy_file(s, dst, overwrite, dry, actions)

    # C API
    if OLD_CAPI.exists():
        for p in OLD_CAPI.rglob('*'):
            if p.is_file():
                dst = NEW_CAPI / rel_to(OLD_CAPI, p)
                copy_file(p, dst, overwrite, dry, actions)

    # Lua binding
    if OLD_LUA.exists():
        for p in OLD_LUA.rglob('*'):
            if p.is_file():
                dst = NEW_LUA / rel_to(OLD_LUA, p)
                copy_file(p, dst, overwrite, dry, actions)

    # Tests
    if OLD_TEST.exists():
        for p in OLD_TEST.glob('*.cpp'):
            dst = NEW_TESTS_ENGINE / p.name
            copy_file(p, dst, overwrite, dry, actions)
        for p in OLD_TEST.glob('*.hpp'):
            dst = NEW_TESTS_ENGINE / p.name
            copy_file(p, dst, overwrite, dry, actions)
        # utils folder
        utils_dir = OLD_TEST / 'utils'
        if utils_dir.exists():
            for p in utils_dir.rglob('*'):
                if p.is_file():
                    dst = NEW_TESTS_ENGINE / 'utils' / rel_to(utils_dir, p)
                    copy_file(p, dst, overwrite, dry, actions)

    print("[migrate] Planned actions (dry-run=" + str(dry) + "):")
    for a in actions:
        print("  " + a)
    print(f"[migrate] Total operations: {len(actions)}")
    if dry:
        print("(Run again with --apply to perform copy; use --overwrite to replace existing)")
    return 0

def main():
    ap = argparse.ArgumentParser(description="Migrate legacy layout to new engine/bindings structure")
    ap.add_argument('--apply', action='store_true', help='Perform file copies (default is dry-run)')
    ap.add_argument('--overwrite', action='store_true', help='Overwrite existing destination files')
    ns = ap.parse_args()
    return migrate(apply=ns.apply, overwrite=ns.overwrite)

if __name__ == '__main__':
    sys.exit(main())
