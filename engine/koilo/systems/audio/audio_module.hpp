// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file audio_module.hpp
 * @brief Audio system as an IModule.
 *
 * Wraps ScriptAudioManager in the module lifecycle so it is loaded,
 * initialized, updated, and shut down by the ModuleLoader instead
 * of being manually managed inside KoiloScriptEngine.
 */

#pragma once

#include <koilo/kernel/unified_module.hpp>
#include <memory>
#include "../../registry/reflect_macros.hpp"

namespace koilo {

class ScriptAudioManager;

class AudioModule : public IModule {
public:
    AudioModule();
    ~AudioModule() override;

    ModuleInfo GetInfo() const override;
    bool Initialize(KoiloKernel& kernel) override;
    void Update(float dt) override;
    void Render(Color888* buffer, int width, int height) override;
    void Shutdown() override;

    ScriptAudioManager* GetManager() { return manager_.get(); }

private:
    std::unique_ptr<ScriptAudioManager> manager_;

    KL_BEGIN_FIELDS(AudioModule)
        /* No reflected fields. */
    KL_END_FIELDS

    KL_BEGIN_METHODS(AudioModule)
        KL_METHOD_AUTO(AudioModule, GetInfo, "Get info"),
        KL_METHOD_AUTO(AudioModule, Update, "Update"),
        KL_METHOD_AUTO(AudioModule, Shutdown, "Shutdown"),
        KL_METHOD_AUTO(AudioModule, GetManager, "Get manager")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(AudioModule)
        KL_CTOR0(AudioModule)
    KL_END_DESCRIBE(AudioModule)

};

} // namespace koilo
