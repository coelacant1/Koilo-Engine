// SPDX-License-Identifier: GPL-3.0-or-later
#include <koilo/scripting/koiloscript_engine.hpp>
#include <koilo/scripting/display_config.hpp>
#include <koilo/core/math/mathematics.hpp>
#include <koilo/core/signal/functiongenerator.hpp>
#include <koilo/systems/scene/animation/ieasyeaseanimator.hpp>
#include <koilo/core/time/timemanager.hpp>
#include <koilo/systems/render/sky/sky.hpp>

// Direct system headers (replacing module wrappers)
#include <koilo/systems/physics/physicsworld.hpp>
#include <koilo/systems/ui/ui.hpp>
#ifdef KOILO_ENABLE_PARTICLES
#include <koilo/systems/particles/particlesystem.hpp>
#endif
#ifdef KOILO_ENABLE_AI
#include <koilo/systems/ai/script_ai_manager.hpp>
#endif
#ifdef KOILO_ENABLE_AUDIO
#include <koilo/systems/audio/script_audio_manager.hpp>
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
    pixelGroup_ = new PixelGroup(pixelWidth * pixelHeight, size, position, pixelWidth);
    
    // Default camera setup
    Vector3D camPos(0, 0, 8);
    cameraTransform_ = new Transform(Vector3D(0, 0, 0), camPos, Vector3D(1, 1, 1));
    cameraLayout_ = new CameraLayout(CameraLayout::YNForward, CameraLayout::ZUp);
    camera_ = new Camera(cameraTransform_, cameraLayout_, pixelGroup_);
    
    scene_ = new Scene();
}


void KoiloScriptEngine::RegisterSceneGlobal() {
    if (scene_) {
        RegisterGlobal("scene", "Scene", scene_);
    }
    if (camera_) {
        RegisterGlobal("cam", "Camera", camera_);
    }
}

void KoiloScriptEngine::RegisterDefaultModules() {
    // Physics (always on)
    physicsWorld_ = new PhysicsWorld();
    RegisterGlobal("physics", "PhysicsWorld", physicsWorld_);
    physicsWorld_->OnCollisionEnter([this](const CollisionEvent& evt) {
        CallFunction("OnCollisionEnter");
    });
    physicsWorld_->OnCollisionExit([this](const CollisionEvent& evt) {
        CallFunction("OnCollisionExit");
    });

    // UI (always on)
    ui_ = new UI();
    RegisterGlobal("ui", "UI", ui_);

    // Particles
#ifdef KOILO_ENABLE_PARTICLES
    particleSystem_ = new ParticleSystem();
    RegisterGlobal("particles", "ParticleSystem", particleSystem_);
#endif

#ifdef KOILO_ENABLE_AI
    aiManager_ = new ScriptAIManager();
    RegisterGlobal("ai", "ScriptAIManager", aiManager_);
#endif

#ifdef KOILO_ENABLE_AUDIO
    audioManager_ = new ScriptAudioManager();
    RegisterGlobal("audio", "ScriptAudioManager", audioManager_);
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
