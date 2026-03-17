// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file ai_module.hpp
 * @brief AI system as an IModule.
 *
 * Wraps ScriptAIManager (state machines, behavior trees, pathfinding)
 * in the module lifecycle.
 */

#pragma once

#include <koilo/kernel/unified_module.hpp>
#include <memory>
#include "../../registry/reflect_macros.hpp"

namespace koilo {

class ScriptAIManager;

class AIModule : public IModule {
public:
    AIModule();
    ~AIModule() override;

    ModuleInfo GetInfo() const override;
    bool Initialize(KoiloKernel& kernel) override;
    void Update(float dt) override;
    void Render(Color888* buffer, int width, int height) override;
    void Shutdown() override;

    ScriptAIManager* GetManager() { return manager_.get(); }

private:
    std::unique_ptr<ScriptAIManager> manager_;

    KL_BEGIN_FIELDS(AIModule)
        /* No reflected fields. */
    KL_END_FIELDS

    KL_BEGIN_METHODS(AIModule)
        KL_METHOD_AUTO(AIModule, GetInfo, "Get info"),
        KL_METHOD_AUTO(AIModule, Update, "Update"),
        KL_METHOD_AUTO(AIModule, Shutdown, "Shutdown"),
        KL_METHOD_AUTO(AIModule, GetManager, "Get manager")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(AIModule)
        KL_CTOR0(AIModule)
    KL_END_DESCRIBE(AIModule)

};

} // namespace koilo
