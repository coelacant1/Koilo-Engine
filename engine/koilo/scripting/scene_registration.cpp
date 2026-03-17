// SPDX-License-Identifier: GPL-3.0-or-later
#include <koilo/scripting/koiloscript_engine.hpp>
#include <koilo/scripting/display_config.hpp>
#include <koilo/core/math/mathematics.hpp>
#include <koilo/core/signal/functiongenerator.hpp>
#include <koilo/systems/scene/animation/ieasyeaseanimator.hpp>
#include <koilo/core/time/timemanager.hpp>
#include <koilo/systems/render/sky/sky.hpp>
#include <koilo/systems/render/core/pixelgroup.hpp>
#include <koilo/systems/scene/camera/camera.hpp>
#include <koilo/systems/scene/camera/cameralayout.hpp>
#include <koilo/systems/scene/scene.hpp>

// Direct system headers (replacing module wrappers)
#include <koilo/systems/physics/physicsworld.hpp>
#include <koilo/systems/physics/physics_module.hpp>
#include <koilo/systems/ui/ui.hpp>
#include <koilo/systems/asset/asset_module.hpp>
#ifdef KOILO_ENABLE_PARTICLES
#include <koilo/systems/particles/particlesystem.hpp>
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
    
    // Create PixelGroup
    Vector2D size((float)pixelWidth, (float)pixelHeight);
    Vector2D position(0, 0);
    pixelGroup_ = std::make_unique<PixelGroup>(pixelWidth * pixelHeight, size, position, pixelWidth);
    
    // Default camera setup
    Vector3D camPos(0, 0, 8);
    cameraTransform_ = std::make_unique<Transform>(Vector3D(0, 0, 0), camPos, Vector3D(1, 1, 1));
    cameraLayout_ = std::make_unique<CameraLayout>(CameraLayout::YNForward, CameraLayout::ZUp);
    camera_ = std::make_unique<Camera>(cameraTransform_.get(), cameraLayout_.get(), pixelGroup_.get());
    
    scene_ = std::make_unique<Scene>();
}


void KoiloScriptEngine::RegisterSceneGlobal() {
    if (scene_) {
        RegisterGlobal("scene", "Scene", scene_.get());
    }
    if (camera_) {
        RegisterGlobal("cam", "Camera", camera_.get());
    }
}

void KoiloScriptEngine::RegisterDefaultModules() {
    // Asset pipeline - registered early so other modules can use it
    moduleLoader_.Register(std::make_unique<AssetModule>());

    // Physics - registered as a module, Step() called from module Update()
    moduleLoader_.Register(std::make_unique<PhysicsModule>());

    // UI (always on) - ensure reflection is registered before creating globals
    (void)UI::Describe();
    ui_ = std::make_unique<UI>();
    ui_->Context().SetScriptCallback([this](const char* fnName) {
        CallFunction(std::string(fnName));
    });
    RegisterGlobal("ui", "UI", ui_.get());

    // Particles
#ifdef KOILO_ENABLE_PARTICLES
    particleSystem_ = std::make_unique<ParticleSystem>();
    RegisterGlobal("particles", "ParticleSystem", particleSystem_.get());
#endif

#ifdef KOILO_ENABLE_AI
    moduleLoader_.Register(std::make_unique<AIModule>());
#endif

#ifdef KOILO_ENABLE_AUDIO
    moduleLoader_.Register(std::make_unique<AudioModule>());
#endif
}

void KoiloScriptEngine::RegisterInputGlobal() {
    RegisterGlobal("input", "InputManager", &inputManager_);
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

} // namespace scripting
} // namespace koilo
