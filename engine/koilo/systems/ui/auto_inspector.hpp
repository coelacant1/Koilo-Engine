// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file auto_inspector.hpp
 * @brief Reflection-driven inspector UI generator.
 *
 * Given a ClassDesc and a live object pointer, GenerateInspector builds
 * a widget tree with appropriate controls for every registered field.
 * Widget selection is driven by FieldKind and FieldHint metadata.
 *
 * @date 03/08/2026
 * @author Coela
 */

#pragma once

#include <koilo/registry/registry.hpp>
#include "ui_context.hpp"
#include "../../registry/reflect_macros.hpp"

namespace koilo {
namespace ui {

/// Result returned by GenerateInspector.
struct InspectorResult {
    int rootWidget = -1;   ///< Pool index of the inspector root panel
    int fieldCount = 0;    ///< Number of visible fields generated

    KL_BEGIN_FIELDS(InspectorResult)
        /* No reflected fields. */
    KL_END_FIELDS

    KL_BEGIN_METHODS(InspectorResult)
        /* No reflected methods. */
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(InspectorResult)
        /* No reflected ctors. */
    KL_END_DESCRIBE(InspectorResult)

};

/// Build a widget tree that inspects all reflected fields of an object.
/// @param desc        ClassDesc from the reflection system
/// @param instance    Live object pointer (must remain valid while inspector exists)
/// @param ctx         UIContext to create widgets in
/// @param parentIdx   Parent widget to attach to (-1 uses the context root)
/// @return InspectorResult with root panel index and visible field count
InspectorResult GenerateInspector(const ClassDesc* desc, void* instance,
                                  UIContext& ctx, int parentIdx = -1);

/// Update inspector widget values from the current object state.
/// Call this each frame or after external changes to keep the UI in sync.
/// @param desc           ClassDesc from the reflection system
/// @param instance       Live object pointer
/// @param ctx            UIContext containing the inspector widgets
/// @param inspectorRoot  Root panel returned by GenerateInspector
void RefreshInspector(const ClassDesc* desc, void* instance,
                      UIContext& ctx, int inspectorRoot);

} // namespace ui
} // namespace koilo
