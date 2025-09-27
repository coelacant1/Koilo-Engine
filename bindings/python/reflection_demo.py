"""Small demonstration script for the PTX reflection Python wrapper.

Usage
-----
$ python -m examples.reflection_demo --lib /path/to/libptx_c_api.so

If ``--lib`` is omitted the loader will try ``$PTX_C_API_LIB`` and a few common
filenames. The script will enumerate classes, construct an ``RGBColor`` via the
PTX reflection registry, mutate its fields, and invoke instance/static methods
using plain Python values.
"""

from __future__ import annotations

import argparse
import contextlib
from ptx import reflection


@contextlib.contextmanager
def managed(obj):
    try:
        yield obj
    finally:
        if hasattr(obj, "release"):
            obj.release()


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--lib",
        help="Path to the PTX reflection shared library (defaults to auto-discovery)",
    )
    args = parser.parse_args()

    ref = reflection.load_reflection(args.lib)
    print("Loaded reflection registry from", ref._clib.path)

    classes = list(ref.iter_classes())
    print(f"Discovered {len(classes)} classes:")
    for cls in classes:
        print(" -", cls.name)

    rgb = ref.find_class("RGBColor")
    if not rgb:
        print("RGBColor is not registered - build the PTXEngine reflection sample.")
        return

    ctor3 = None
    for ctor in rgb.constructors:
        if ctor.arg_count == 3:
            ctor3 = ctor
            break
    if not ctor3:
        print("RGBColor has no 3-argument constructor; cannot continue demo.")
        return

    with managed(ctor3(10, 20, 30)) as color:
        print("Constructed RGBColor at", color.pointer)
        fields = {field.name: field for field in rgb.fields}
        print("Initial components:")
        for name in ("R", "G", "B"):
            fld = fields.get(name)
            if fld:
                print(f"  {name} =", fld.read(color))

        print("Setting G to 200 via reflection...")
        fields["G"].write(color, 200)
        print("  G =", fields["G"].read(color))

        add_method = rgb.find_method("Add")
        if add_method:
            with managed(add_method(color, 25)) as brighter:
                print("Add(25) ->", brighter.pointer)
                for name in ("R", "G", "B"):
                    fld = fields.get(name)
                    if fld:
                        print(f"    {name} =", fld.read(brighter))

        lerp = rgb.find_method("InterpolateColors")
        if lerp:
            with managed(ctor3(0, 0, 0)) as black, managed(ctor3(255, 128, 64)) as warm:
                with managed(lerp(None, black, warm, 0.5)) as mid:
                    print("InterpolateColors(..., 0.5) ->", mid.pointer)
                    for name in ("R", "G", "B"):
                        fld = fields.get(name)
                        if fld:
                            print(f"    {name} =", fld.read(mid))


if __name__ == "__main__":
    main()
