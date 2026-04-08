#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""
Convert legacy camera .h files to .klcam JSON format.

Parses files like assets/cameras/ProtoDR.h that contain arrays of
Vector2D(x, y) positions (in millimeters) and outputs .klcam JSON
with the same data in a runtime-loadable format.

Usage:
    python3 scripts/convert_camera_h.py assets/cameras/ProtoDR.h
    python3 scripts/convert_camera_h.py assets/cameras/*.h --outdir assets/cameras/
"""

import argparse
import json
import os
import re
import sys


def parse_camera_header(path: str) -> tuple:
    """Parse a camera .h file and return (name, positions).

    Returns:
        (name, positions) where name is the array variable name and
        positions is a list of [x, y] pairs (floats, in mm).
    """
    with open(path, "r") as f:
        text = f.read()

    # Match: <optional qualifiers> Vector2D Name[N] = { ... };
    array_match = re.search(
        r"(?:DMAMEM\s+|const\s+)*Vector2D\s+(\w+)\s*\[\s*(\d+)\s*\]\s*=\s*\{",
        text,
    )
    if not array_match:
        raise ValueError(f"No Vector2D array found in {path}")

    name = array_match.group(1)
    declared_count = int(array_match.group(2))

    # Extract all Vector2D(x, y) entries.
    positions = []
    for m in re.finditer(r"Vector2D\s*\(\s*([0-9.eE+-]+)f?\s*,\s*([0-9.eE+-]+)f?\s*\)", text):
        x = float(m.group(1))
        y = float(m.group(2))
        positions.append([round(x, 4), round(y, 4)])

    if len(positions) != declared_count:
        print(
            f"Warning: {path}: declared {declared_count} entries but parsed {len(positions)}",
            file=sys.stderr,
        )

    return name, positions


def build_klcam(name: str, positions: list) -> dict:
    """Build a .klcam JSON structure."""
    return {
        "name": name,
        "units": "mm",
        "count": len(positions),
        "positions": positions,
    }


def main():
    parser = argparse.ArgumentParser(
        description="Convert camera .h files to .klcam JSON"
    )
    parser.add_argument("files", nargs="+", help="Input .h files")
    parser.add_argument(
        "--outdir",
        default=None,
        help="Output directory (default: same as input file)",
    )
    parser.add_argument(
        "--indent",
        type=int,
        default=2,
        help="JSON indent (default: 2, use 0 for compact)",
    )
    args = parser.parse_args()

    for filepath in args.files:
        try:
            name, positions = parse_camera_header(filepath)
        except ValueError as e:
            print(f"Error: {e}", file=sys.stderr)
            continue

        klcam = build_klcam(name, positions)

        # Determine output path.
        basename = os.path.splitext(os.path.basename(filepath))[0] + ".klcam"
        if args.outdir:
            os.makedirs(args.outdir, exist_ok=True)
            outpath = os.path.join(args.outdir, basename)
        else:
            outpath = os.path.join(os.path.dirname(filepath), basename)

        indent = args.indent if args.indent > 0 else None
        with open(outpath, "w") as f:
            json.dump(klcam, f, indent=indent)
            f.write("\n")

        print(f"{filepath} -> {outpath}  ({klcam['count']} LEDs, '{name}')")


if __name__ == "__main__":
    main()
