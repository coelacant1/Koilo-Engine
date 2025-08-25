#!/usr/bin/env python3
"""
GeneratePTXAll.py
---------------------
Create (or update) `lib/ptx/ptxall.hpp`, an umbrella header that
#include-s every public *.hpp in the project.

 * Runs automatically when listed in PlatformIO `extra_scripts`.
 * Can also be run stand-alone:  `python .scripts/GeneratePTXAll.py`
"""

from pathlib import Path
import os
import sys

# 1. Locate the repo root - two cases
try:
    # We are inside PlatformIO => PROJECT_DIR exists
    Import("env")                         # pylint: disable=undefined-variable
    REPO_ROOT = Path(env["PROJECT_DIR"]).resolve()
except NameError:
    # Stand-alone run: script lives in <root>/.scripts/
    REPO_ROOT = Path(__file__).resolve().parents[1]

INCLUDE_ROOT = REPO_ROOT / "lib" / "ptx"
OUTPUT_FILE  = INCLUDE_ROOT / "ptxall.hpp"


# 2. Collect all .hpp headers (skip the umbrella itself)
def gather_headers() -> list[str]:
    headers: list[str] = []
    for path, _, files in os.walk(INCLUDE_ROOT):
        for f in files:
            if f.endswith(".hpp") and f != OUTPUT_FILE.name:
                rel = Path(path, f).relative_to(INCLUDE_ROOT)
                headers.append(str(rel).replace(os.sep, "/"))
    return sorted(headers)


# 3. Write the umbrella header 
def generate() -> int:
    hdrs = gather_headers()
    OUTPUT_FILE.parent.mkdir(parents=True, exist_ok=True)

    with OUTPUT_FILE.open("w", encoding="utf-8") as fp:
        fp.write(
            "/**\n"
            " * @file ptxall.hpp (auto-generated)\n"
            " * @brief Umbrella header - DO NOT EDIT BY HAND.\n"
            " */\n"
            "#pragma once\n\n"
        )
        for h in hdrs:
            fp.write(f'#include "{h}"\n')

    print(f"Regenerated {OUTPUT_FILE} with {len(hdrs)} headers")
    return 0


# 4. Execute immediately (both PIO + CLI)
if __name__ == "__main__":
    sys.exit(generate())
else:
    generate()
