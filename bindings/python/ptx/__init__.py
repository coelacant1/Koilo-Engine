"""Python interface for the PTX reflection runtime.

The :mod:`ptx.reflection` module exposes helpers that load the C ABI defined in
``lib/ptx_c_api/reflect.h`` and provide high-level Python objects for classes,
methods, and fields that live in the PTX registry.

Example
-------
>>> from ptx import reflection
>>> ref = reflection.load_reflection()
>>> rgb = ref.find_class("RGBColor")
>>> ctor = rgb.constructors[0]
>>> color = ctor(*[0, 128, 255])
>>> [f.name for f in rgb.fields]
['R', 'G', 'B']
"""

from .reflection import (
    load_reflection,
    PTXReflection,
    PTXClass,
    PTXField,
    PTXMethod,
    PTXConstructor,
    PTXObject,
)

__all__ = [
    "load_reflection",
    "PTXReflection",
    "PTXClass",
    "PTXField",
    "PTXMethod",
    "PTXConstructor",
    "PTXObject",
]
