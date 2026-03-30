// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file ui_module.hpp
 * @brief UI system as an IModule with Overlay lifecycle phase.
 *
 * Owns the UI instance and manages font loading, animation updates,
 * and GPU/software rendering through the module lifecycle.
 *
 * @date 02/06/2026
 * @author Coela
 */
#pragma once

#include <koilo/kernel/unified_module.hpp>
#include <memory>
#include "../../registry/reflect_macros.hpp"

namespace koilo {

class UI;
namespace ui { class WidgetTypeRegistry; }

class UIModule : public IModule {
public:
    UIModule();
    ~UIModule() override;

    ModuleInfo GetInfo() const override;
    bool Initialize(KoiloKernel& kernel) override;
    void Update(float dt) override;
    void Render(Color888* buffer, int width, int height) override;
    void Shutdown() override;

    UI* GetUI() { return ui_.get(); }

    /// Update animations only (software path, called separately).
    void UpdateAnimations(float dt);

private:
    std::unique_ptr<UI> ui_;
    std::unique_ptr<ui::WidgetTypeRegistry> widgetRegistry_;

    /// Ensure font is loaded (lazy init).
    void EnsureFont();

    KL_BEGIN_FIELDS(UIModule)
        /* No reflected fields. */
    KL_END_FIELDS

    KL_BEGIN_METHODS(UIModule)
        KL_METHOD_AUTO(UIModule, GetInfo, "Get info"),
        KL_METHOD_AUTO(UIModule, Initialize, "Initialize"),
        KL_METHOD_AUTO(UIModule, Update, "Update"),
        KL_METHOD_AUTO(UIModule, Render, "Render"),
        KL_METHOD_AUTO(UIModule, Shutdown, "Shutdown"),
        KL_METHOD_AUTO(UIModule, GetUI, "Get ui")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(UIModule)
        KL_CTOR0(UIModule)
    KL_END_DESCRIBE(UIModule)

};

} // namespace koilo
