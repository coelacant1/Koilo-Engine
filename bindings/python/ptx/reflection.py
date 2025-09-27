"""ctypes-based wrapper around the PTX reflection C API.

This module talks to the C ABI declared in ``lib/ptx_c_api/reflect.h`` and
lifts registry metadata into small Python helper classes that are friendly to
use from Python scripts or REPL sessions.

The wrapper intentionally keeps a low-level feel - it exposes the same
information the C side does - but adds enough convenience to construct objects,
inspect fields/methods, and invoke functions with plain Python scalars.

The shared library must export the symbols defined in ``reflect.h``. By
default :func:`load_reflection` looks for the library via the
``PTX_C_API_LIB`` environment variable, the platform dynamic loader
(``ctypes.util.find_library"), or a few common filenames in the repository.
"""

from __future__ import annotations

import ctypes
from ctypes import util as ctypes_util
import os
from pathlib import Path
from typing import Iterator, List, Optional, Sequence, Tuple, Union, Callable

__all__ = [
    "load_reflection",
    "PTXReflection",
    "PTXClass",
    "PTXField",
    "PTXMethod",
    "PTXConstructor",
    "PTXObject",
]

# ---------------------------------------------------------------------------
# Low level: library loading and type conversion helpers.
# ---------------------------------------------------------------------------

_DEFAULT_LIBRARY_NAMES = (
    # Legacy / previous naming attempts
    "ptx_c_api",
    "libptx_c_api.so",
    "libptx_c_api.dylib",
    "ptx_c_api.dll",
    # Current engine shared reflection lib produced by CMake
    "ptx_reflect",
    "libptx_reflect.so",
    # Fallback generic engine name
    "libptxengine.so",
)

_SIMPLE_TYPE_MAP = {
    "bool": ctypes.c_bool,
    "char": ctypes.c_char,
    "signed char": ctypes.c_byte,
    "unsigned char": ctypes.c_ubyte,
    "std::uint8_t": ctypes.c_ubyte,
    "uint8_t": ctypes.c_ubyte,
    "std::int8_t": ctypes.c_byte,
    "int8_t": ctypes.c_byte,
    "short": ctypes.c_short,
    "signed short": ctypes.c_short,
    "unsigned short": ctypes.c_ushort,
    "std::uint16_t": ctypes.c_ushort,
    "uint16_t": ctypes.c_ushort,
    "std::int16_t": ctypes.c_short,
    "int16_t": ctypes.c_short,
    "int": ctypes.c_int,
    "signed int": ctypes.c_int,
    "unsigned int": ctypes.c_uint,
    "std::uint32_t": ctypes.c_uint,
    "uint32_t": ctypes.c_uint,
    "std::int32_t": ctypes.c_int,
    "int32_t": ctypes.c_int,
    "long": ctypes.c_long,
    "signed long": ctypes.c_long,
    "unsigned long": ctypes.c_ulong,
    "long long": ctypes.c_longlong,
    "signed long long": ctypes.c_longlong,
    "unsigned long long": ctypes.c_ulonglong,
    "std::uint64_t": ctypes.c_ulonglong,
    "uint64_t": ctypes.c_ulonglong,
    "std::int64_t": ctypes.c_longlong,
    "int64_t": ctypes.c_longlong,
    "float": ctypes.c_float,
    "double": ctypes.c_double,
    "long double": ctypes.c_longdouble,
}


class _PTXCLibrary:
    """Thin helper that loads the shared library and prepares ctypes signatures."""

    def __init__(self, path: Optional[Union[str, os.PathLike[str]]] = None):
        resolved = _resolve_library_path(path)
        self.path = str(resolved)
        self.lib = ctypes.CDLL(self.path)
        self._configure()

    # ------------------------------------------------------------------
    # C function wrappers (mostly signature setup so we get type safety)
    # ------------------------------------------------------------------

    def _configure(self) -> None:
        lib = self.lib

        lib.ptx_registry_class_count.restype = ctypes.c_size_t

        lib.ptx_registry_class_at.argtypes = [ctypes.c_size_t]
        lib.ptx_registry_class_at.restype = ctypes.c_void_p

        lib.ptx_registry_find_class.argtypes = [ctypes.c_char_p]
        lib.ptx_registry_find_class.restype = ctypes.c_void_p

        lib.ptx_class_name.argtypes = [ctypes.c_void_p]
        lib.ptx_class_name.restype = ctypes.c_char_p

        lib.ptx_class_field_count.argtypes = [ctypes.c_void_p]
        lib.ptx_class_field_count.restype = ctypes.c_size_t

        lib.ptx_class_field_at.argtypes = [ctypes.c_void_p, ctypes.c_size_t]
        lib.ptx_class_field_at.restype = ctypes.c_void_p

        lib.ptx_class_find_field.argtypes = [ctypes.c_void_p, ctypes.c_char_p]
        lib.ptx_class_find_field.restype = ctypes.c_void_p

        lib.ptx_class_method_count.argtypes = [ctypes.c_void_p]
        lib.ptx_class_method_count.restype = ctypes.c_size_t

        lib.ptx_class_method_at.argtypes = [ctypes.c_void_p, ctypes.c_size_t]
        lib.ptx_class_method_at.restype = ctypes.c_void_p

        lib.ptx_class_find_method.argtypes = [ctypes.c_void_p, ctypes.c_char_p]
        lib.ptx_class_find_method.restype = ctypes.c_void_p

        lib.ptx_class_constructor_count.argtypes = [ctypes.c_void_p]
        lib.ptx_class_constructor_count.restype = ctypes.c_size_t

        lib.ptx_class_constructor_at.argtypes = [ctypes.c_void_p, ctypes.c_size_t]
        lib.ptx_class_constructor_at.restype = ctypes.c_void_p

        lib.ptx_class_destroy_instance.argtypes = [ctypes.c_void_p, ctypes.c_void_p]
        lib.ptx_class_destroy_instance.restype = None

        lib.ptx_field_name.argtypes = [ctypes.c_void_p]
        lib.ptx_field_name.restype = ctypes.c_char_p

        lib.ptx_field_description.argtypes = [ctypes.c_void_p]
        lib.ptx_field_description.restype = ctypes.c_char_p

        lib.ptx_field_size.argtypes = [ctypes.c_void_p]
        lib.ptx_field_size.restype = ctypes.c_size_t

        lib.ptx_field_min_value.argtypes = [ctypes.c_void_p]
        lib.ptx_field_min_value.restype = ctypes.c_double

        lib.ptx_field_max_value.argtypes = [ctypes.c_void_p]
        lib.ptx_field_max_value.restype = ctypes.c_double

        lib.ptx_field_type.argtypes = [ctypes.c_void_p]
        lib.ptx_field_type.restype = ctypes.c_void_p

        lib.ptx_field_get_ptr.argtypes = [ctypes.c_void_p, ctypes.c_void_p]
        lib.ptx_field_get_ptr.restype = ctypes.c_void_p

        lib.ptx_field_get_const_ptr.argtypes = [ctypes.c_void_p, ctypes.c_void_p]
        lib.ptx_field_get_const_ptr.restype = ctypes.c_void_p

        lib.ptx_method_name.argtypes = [ctypes.c_void_p]
        lib.ptx_method_name.restype = ctypes.c_char_p

        lib.ptx_method_doc.argtypes = [ctypes.c_void_p]
        lib.ptx_method_doc.restype = ctypes.c_char_p

        lib.ptx_method_is_static.argtypes = [ctypes.c_void_p]
        lib.ptx_method_is_static.restype = ctypes.c_bool

        lib.ptx_method_arg_count.argtypes = [ctypes.c_void_p]
        lib.ptx_method_arg_count.restype = ctypes.c_size_t

        lib.ptx_method_arg_type.argtypes = [ctypes.c_void_p, ctypes.c_size_t]
        lib.ptx_method_arg_type.restype = ctypes.c_void_p

        lib.ptx_method_return_type.argtypes = [ctypes.c_void_p]
        lib.ptx_method_return_type.restype = ctypes.c_void_p

        lib.ptx_method_signature.argtypes = [ctypes.c_void_p]
        lib.ptx_method_signature.restype = ctypes.c_char_p

        lib.ptx_method_return_size.argtypes = [ctypes.c_void_p]
        lib.ptx_method_return_size.restype = ctypes.c_size_t

        lib.ptx_method_invoke.argtypes = [
            ctypes.c_void_p,
            ctypes.c_void_p,
            ctypes.POINTER(ctypes.c_void_p),
        ]
        lib.ptx_method_invoke.restype = ctypes.c_void_p

        lib.ptx_method_destroy_return.argtypes = [ctypes.c_void_p, ctypes.c_void_p]
        lib.ptx_method_destroy_return.restype = None

        lib.ptx_constructor_signature.argtypes = [ctypes.c_void_p]
        lib.ptx_constructor_signature.restype = ctypes.c_char_p

        lib.ptx_constructor_arg_count.argtypes = [ctypes.c_void_p]
        lib.ptx_constructor_arg_count.restype = ctypes.c_size_t

        lib.ptx_constructor_arg_type.argtypes = [ctypes.c_void_p, ctypes.c_size_t]
        lib.ptx_constructor_arg_type.restype = ctypes.c_void_p

        lib.ptx_constructor_invoke.argtypes = [
            ctypes.c_void_p,
            ctypes.POINTER(ctypes.c_void_p),
        ]
        lib.ptx_constructor_invoke.restype = ctypes.c_void_p

        lib.ptx_type_info_name.argtypes = [ctypes.c_void_p]
        lib.ptx_type_info_name.restype = ctypes.c_char_p

        lib.ptx_type_info_pretty_name.argtypes = [ctypes.c_void_p]
        lib.ptx_type_info_pretty_name.restype = ctypes.c_char_p

        lib.ptx_type_info_hash.argtypes = [ctypes.c_void_p]
        lib.ptx_type_info_hash.restype = ctypes.c_size_t


# ---------------------------------------------------------------------------
# High-level wrappers
# ---------------------------------------------------------------------------

def load_reflection(
    library: Optional[Union[str, os.PathLike[str]]] = None,
) -> "PTXReflection":
    """Load the reflection registry through the C ABI and return a helper."""

    return PTXReflection(library)


def _resolve_library_path(
    path: Optional[Union[str, os.PathLike[str]]],
) -> Path:
    if path:
        return Path(path)

    env = os.environ.get("PTX_C_API_LIB")
    if env:
        return Path(env)

    # Try platform lookup (e.g. ``libptx_c_api.so`` on Linux, ``ptx_c_api`` on Windows)
    for name in _DEFAULT_LIBRARY_NAMES:
        found = ctypes_util.find_library(name)
        if found:
            return Path(found)

    debug = bool(os.environ.get("PTX_REFLECT_DEBUG"))
    tried: list[str] = []

    # Upward walk from this file toward root, looking for plausible build dirs.
    this_file = Path(__file__).resolve()
    start_dir = this_file.parent
    # We expect to be at .../PTXEngine/bindings/python/ptx, so project root likely 3 levels up
    # but we generalize with a walk.
    filenames = [
        "libptx_reflect.so",  # primary
        "ptx_reflect.so",     # copied Lua-style name (if ever reused)
        "libptx_c_api.so",    # legacy fallback
    ]
    # Within each candidate root, check these relative subdirectories
    subdirs = [
        Path("build"),
        Path("build/lib"),
        Path("PTXEngine/build"),
        Path("PTXEngine/build/lib"),
    ]

    seen_roots: set[Path] = set()
    for ancestor in [start_dir] + list(start_dir.parents):
        if ancestor in seen_roots:
            continue
        seen_roots.add(ancestor)
        for sub in subdirs:
            root = ancestor / sub
            for fn in filenames:
                candidate = root / fn
                tried.append(str(candidate))
                if candidate.exists():
                    if debug:
                        print(f"[PTX_REFLECT_DEBUG] Using {candidate}")
                    return candidate

    if debug:
        print("[PTX_REFLECT_DEBUG] Tried paths:")
        for t in tried:
            print("  -", t)

    hint = " or ".join(_DEFAULT_LIBRARY_NAMES)
    raise FileNotFoundError(
        "Could not locate the PTX reflection shared library. "
        "Set PTX_C_API_LIB to the built library path or pass it explicitly. "
        f"Looked for {hint}."
    )


def _decode(optional_bytes: Optional[bytes]) -> Optional[str]:
    if not optional_bytes:
        return None
    return optional_bytes.decode("utf-8", errors="replace")


def _as_void_p(handle: Union[int, ctypes.c_void_p, None]) -> ctypes.c_void_p:
    if isinstance(handle, ctypes.c_void_p):
        return handle
    return ctypes.c_void_p(0 if handle is None else handle)


class PTXReflection:
    """Gateway to the global reflection registry."""

    def __init__(self, library: Optional[Union[str, os.PathLike[str]]] = None):
        self._clib = _PTXCLibrary(library)
        self._classes_by_ptr: dict[int, PTXClass] = {}
        self._classes_by_name: dict[str, PTXClass] = {}

    # ------------------------ class enumeration -------------------------

    def iter_classes(self) -> Iterator["PTXClass"]:
        count = self._clib.lib.ptx_registry_class_count()
        for index in range(count):
            handle = self._clib.lib.ptx_registry_class_at(index)
            cls = self._class_from_handle(handle)
            if cls:
                yield cls

    @property
    def classes(self) -> List["PTXClass"]:
        return list(self.iter_classes())

    def find_class(self, name: str) -> Optional["PTXClass"]:
        handle = self._clib.lib.ptx_registry_find_class(name.encode("utf-8"))
        return self._class_from_handle(handle)

    # ------------- helpers used by class/descriptor wrappers ------------

    @property
    def lib(self) -> ctypes.CDLL:
        return self._clib.lib

    def _class_from_handle(self, handle: Union[int, ctypes.c_void_p, None]) -> Optional["PTXClass"]:
        if not handle:
            return None
        ptr = _as_void_p(handle)
        key = int(ptr.value)
        cached = self._classes_by_ptr.get(key)
        if cached:
            return cached
        cls = PTXClass(self, ptr)
        self._classes_by_ptr[key] = cls
        name = cls.name
        if name:
            self._classes_by_name[name] = cls
        return cls

    def _field_type(self, handle: ctypes.c_void_p) -> "PTXType":
        return PTXType(self, self.lib.ptx_field_type(handle))

    def _method_arg_types(self, handle: ctypes.c_void_p) -> List["PTXType"]:
        count = self.lib.ptx_method_arg_count(handle)
        return [PTXType(self, self.lib.ptx_method_arg_type(handle, i)) for i in range(count)]

    def _method_return_type(self, handle: ctypes.c_void_p) -> Optional["PTXType"]:
        ptr = self.lib.ptx_method_return_type(handle)
        if not ptr:
            return None
        return PTXType(self, ptr)

    def _constructor_arg_types(self, handle: ctypes.c_void_p) -> List["PTXType"]:
        count = self.lib.ptx_constructor_arg_count(handle)
        return [PTXType(self, self.lib.ptx_constructor_arg_type(handle, i)) for i in range(count)]

    def class_by_name(self, name: str) -> Optional["PTXClass"]:
        cached = self._classes_by_name.get(name)
        if cached:
            return cached
        return self.find_class(name)


class PTXType:
    """Represents a C++ ``std::type_info`` entry."""

    def __init__(self, registry: PTXReflection, handle: Union[int, ctypes.c_void_p]):
        self._registry = registry
        self._handle = _as_void_p(handle)
        self._pretty: Optional[str] = None
        self._name: Optional[str] = None
        self._hash: Optional[int] = None
        self._analysis: Optional[Tuple[str, int, bool]] = None

    @property
    def handle(self) -> ctypes.c_void_p:
        return self._handle

    @property
    def name(self) -> Optional[str]:
        if self._name is None:
            self._name = _decode(self._registry.lib.ptx_type_info_name(self._handle))
        return self._name

    @property
    def pretty_name(self) -> Optional[str]:
        if self._pretty is None:
            self._pretty = _decode(self._registry.lib.ptx_type_info_pretty_name(self._handle))
        return self._pretty

    @property
    def hash(self) -> Optional[int]:
        if self._hash is None:
            value = self._registry.lib.ptx_type_info_hash(self._handle)
            self._hash = int(value) if value else 0
        return self._hash

    # ---------------------------- analysis -----------------------------

    def _analyze(self) -> Tuple[str, int, bool]:
        if self._analysis:
            return self._analysis
        pretty = self.pretty_name or ""
        base = pretty.strip()
        is_ref = base.endswith("&")
        if is_ref:
            base = base[:-1].strip()
        pointer_depth = 0
        while base.endswith("*"):
            pointer_depth += 1
            base = base[:-1].strip()
        if base.startswith("const "):
            base = base[len("const "):].strip()
        if base.startswith("volatile "):
            base = base[len("volatile "):].strip()
        self._analysis = (base, pointer_depth, is_ref)
        return self._analysis

    @property
    def base_name(self) -> str:
        return self._analyze()[0]

    @property
    def pointer_depth(self) -> int:
        return self._analyze()[1]

    @property
    def is_reference(self) -> bool:
        return self._analyze()[2]

    @property
    def is_pointer_like(self) -> bool:
        return self.pointer_depth > 0

    @property
    def is_fundamental(self) -> bool:
        base = self.base_name
        return base in _SIMPLE_TYPE_MAP

    def ctypes_type(self) -> Optional[ctypes._SimpleCData]:
        base = self.base_name
        ctype = _SIMPLE_TYPE_MAP.get(base)
        if ctype:
            return ctype
        return None

    def __repr__(self) -> str:
        return f"PTXType({self.pretty_name or '<unknown>'})"


class PTXClass:
    """Wrapper around ``ptx::ClassDesc``."""

    def __init__(self, registry: PTXReflection, handle: ctypes.c_void_p):
        self._registry = registry
        self._handle = handle
        self._fields: Optional[List[PTXField]] = None
        self._methods: Optional[List[PTXMethod]] = None
        self._ctors: Optional[List[PTXConstructor]] = None

    @property
    def handle(self) -> ctypes.c_void_p:
        return self._handle

    @property
    def name(self) -> Optional[str]:
        return _decode(self._registry.lib.ptx_class_name(self._handle))

    # ------------------------------ fields -----------------------------

    @property
    def fields(self) -> List["PTXField"]:
        if self._fields is None:
            count = self._registry.lib.ptx_class_field_count(self._handle)
            self._fields = []
            for idx in range(count):
                handle = self._registry.lib.ptx_class_field_at(self._handle, idx)
                if handle:
                    self._fields.append(PTXField(self, _as_void_p(handle)))
        return list(self._fields)

    def find_field(self, name: str) -> Optional["PTXField"]:
        handle = self._registry.lib.ptx_class_find_field(
            self._handle, name.encode("utf-8")
        )
        if not handle:
            return None
        return PTXField(self, _as_void_p(handle))

    # ------------------------------ methods ----------------------------

    @property
    def methods(self) -> List["PTXMethod"]:
        if self._methods is None:
            count = self._registry.lib.ptx_class_method_count(self._handle)
            self._methods = []
            for idx in range(count):
                handle = self._registry.lib.ptx_class_method_at(self._handle, idx)
                if handle:
                    self._methods.append(PTXMethod(self, _as_void_p(handle)))
        return list(self._methods)

    def find_method(self, name: str) -> Optional["PTXMethod"]:
        handle = self._registry.lib.ptx_class_find_method(
            self._handle, name.encode("utf-8")
        )
        if not handle:
            return None
        return PTXMethod(self, _as_void_p(handle))

    # --------------------------- constructors --------------------------

    @property
    def constructors(self) -> List["PTXConstructor"]:
        if self._ctors is None:
            count = self._registry.lib.ptx_class_constructor_count(self._handle)
            self._ctors = []
            for idx in range(count):
                handle = self._registry.lib.ptx_class_constructor_at(self._handle, idx)
                if handle:
                    self._ctors.append(PTXConstructor(self, _as_void_p(handle)))
        return list(self._ctors)

    def destroy_instance(self, instance: Union[int, ctypes.c_void_p, "PTXObject", None]) -> None:
        if isinstance(instance, PTXObject):
            ptr = instance.pointer
        else:
            ptr = instance
        if not ptr:
            return
        self._registry.lib.ptx_class_destroy_instance(self._handle, _as_void_p(ptr))

    # ------------------------ convenience helpers ---------------------

    def construct(self, *args: object, signature: Optional[str] = None) -> "PTXObject":
        """Invoke the first matching constructor and return a managed object."""

        ctors = self.constructors
        chosen: Optional[PTXConstructor] = None
        if signature:
            for ctor in ctors:
                if ctor.signature == signature:
                    chosen = ctor
                    break
        if chosen is None:
            # choose the first ctor with matching argc
            for ctor in ctors:
                if ctor.arg_count == len(args):
                    chosen = ctor
                    break
        if chosen is None:
            raise ValueError(
                f"No constructor on {self.name or '<unnamed>'} accepts {len(args)} args"
            )
        return chosen(*args)

    def __repr__(self) -> str:
        return f"PTXClass({self.name or hex(int(self._handle.value))})"


class PTXField:
    """Reflection view over ``ptx::FieldDecl``."""

    def __init__(self, owner: PTXClass, handle: ctypes.c_void_p):
        self._owner = owner
        self._handle = handle
        self._type: Optional[PTXType] = None

    @property
    def handle(self) -> ctypes.c_void_p:
        return self._handle

    @property
    def name(self) -> Optional[str]:
        return _decode(self._owner._registry.lib.ptx_field_name(self._handle))

    @property
    def description(self) -> Optional[str]:
        return _decode(self._owner._registry.lib.ptx_field_description(self._handle))

    @property
    def size(self) -> int:
        return int(self._owner._registry.lib.ptx_field_size(self._handle))

    @property
    def min_value(self) -> float:
        return float(self._owner._registry.lib.ptx_field_min_value(self._handle))

    @property
    def max_value(self) -> float:
        return float(self._owner._registry.lib.ptx_field_max_value(self._handle))

    @property
    def type(self) -> PTXType:
        if self._type is None:
            self._type = self._owner._registry._field_type(self._handle)
        return self._type

    def read(self, obj: "PTXObject") -> object:
        ptr = self._owner._registry.lib.ptx_field_get_const_ptr(
            self._handle, obj.pointer
        )
        if not ptr:
            raise ValueError("Field pointer is null")
        ptype = self.type
        ctype = ptype.ctypes_type()
        if ctype is None:
            raise TypeError(
                f"Field {self.name} has unsupported type {ptype.pretty_name} for direct read"
            )
        as_ptr = ctypes.cast(ctypes.c_void_p(ptr), ctypes.POINTER(ctype))
        return as_ptr.contents.value

    def write(self, obj: "PTXObject", value: object) -> None:
        ptr = self._owner._registry.lib.ptx_field_get_ptr(self._handle, obj.pointer)
        if not ptr:
            raise ValueError("Field pointer is null")
        ptype = self.type
        ctype = ptype.ctypes_type()
        if ctype is None:
            raise TypeError(
                f"Field {self.name} has unsupported type {ptype.pretty_name} for direct write"
            )
        target = ctypes.cast(ctypes.c_void_p(ptr), ctypes.POINTER(ctype))
        target.contents.value = ctype(value).value

    def __repr__(self) -> str:
        return f"PTXField({self.name or '<unnamed>'})"


class PTXMethod:
    """Reflection wrapper over ``ptx::MethodDesc`` with call helpers."""

    def __init__(self, owner: PTXClass, handle: ctypes.c_void_p):
        self._owner = owner
        self._handle = handle
        self._arg_types: Optional[List[PTXType]] = None
        self._ret_type: Optional[PTXType] = None

    @property
    def handle(self) -> ctypes.c_void_p:
        return self._handle

    @property
    def name(self) -> Optional[str]:
        return _decode(self._owner._registry.lib.ptx_method_name(self._handle))

    @property
    def doc(self) -> Optional[str]:
        return _decode(self._owner._registry.lib.ptx_method_doc(self._handle))

    @property
    def is_static(self) -> bool:
        return bool(self._owner._registry.lib.ptx_method_is_static(self._handle))

    @property
    def arg_types(self) -> List[PTXType]:
        if self._arg_types is None:
            self._arg_types = self._owner._registry._method_arg_types(self._handle)
        return list(self._arg_types)

    @property
    def return_type(self) -> Optional[PTXType]:
        if self._ret_type is None:
            self._ret_type = self._owner._registry._method_return_type(self._handle)
        return self._ret_type

    @property
    def signature(self) -> Optional[str]:
        return _decode(self._owner._registry.lib.ptx_method_signature(self._handle))

    @property
    def return_size(self) -> int:
        return int(self._owner._registry.lib.ptx_method_return_size(self._handle))

    def _prepare_args(self, args: Sequence[object]) -> Tuple[Optional[ctypes.Array], List[object]]:
        expected = self.arg_types
        if len(expected) != len(args):
            raise ValueError(
                f"Method {self.name or '<unnamed>'} expects {len(expected)} args, got {len(args)}"
            )
        if not args:
            return None, []
        keepalive: List[object] = []
        void_ptrs: List[ctypes.c_void_p] = []
        for ptype, value in zip(expected, args):
            void_ptrs.append(_box_argument(ptype, value, keepalive))
        array_type = ctypes.c_void_p * len(void_ptrs)
        arr = array_type(*[vp for vp in void_ptrs])
        keepalive.append(arr)
        return arr, keepalive

    def invoke(self, instance: Optional["PTXObject"] = None, *args: object) -> object:
        argv, keepalive = self._prepare_args(args)
        instance_ptr = instance.pointer if instance else None
        if instance_ptr is None and not self.is_static:
            raise ValueError(
                "Non-static method requires an instance"
            )
        raw = self._owner._registry.lib.ptx_method_invoke(
            self._handle,
            _as_void_p(instance_ptr),
            argv,
        )
        result = _unbox_result(self, raw)
        # ensure keepalive not garbage collected before invocation completes
        keepalive.clear()
        return result

    def destroy_return(self, value_ptr: Union[int, ctypes.c_void_p, None]) -> None:
        if not value_ptr:
            return
        self._owner._registry.lib.ptx_method_destroy_return(
            self._handle, _as_void_p(value_ptr)
        )

    def __call__(self, instance: Optional["PTXObject"] = None, *args: object) -> object:
        return self.invoke(instance, *args)

    def __repr__(self) -> str:
        return f"PTXMethod({self.name or '<unnamed>'})"


class PTXConstructor:
    """Reflection wrapper over ``ptx::ConstructorDesc``."""

    def __init__(self, owner: PTXClass, handle: ctypes.c_void_p):
        self._owner = owner
        self._handle = handle
        self._arg_types: Optional[List[PTXType]] = None

    @property
    def handle(self) -> ctypes.c_void_p:
        return self._handle

    @property
    def signature(self) -> Optional[str]:
        return _decode(self._owner._registry.lib.ptx_constructor_signature(self._handle))

    @property
    def arg_types(self) -> List[PTXType]:
        if self._arg_types is None:
            self._arg_types = self._owner._registry._constructor_arg_types(self._handle)
        return list(self._arg_types)

    @property
    def arg_count(self) -> int:
        return len(self.arg_types)

    def _prepare_args(self, args: Sequence[object]) -> Tuple[Optional[ctypes.Array], List[object]]:
        expected = self.arg_types
        if len(expected) != len(args):
            raise ValueError(
                f"Constructor expects {len(expected)} args, got {len(args)}"
            )
        if not args:
            return None, []
        keepalive: List[object] = []
        void_ptrs: List[ctypes.c_void_p] = []
        for ptype, value in zip(expected, args):
            void_ptrs.append(_box_argument(ptype, value, keepalive))
        array_type = ctypes.c_void_p * len(void_ptrs)
        arr = array_type(*[vp for vp in void_ptrs])
        keepalive.append(arr)
        return arr, keepalive

    def invoke(self, *args: object) -> "PTXObject":
        argv, keepalive = self._prepare_args(args)
        raw_ptr = self._owner._registry.lib.ptx_constructor_invoke(
            self._handle, argv
        )
        keepalive.clear()
        if not raw_ptr:
            raise RuntimeError("Constructor returned null pointer")
        return PTXObject(
            self._owner,
            _as_void_p(raw_ptr),
            destroy=self._owner.destroy_instance,
        )

    def __call__(self, *args: object) -> "PTXObject":
        return self.invoke(*args)

    def __repr__(self) -> str:
        return f"PTXConstructor({self.signature or '<default>'})"


class PTXObject:
    """Opaque owner for an instance created through the registry."""

    def __init__(
        self,
        cls: Optional[PTXClass],
        pointer: ctypes.c_void_p,
        destroy: Optional[Callable[[ctypes.c_void_p], None]] = None,
    ):
        self.cls = cls
        self._pointer = _as_void_p(pointer)
        self._destroy = destroy
        self._alive = True

    @property
    def pointer(self) -> ctypes.c_void_p:
        return self._pointer

    def release(self) -> None:
        if self._alive and self._destroy and self._pointer:
            self._destroy(self._pointer)
        self._pointer = ctypes.c_void_p()
        self._alive = False

    def __del__(self) -> None:
        try:
            self.release()
        except Exception:
            pass

    def __repr__(self) -> str:
        cls_name = self.cls.name if self.cls else "<unknown>"
        return f"PTXObject({cls_name}, {hex(int(self._pointer.value)) if self._pointer else 'null'})"


# ---------------------------------------------------------------------------
# Argument/return boxing helpers
# ---------------------------------------------------------------------------

def _box_argument(ptype: PTXType, value: object, keepalive: List[object]) -> ctypes.c_void_p:
    """Convert a Python value into the ``void*`` slot expected by the ABI."""

    ctype = ptype.ctypes_type()
    if isinstance(value, ctypes._SimpleCData):
        keepalive.append(value)
        return ctypes.cast(ctypes.byref(value), ctypes.c_void_p)

    if ctype is not None and ptype.pointer_depth == 0:
        c_value = ctype(value)
        keepalive.append(c_value)
        return ctypes.cast(ctypes.byref(c_value), ctypes.c_void_p)

    if isinstance(value, PTXObject):
        return value.pointer

    raise TypeError(
        f"Cannot box argument of type {ptype.pretty_name} from value {value!r}"
    )


def _unbox_result(method: PTXMethod, raw_ptr: Union[int, ctypes.c_void_p]) -> object:
    if not raw_ptr:
        return None
    ret_type = method.return_type
    if ret_type is None:
        return None

    ctype = ret_type.ctypes_type()
    ptr = _as_void_p(raw_ptr)

    if ctype is not None and ret_type.pointer_depth == 0:
        value = ctypes.cast(ptr, ctypes.POINTER(ctype)).contents.value
        method.destroy_return(ptr)
        return value

    # class-like return; try to wrap into PTXObject using class lookup.
    cls = method._owner._registry.class_by_name(ret_type.base_name)
    destroy = lambda p: method.destroy_return(p)
    return PTXObject(cls, ptr, destroy=destroy)


