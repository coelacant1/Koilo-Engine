#!/usr/bin/env python3
"""GeneratePTXAll.py
---------------------
Generate an umbrella header that `#include`s every public *.hpp under a root.

Default behavior (no --output provided): write into `<root>/ptxall.hpp`.
Preferred modern usage: supply an explicit --output path inside the CMake
build tree (e.g. `${CMAKE_BINARY_DIR}/generated/ptx/ptxall.hpp`) so the file
is not tracked in the source repo.

Examples:
    python scripts/GeneratePTXAll.py --root engine/include/ptx \
                 --output build/generated/ptx/ptxall.hpp
    python scripts/GeneratePTXAll.py --root engine/include/ptx --output build/gen/ptx/ptxall.hpp
"""

from pathlib import Path
import os
import sys
import argparse

try:  # When invoked via PlatformIO extra_scripts
    Import("env")  # type: ignore  # noqa: F821
    DEFAULT_REPO_ROOT = Path(env["PROJECT_DIR"]).resolve()  # type: ignore  # noqa: F821
except Exception:  # Stand-alone
    DEFAULT_REPO_ROOT = Path(__file__).resolve().parents[1]

DEFAULT_HEADER_ROOT = Path("engine") / "include" / "ptx"

def parse_args() -> argparse.Namespace:
    ap = argparse.ArgumentParser(description="Generate umbrella ptxall.hpp")
    ap.add_argument("--root", default=str(DEFAULT_HEADER_ROOT), help="Header root to scan (default: engine/include/ptx)")
    ap.add_argument("--output", default=None, help="Output file path (default: <root>/ptxall.hpp)")
    return ap.parse_args()

def gather_headers(include_root: Path, output_name: str) -> list[str]:
    headers: list[str] = []
    for path, _, files in os.walk(include_root):
        for f in files:
            if not f.endswith(".hpp"):
                continue
            if f == output_name:
                continue
            rel = Path(path, f).relative_to(include_root)
            headers.append(str(rel).replace(os.sep, "/"))
    return sorted(headers)

def generate(include_root: Path, output_file: Path) -> int:
    hdrs = gather_headers(include_root, output_file.name)
    output_file.parent.mkdir(parents=True, exist_ok=True)

    with output_file.open("w", encoding="utf-8") as fp:
        fp.write(
            "/**\n"
            " * @file ptxall.hpp (auto-generated)\n"
            " * @brief Umbrella header - DO NOT EDIT BY HAND.\n"
            " */\n"
            "#pragma once\n\n"
        )
        for h in hdrs:
            fp.write(f'#include "{h}"\n')

    print(f"Regenerated {output_file} with {len(hdrs)} headers (root={include_root}, count={len(hdrs)})")
    return 0

def main_cli() -> int:
    ns = parse_args()
    repo_root = DEFAULT_REPO_ROOT
    root_path = Path(ns.root)
    if not root_path.is_absolute():
        root_path = (repo_root / root_path).resolve()
    if not root_path.exists():
        print(f"[ptxall] ERROR: root path does not exist: {root_path}", file=sys.stderr)
        return 2
    if ns.output:
        out_path = Path(ns.output)
        if not out_path.is_absolute():
            out_path = (repo_root / out_path).resolve()
    else:
        out_path = root_path / "ptxall.hpp"
    return generate(root_path, out_path)

if __name__ == "__main__":
    sys.exit(main_cli())
else:  # Imported (e.g. via PlatformIO) - retain legacy behavior
    try:
        root_candidate = DEFAULT_REPO_ROOT / DEFAULT_HEADER_ROOT
        if not root_candidate.exists():
            root_candidate = DEFAULT_REPO_ROOT / "lib" / "ptx"
        legacy_out = root_candidate / "ptxall.hpp"
        generate(root_candidate, legacy_out)
    except Exception:
        pass
