// SPDX-License-Identifier: GPL-3.0-or-later
/// @file canvas2d.cpp
/// Out-of-class definitions for Canvas2D's reflection metadata.
///
/// Keeping Methods(), Fields(), and Describe() here (rather than inline in
/// the header) gives each a single strong external-linkage definition.  This
/// eliminates the COMDAT/weak-symbol ambiguity that arises when multiple
/// translation units include canvas2d.hpp and the build cache contains stale
/// object files compiled against an older version of the header.

#include <koilo/systems/render/canvas2d.hpp>

namespace koilo {

KL_DEFINE_FIELDS(Canvas2D)
KL_END_FIELDS

KL_DEFINE_METHODS(Canvas2D)
    KL_METHOD_AUTO(Canvas2D, Attach,         "Attach for rendering"),
    KL_METHOD_AUTO(Canvas2D, Detach,         "Detach from rendering"),
    KL_METHOD_AUTO(Canvas2D, IsAttached,     "Is attached"),
    KL_METHOD_AUTO(Canvas2D, Resize,         "Resize canvas"),
    KL_METHOD_AUTO(Canvas2D, RenderSize,     "Set render buffer size"),
    KL_METHOD_AUTO(Canvas2D, Scale,          "Set display scale (scales canvas to this size on composite)"),
    KL_METHOD_AUTO(Canvas2D, EnsureSize,     "Ensure canvas size"),
    KL_METHOD_AUTO(Canvas2D, Clear,          "Clear canvas"),
    KL_METHOD_AUTO(Canvas2D, ClearWithColor, "Clear with color"),
    KL_METHOD_AUTO(Canvas2D, SetPixel,       "Set pixel"),
    KL_METHOD_AUTO(Canvas2D, FillRect,       "Fill rectangle"),
    KL_METHOD_AUTO(Canvas2D, DrawRect,       "Draw rectangle outline"),
    KL_METHOD_AUTO(Canvas2D, DrawLine,       "Draw line"),
    KL_METHOD_AUTO(Canvas2D, DrawCircle,     "Draw circle outline"),
    KL_METHOD_AUTO(Canvas2D, FillCircle,     "Fill circle"),
    KL_METHOD_AUTO(Canvas2D, DrawText,       "Draw 3x5 text"),
    KL_METHOD_AUTO(Canvas2D, GetWidth,       "Get render width"),
    KL_METHOD_AUTO(Canvas2D, GetHeight,      "Get render height")
KL_END_METHODS

KL_DEFINE_DESCRIBE(Canvas2D)
    KL_CTOR0(Canvas2D)
KL_END_DESCRIBE(Canvas2D)

} // namespace koilo
