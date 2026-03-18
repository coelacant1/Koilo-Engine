// SPDX-License-Identifier: GPL-3.0-or-later
#include <koilo/scripting/koiloscript_engine.hpp>
#include <koilo/scripting/display_config.hpp>
#include <koilo/core/math/mathematics.hpp>
#include <koilo/core/signal/functiongenerator.hpp>
#include <koilo/systems/scene/animation/ieasyeaseanimator.hpp>
#include <koilo/core/time/timemanager.hpp>
#include <koilo/systems/render/sky/sky.hpp>
#include <koilo/systems/render/core/pixelgroup.hpp>
#include <koilo/systems/render/irenderbackend.hpp>
#include <koilo/kernel/kernel.hpp>
#include <koilo/kernel/logging/log.hpp>
#include <koilo/systems/scene/camera/camera.hpp>
#include <koilo/systems/scene/camera/cameralayout.hpp>
#include <koilo/systems/scene/scene.hpp>

// Direct system headers (replacing module wrappers)
#include <koilo/systems/physics/physicsworld.hpp>
#include <koilo/systems/physics/physics_module.hpp>
#include <koilo/systems/input/input_module.hpp>
#include <koilo/systems/ui/ui_module.hpp>
#include <koilo/systems/ui/ui.hpp>
#include <koilo/systems/render/render_module.hpp>
#include <koilo/systems/scene/scene_module.hpp>
#include <koilo/systems/asset/asset_module.hpp>
#ifdef KOILO_ENABLE_PARTICLES
#include <koilo/systems/particles/particle_module.hpp>
#endif
#ifdef KOILO_ENABLE_AI
#include <koilo/systems/ai/ai_module.hpp>
#endif
#ifdef KOILO_ENABLE_AUDIO
#include <koilo/systems/audio/audio_module.hpp>
#endif

namespace koilo {
namespace scripting {

void KoiloScriptEngine::RegisterGlobal(const char* name, const char* className, void* instance) {
    const ClassDesc* desc = ReflectionBridge::FindClass(className);
    if (desc) {
        activeCtx_->reflectedObjects[name] = ReflectedObject(instance, desc, name, false);
    }
}

void KoiloScriptEngine::BuildCamera() {
    // Get pixel dimensions from display config
    int pixelWidth = 192, pixelHeight = 94;
    auto pwIt = displayConfig.find("pixel_width");
    auto phIt = displayConfig.find("pixel_height");
    if (pwIt != displayConfig.end()) pixelWidth = std::stoi(pwIt->second);
    if (phIt != displayConfig.end()) pixelHeight = std::stoi(phIt->second);
    
    // Delegate to SceneModule
    auto* mod = dynamic_cast<SceneModule*>(moduleLoader_.GetModule("scene"));
    if (mod) {
        mod->BuildCamera(pixelWidth, pixelHeight);
    }
}


void KoiloScriptEngine::RegisterSceneGlobal() {
    // Scene/camera registration handled by SceneModule::BuildCamera
}

void KoiloScriptEngine::RegisterDefaultModules() {
    // Asset pipeline - registered early so other modules can use it
    moduleLoader_.Register(std::make_unique<AssetModule>());

    // Physics - registered as a module, Step() called from module Update()
    moduleLoader_.Register(std::make_unique<PhysicsModule>());

    // Scene - managed as a module with Core phase (camera built later)
    moduleLoader_.Register(std::make_unique<SceneModule>());

    // Input - managed as a module with Core phase
    moduleLoader_.Register(std::make_unique<InputModule>());

    // UI - managed as a module with Overlay phase
    moduleLoader_.Register(std::make_unique<UIModule>());

    // Render - managed as a module (backend set later by host)
    moduleLoader_.Register(std::make_unique<RenderModule>());

    // Particles - managed as a module with Render phase
#ifdef KOILO_ENABLE_PARTICLES
    moduleLoader_.Register(std::make_unique<ParticleModule>());
#endif

#ifdef KOILO_ENABLE_AI
    moduleLoader_.Register(std::make_unique<AIModule>());
#endif

#ifdef KOILO_ENABLE_AUDIO
    moduleLoader_.Register(std::make_unique<AudioModule>());
#endif
}

void KoiloScriptEngine::RegisterInputGlobal() {
    // Input registration handled by InputModule
}

void KoiloScriptEngine::RegisterDebugGlobal() {
    RegisterGlobal("debug", "DebugDraw", &DebugDraw::GetInstance());
    RegisterGlobal("sky", "Sky", &Sky::GetInstance());
}

void KoiloScriptEngine::RegisterEntitiesGlobal() {
    RegisterGlobal("entities", "ScriptEntityManager", &scriptEntities_);
}

void KoiloScriptEngine::RegisterWorldGlobal() {
    RegisterGlobal("world", "ScriptWorldManager", &scriptWorld_);
}

void KoiloScriptEngine::RegisterStaticGlobals() {
    RegisterGlobal("Time", "TimeManager", &TimeManager::GetInstance());
    
    // Register enum constants as script variables
    // FunctionGenerator::Function
    SetGlobal("Triangle",  Value(static_cast<double>(FunctionGenerator::Triangle)));
    SetGlobal("Square",    Value(static_cast<double>(FunctionGenerator::Square)));
    SetGlobal("Sine",      Value(static_cast<double>(FunctionGenerator::Sine)));
    SetGlobal("Sawtooth",  Value(static_cast<double>(FunctionGenerator::Sawtooth)));
    SetGlobal("Gravity",   Value(static_cast<double>(FunctionGenerator::Gravity)));
    
    // IEasyEaseAnimator::InterpolationMethod
    SetGlobal("Cosine",    Value(static_cast<double>(IEasyEaseAnimator::Cosine)));
    SetGlobal("Bounce",    Value(static_cast<double>(IEasyEaseAnimator::Bounce)));
    SetGlobal("Linear",    Value(static_cast<double>(IEasyEaseAnimator::Linear)));
    SetGlobal("Overshoot", Value(static_cast<double>(IEasyEaseAnimator::Overshoot)));
}

void KoiloScriptEngine::RegisterEngineServices() {
    if (!kernel_) return;
    auto& svc = kernel_->Services();

    // scene/camera registered by SceneModule, ui by UIModule, input by InputModule
    svc.Register("entities", &scriptEntities_);
    svc.Register("world",    &scriptWorld_);
    svc.Register("signals",  &signalRegistry_);
    // particles registered by ParticleModule, render_backend by RenderModule

    KL_DBG("koilo", "Registered engine subsystems as kernel services");
}

void KoiloScriptEngine::UnregisterEngineServices() {
    if (!kernel_) return;
    auto& svc = kernel_->Services();

    const char* names[] = {
        "scene", "camera", "ui", "input", "entities",
        "world", "signals", "particles", "render_backend"
    };
    for (auto* name : names) {
        if (svc.Has(name)) svc.Unregister(name);
    }
}

} // namespace scripting
} // namespace koilo
