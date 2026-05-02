// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file igpu_render_backend.hpp
 * @brief Back-compat alias for the now-collapsed GPU render backend.
 *
 * The GPU-oriented hooks (BlitToScreen, CompositeCanvasOverlays,
 * PrepareFrame, FinishFrame) live directly on IRenderBackend with no-op
 * defaults. This header is retained so existing includes keep compiling;
 * `IGPURenderBackend` is now an alias for `IRenderBackend`. (C1)
 *
 * @date 03/15/2026
 * @author Coela
 */

#pragma once

#include <koilo/systems/render/irenderbackend.hpp>

namespace koilo {

using IGPURenderBackend = IRenderBackend;

} // namespace koilo
