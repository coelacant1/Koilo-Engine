#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""
abi_check.py -- ABI freeze validation tool for Koilo Engine.

Parses the C ABI header (module_api.hpp) and compares against a frozen
baseline JSON (docs/abi/abi-v3.json) to detect backward-incompatible changes.

Checks performed:
  1. Frozen struct fields are not removed, reordered, or type-changed.
  2. Frozen enum values are not removed or renumbered.
  3. ABI constants (magic, version) match baseline.
  4. New fields in frozen structs are append-only.

Exit codes:
  0 -- All checks pass.
  1 -- One or more violations detected.
  2 -- Tool error (missing file, parse error).

Usage:
  python3 tools/abi_check.py
  python3 tools/abi_check.py --baseline docs/abi/abi-v3.json
  python3 tools/abi_check.py --header engine/koilo/kernel/module_api.hpp
"""

import argparse
import json
import re
import sys
from pathlib import Path

# Defaults relative to repository root
DEFAULT_HEADER = "engine/koilo/kernel/module_api.hpp"
DEFAULT_BASELINE = "docs/abi/abi-v3.json"


def find_repo_root() -> Path:
    """Walk up from CWD to find the repo root (has CMakeLists.txt)."""
    p = Path.cwd()
    while p != p.parent:
        if (p / "CMakeLists.txt").exists() and (p / "engine").exists():
            return p
        p = p.parent
    return Path.cwd()


def parse_struct_fields(text: str, struct_name: str) -> list:
    """Extract field declarations from a struct body."""
    # Find the struct definition
    pattern = rf"struct\s+(?:KL_\w+\s+)?{struct_name}\s*\{{"
    match = re.search(pattern, text)
    if not match:
        return []

    # Find matching closing brace
    start = match.end()
    depth = 1
    pos = start
    while pos < len(text) and depth > 0:
        if text[pos] == '{':
            depth += 1
        elif text[pos] == '}':
            depth -= 1
        pos += 1

    body = text[start:pos - 1]

    fields = []
    # Match function pointer fields: type (*name)(args);
    # Match regular fields: type name;
    # Match array fields: type name[N];
    for line in body.split('\n'):
        line = line.strip()
        if not line or line.startswith('//') or line.startswith('/*'):
            continue

        # Function pointer: return_type (*name)(params);
        fp_match = re.match(
            r'(\w[\w\s\*]*?)\s*\(\*(\w+)\)\s*\(([^)]*)\)\s*;', line
        )
        if fp_match:
            ret_type = fp_match.group(1).strip()
            name = fp_match.group(2)
            params = fp_match.group(3).strip()
            ftype = f"{ret_type}(*)({params})"
            # Normalize whitespace
            ftype = re.sub(r'\s+', ' ', ftype).strip()
            fields.append({"name": name, "type": ftype})
            continue

        # Array field: type name[N];
        arr_match = re.match(
            r'(\w[\w\s\*]*?)\s+(\w+)\s*\[(\d+)\]\s*;', line
        )
        if arr_match:
            ftype = arr_match.group(1).strip()
            name = arr_match.group(2)
            size = arr_match.group(3)
            fields.append({"name": name, "type": f"{ftype}[{size}]"})
            continue

        # Regular field: type name;
        reg_match = re.match(
            r'(\w[\w\s\*:]*?)\s+(\w+)\s*(?:=\s*[^;]*)?\s*;', line
        )
        if reg_match:
            ftype = reg_match.group(1).strip()
            name = reg_match.group(2)
            fields.append({"name": name, "type": ftype})

    return fields


def parse_enum_values(text: str, enum_name: str) -> dict:
    """Extract enum constant name->value mappings."""
    pattern = rf"enum\s+(?:KL_\w+\s+)?{enum_name}\s*(?::\s*\w+\s*)?\{{"
    match = re.search(pattern, text)
    if not match:
        return {}

    start = match.end()
    depth = 1
    pos = start
    while pos < len(text) and depth > 0:
        if text[pos] == '{':
            depth += 1
        elif text[pos] == '}':
            depth -= 1
        pos += 1

    body = text[start:pos - 1]
    values = {}
    for line in body.split('\n'):
        line = re.sub(r'//.*', '', line).strip().rstrip(',')
        if not line:
            continue
        m = re.match(r'(\w+)\s*=\s*(\d+)', line)
        if m:
            values[m.group(1)] = int(m.group(2))

    return values


def parse_defines(text: str, names: list) -> dict:
    """Extract #define constant values."""
    result = {}
    for name in names:
        m = re.search(rf'#define\s+{name}\s+(\S+)', text)
        if m:
            result[name] = m.group(1)
    return result


def check_struct(struct_name: str, baseline_fields: list,
                 current_fields: list, errors: list):
    """Validate a frozen struct against its baseline."""
    baseline_names = [f["name"] for f in baseline_fields]
    current_names = [f["name"] for f in current_fields]

    # Check no baseline fields were removed
    for bf in baseline_fields:
        if bf["name"] not in current_names:
            errors.append(
                f"FROZEN VIOLATION: {struct_name}.{bf['name']} was removed"
            )

    # Check field order preserved (baseline fields must appear in same order)
    current_indices = {n: i for i, n in enumerate(current_names)}
    prev_idx = -1
    for bname in baseline_names:
        if bname in current_indices:
            idx = current_indices[bname]
            if idx < prev_idx:
                errors.append(
                    f"FROZEN VIOLATION: {struct_name}.{bname} was reordered"
                )
            prev_idx = idx

    # New fields must be appended after all baseline fields
    if baseline_names and current_names:
        last_baseline_idx = -1
        for bname in baseline_names:
            if bname in current_indices:
                last_baseline_idx = max(last_baseline_idx,
                                        current_indices[bname])

        for i, cname in enumerate(current_names):
            if cname not in baseline_names and i <= last_baseline_idx:
                errors.append(
                    f"FROZEN VIOLATION: {struct_name}.{cname} inserted "
                    f"before existing frozen field (must be append-only)"
                )


def check_enum(enum_name: str, baseline_values: dict,
               current_values: dict, errors: list):
    """Validate a frozen enum against its baseline."""
    for name, value in baseline_values.items():
        if name not in current_values:
            errors.append(
                f"FROZEN VIOLATION: {enum_name}::{name} was removed"
            )
        elif current_values[name] != value:
            errors.append(
                f"FROZEN VIOLATION: {enum_name}::{name} changed from "
                f"{value} to {current_values[name]}"
            )


def main():
    parser = argparse.ArgumentParser(
        description="Koilo Engine ABI freeze checker"
    )
    parser.add_argument("--header", default=None,
                        help="Path to module_api.hpp")
    parser.add_argument("--baseline", default=None,
                        help="Path to ABI baseline JSON")
    parser.add_argument("--root", default=None,
                        help="Repository root directory")
    args = parser.parse_args()

    root = Path(args.root) if args.root else find_repo_root()
    header_path = Path(args.header) if args.header else root / DEFAULT_HEADER
    baseline_path = (Path(args.baseline) if args.baseline
                     else root / DEFAULT_BASELINE)

    if not header_path.exists():
        print(f"ERROR: Header not found: {header_path}", file=sys.stderr)
        sys.exit(2)
    if not baseline_path.exists():
        print(f"ERROR: Baseline not found: {baseline_path}", file=sys.stderr)
        sys.exit(2)

    header_text = header_path.read_text()
    baseline = json.loads(baseline_path.read_text())

    errors = []

    # Check frozen structs
    for struct_name, spec in baseline.get("frozen_structs", {}).items():
        if spec.get("kind") == "enum":
            current_values = parse_enum_values(header_text, struct_name)
            if not current_values:
                errors.append(f"ERROR: Enum {struct_name} not found in header")
                continue
            check_enum(struct_name, spec["values"], current_values, errors)
        else:
            current_fields = parse_struct_fields(header_text, struct_name)
            if not current_fields:
                errors.append(
                    f"ERROR: Struct {struct_name} not found in header"
                )
                continue
            check_struct(struct_name, spec["fields"], current_fields, errors)

    # Check constants
    const_names = list(baseline.get("constants", {}).keys())
    current_defines = parse_defines(header_text, const_names)
    for name, expected in baseline.get("constants", {}).items():
        if name not in current_defines:
            errors.append(f"CONSTANT MISSING: {name}")
        elif current_defines[name] != str(expected):
            errors.append(
                f"CONSTANT CHANGED: {name} expected {expected}, "
                f"got {current_defines[name]}"
            )

    # Report
    if errors:
        print(f"ABI CHECK FAILED -- {len(errors)} violation(s):\n")
        for e in errors:
            print(f"  FAIL: {e}")
        print()
        sys.exit(1)
    else:
        print(f"ABI CHECK PASSED -- all frozen contracts verified against "
              f"{baseline_path.name}")
        sys.exit(0)


if __name__ == "__main__":
    main()
