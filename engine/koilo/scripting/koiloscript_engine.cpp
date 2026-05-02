// SPDX-License-Identifier: GPL-3.0-or-later
#include <koilo/scripting/koiloscript_engine.hpp>
#include <koilo/scripting/bytecode_compiler.hpp>
#include <koilo/scripting/bytecode_vm.hpp>
#include <koilo/scripting/value_marshaller.hpp>
#include <koilo/scripting/display_config.hpp>
#include <koilo/registry/ensure_registration.hpp>
#include <koilo/systems/render/irenderbackend.hpp>
#include <koilo/systems/render/igpu_render_backend.hpp>
#include <koilo/systems/render/core/pixelgroup.hpp>
#include <koilo/systems/scene/camera/camera.hpp>
#include <koilo/systems/scene/camera/cameralayout.hpp>
#include <koilo/systems/scene/scene.hpp>
#include <koilo/systems/render/canvas2d.hpp>
#include <koilo/systems/scene/morphablemesh.hpp>
#include <koilo/systems/scene/primitivemesh.hpp>
#include <koilo/systems/render/raster/rasterizer.hpp>
#include <koilo/core/time/timemanager.hpp>
#include <koilo/debug/profiler.hpp>
#include <koilo/debug/debugdraw.hpp>
#include <koilo/debug/debugrenderer.hpp>
#include <koilo/systems/render/sky/sky.hpp>
#include <koilo/systems/physics/physicsworld.hpp>
#include <koilo/systems/physics/physics_module.hpp>
#include <koilo/systems/input/input_module.hpp>
#include <koilo/systems/ui/ui_module.hpp>
#include <koilo/systems/render/render_module.hpp>
#include <koilo/systems/scene/scene_module.hpp>
#include <koilo/kernel/kernel.hpp>
#include <koilo/systems/ui/ui.hpp>
#ifdef KOILO_ENABLE_PARTICLES
#include <koilo/systems/particles/particlesystem.hpp>
#include <koilo/systems/particles/particle_module.hpp>
#endif
#ifdef KOILO_ENABLE_AI
#include <koilo/systems/ai/script_ai_manager.hpp>
#include <koilo/systems/ai/ai_module.hpp>
#endif
#ifdef KOILO_ENABLE_AUDIO
#include <koilo/systems/audio/script_audio_manager.hpp>
#include <koilo/systems/audio/audio_module.hpp>
#endif
#include <cstring>
#include <cstdio>
#include <algorithm>

namespace koilo {
namespace scripting {

// Fast double->string (replaces std::ostringstream on hot path)
static inline std::string NumToStr(double val) {
    char buf[32];
    int len = snprintf(buf, sizeof(buf), "%g", val);
    return std::string(buf, len > 0 ? len : 0);
}

// Force registration of all core types that scripts need
static void EnsureCommonTypesRegistered() {
    static bool registered = false;
    if (!registered) {
        registry::EnsureCoreReflectionRegistered();
        registered = true;
    }
}

KoiloScriptEngine::KoiloScriptEngine(platform::IScriptFileReader* fileReader, bool loadDefaultModules)
    : fileReader(fileReader), hasError(false), activeCtx_(&ctx_) {
    EnsureCommonTypesRegistered();
    if (loadDefaultModules) RegisterDefaultModules();
}

KoiloScriptEngine::~KoiloScriptEngine() {
    UnregisterEngineServices();
    CleanupAllPersistentObjects();
    for (auto* mesh : ownedMeshes_) {
        delete mesh;
    }
    ownedMeshes_.clear();
    // Shutdown module-managed systems first
    moduleLoader_.ShutdownAll();
    // Directly-owned subsystems cleaned up by unique_ptr destructors
}

// --- Module-backed system accessors ------------------------------------------

PhysicsWorld* KoiloScriptEngine::GetPhysicsWorld() {
    auto* mod = dynamic_cast<PhysicsModule*>(moduleLoader_.GetModule("physics"));
    return mod ? mod->GetWorld() : nullptr;
}

ScriptAIManager* KoiloScriptEngine::GetAI() {
#ifdef KOILO_ENABLE_AI
    auto* mod = dynamic_cast<AIModule*>(moduleLoader_.GetModule("ai"));
    return mod ? mod->GetManager() : nullptr;
#else
    return nullptr;
#endif
}

ScriptAudioManager* KoiloScriptEngine::GetAudio() {
#ifdef KOILO_ENABLE_AUDIO
    auto* mod = dynamic_cast<AudioModule*>(moduleLoader_.GetModule("audio"));
    return mod ? mod->GetManager() : nullptr;
#else
    return nullptr;
#endif
}

ParticleSystem* KoiloScriptEngine::GetParticleSystem() {
#ifdef KOILO_ENABLE_PARTICLES
    auto* mod = dynamic_cast<ParticleModule*>(moduleLoader_.GetModule("particles"));
    return mod ? mod->GetSystem() : nullptr;
#else
    return nullptr;
#endif
}

InputManager* KoiloScriptEngine::GetInputManager() {
    auto* mod = dynamic_cast<InputModule*>(moduleLoader_.GetModule("input"));
    return mod ? &mod->GetManager() : nullptr;
}

UI* KoiloScriptEngine::GetUI() {
    auto* mod = dynamic_cast<UIModule*>(moduleLoader_.GetModule("ui"));
    return mod ? mod->GetUI() : nullptr;
}

IRenderBackend* KoiloScriptEngine::GetRenderBackend() {
    auto* mod = dynamic_cast<RenderModule*>(moduleLoader_.GetModule("render"));
    return mod ? mod->GetBackend() : nullptr;
}

Scene* KoiloScriptEngine::GetScene() const {
    auto* mod = dynamic_cast<SceneModule*>(const_cast<ModuleLoader&>(moduleLoader_).GetModule("scene"));
    return mod ? mod->GetScene() : nullptr;
}

Camera* KoiloScriptEngine::GetCamera() const {
    auto* mod = dynamic_cast<SceneModule*>(const_cast<ModuleLoader&>(moduleLoader_).GetModule("scene"));
    return mod ? mod->GetCamera() : nullptr;
}

PixelGroup* KoiloScriptEngine::GetPixelGroup() const {
    auto* mod = dynamic_cast<SceneModule*>(const_cast<ModuleLoader&>(moduleLoader_).GetModule("scene"));
    return mod ? mod->GetPixelGroup() : nullptr;
}

bool KoiloScriptEngine::LoadScript(const char* filepath) {
    if (engineState_ != EngineState::Created) {
        SetError("LoadScript() must be called before BuildScene() (engine already initialized)");
        engineState_ = EngineState::Error;
        return false;
    }
    currentScriptPath = filepath;
    
    // 1. Read file
    std::string sourceCode;
    if (!fileReader->Read(filepath, sourceCode)) {
        SetError(std::string("Failed to read script: ") + fileReader->GetLastError());
        return false;
    }
    
    // 2. Lex
    KoiloScriptLexer lexer(sourceCode);
    std::vector<Token> tokens = lexer.Tokenize();
    
    if (lexer.HasError()) {
        SetError(std::string("Lexer error: ") + lexer.GetError());
        return false;
    }
    
    // 3. Parse
    KoiloScriptParser parser(tokens);
    if (!parser.Parse(activeCtx_->ast)) {
        SetError(std::string("Parser error: ") + parser.GetError());
        return false;
    }
    
    // 3.5. Process imports
    if (!activeCtx_->ast.imports.empty()) {
        std::set<std::string> importedFiles;
        importedFiles.insert(filepath);
        if (!ProcessImports(activeCtx_->ast, filepath, importedFiles)) {
            return false;
        }
    }
    
    // 4. Compile to bytecode
    BytecodeCompiler compiler;
    compiledScript_ = std::make_unique<CompiledScript>(compiler.Compile(activeCtx_->ast));
    compiledScript_->FixupAtomPointers();  // Re-point chunk atoms after move
    if (compiler.HasError()) {
        SetError(("Bytecode compilation failed: " + compiler.GetError()).c_str());
        compiledScript_.reset();
        return false;
    }
    vm_ = std::make_unique<BytecodeVM>(this);
    vm_->SetCompiledScript(compiledScript_.get());
    
    hasError = false;
    engineState_ = EngineState::ScriptLoaded;
    return true;
}

bool KoiloScriptEngine::ProcessImports(ScriptAST& ast, const std::string& basePath,
                                      std::set<std::string>& importedFiles) {
    // Resolve directory of the importing script
    std::string baseDir;
    size_t lastSlash = basePath.find_last_of("/\\");
    if (lastSlash != std::string::npos) {
        baseDir = basePath.substr(0, lastSlash + 1);
    }
    
    for (auto& importPath : ast.imports) {
        std::string resolvedPath = baseDir + importPath;
        
        // Circular import detection
        if (importedFiles.count(resolvedPath)) {
            continue; // Already imported, skip silently
        }
        importedFiles.insert(resolvedPath);
        
        // Read imported file
        std::string sourceCode;
        if (!fileReader->Read(resolvedPath.c_str(), sourceCode)) {
            SetError("Import failed: could not read '" + importPath + "' (resolved to '" + resolvedPath + "')");
            return false;
        }
        
        // Lex
        KoiloScriptLexer lexer(sourceCode);
        std::vector<Token> tokens = lexer.Tokenize();
        if (lexer.HasError()) {
            SetError("Import '" + importPath + "': lexer error: " + lexer.GetError());
            return false;
        }
        
        // Parse into a temporary AST
        ScriptAST importedAST;
        KoiloScriptParser parser(tokens);
        if (!parser.Parse(importedAST)) {
            SetError("Import '" + importPath + "': parser error: " + parser.GetError());
            return false;
        }
        
        // Recursively process imports in the imported file
        if (!importedAST.imports.empty()) {
            if (!ProcessImports(importedAST, resolvedPath, importedFiles)) {
                return false;
            }
        }
        
        // Merge imported functions into main AST
        for (auto& fn : importedAST.functions) {
            ast.functions.push_back(std::move(fn));
        }
        
        // Merge imported init statements (var decls, class decls) into main AST
        // Insert before existing init statements so imports are processed first
        for (int i = static_cast<int>(importedAST.initStatements.size()) - 1; i >= 0; --i) {
            ast.initStatements.insert(ast.initStatements.begin(),
                                      std::move(importedAST.initStatements[i]));
        }
    }
    
    return true;
}

bool KoiloScriptEngine::BuildScene() {
    if (engineState_ != EngineState::ScriptLoaded) {
        SetError("BuildScene() requires LoadScript() to be called first");
        engineState_ = EngineState::Error;
        return false;
    }
    if (hasError) return false;
    
    // Clear any previously attached canvases (reload safety)
    Canvas2D::DetachAll();
    
    // Register display config as global object
    RegisterGlobal("display", "DisplayConfig", &displayConfigObj_);
    
    // Register all user-defined functions
    for (auto& func : activeCtx_->ast.functions) {
        Value funcVal;
        funcVal.type = Value::Type::FUNCTION;
        funcVal.functionRef = func.get();
        activeCtx_->variables[func->name] = funcVal;
    }
    
    // Register static-only class globals and enum constants BEFORE init
    // so top-level code can use Math.Lerp(), Sine, Cosine, etc.
    RegisterStaticGlobals();
    
    // Execute top-level init statements (var declarations, assignments, constructors)
    if (compiledScript_ && vm_) {
        vm_->Execute(compiledScript_->mainChunk);
        if (vm_->HasError()) {
            SetError(vm_->GetError());
            return false;
        }
    }
    if (hasError) return false;
    
    // Read display config - init code or defaults may have set values
    displayConfig = displayConfigObj_.ToConfigMap();
    
    // Initialize modules so GetModule() works for BuildCamera/RegisterXGlobal
    if (kernel_) {
        moduleLoader_.InitializeAll(*kernel_);
    }
    
    // Build camera/scene from display dimensions (creates scene object)
    BuildCamera();
    
    // Register core globals (scene, input, debug, entities, world)
    RegisterSceneGlobal();
    RegisterInputGlobal();
    RegisterDebugGlobal();
    RegisterEntitiesGlobal();
    RegisterWorldGlobal();
    
    // Call fn Setup() if defined - loads assets, builds scene objects
    if (compiledScript_ && vm_) {
        if (compiledScript_->functions.count("Setup")) {
            vm_->CallFunction(*compiledScript_, "Setup");
            if (vm_->HasError()) {
                SetError(vm_->GetError());
                return false;
            }
        }
    }
    
    // Re-read display config in case Setup() changed it
    displayConfig = displayConfigObj_.ToConfigMap();
    
    if (!hasError) engineState_ = EngineState::SceneBuilt;
    sceneCounterBase_ = activeCtx_->tempCounter;

    // Expose subsystems through the kernel service registry
    RegisterEngineServices();

    // Mirror display config into kernel ConfigStore
    if (kernel_) {
        auto& cfg = kernel_->GetConfig();
        cfg.SetString("display.type", displayConfigObj_.GetType().c_str());
        cfg.SetInt("display.width",       displayConfigObj_.GetWidth());
        cfg.SetInt("display.height",      displayConfigObj_.GetHeight());
        cfg.SetInt("display.pixelWidth",  displayConfigObj_.GetPixelWidth());
        cfg.SetInt("display.pixelHeight", displayConfigObj_.GetPixelHeight());
        cfg.SetInt("display.brightness",  displayConfigObj_.GetBrightness());
        cfg.SetInt("display.targetFPS",   displayConfigObj_.GetTargetFPS());
        cfg.SetBool("display.capFPS",     displayConfigObj_.GetCapFPS());
    }

    return !hasError;
}

void KoiloScriptEngine::ExecuteUpdate() {
    KL_PROFILE_SCOPE("ScriptUpdate");
    if (hasError) return;
    if (engineState_ != EngineState::SceneBuilt && engineState_ != EngineState::Running) {
        SetError("ExecuteUpdate() requires BuildScene() to be called first");
        return;
    }
    engineState_ = EngineState::Running;
    
    float deltaTime = TimeManager::GetInstance().GetDeltaTime();
    
    activeCtx_->currentTime += deltaTime;
    
    // Set built-in activeCtx_->variables in global scope
    // Built-in time variables: cached Value* slots.
    // First call (or after rehash) does try_emplace + pointer fetch; steady
    // state is direct pointer write - no hashing, no string allocation.
    if (activeCtx_->builtinSlotsBucketCount != activeCtx_->variables.bucket_count()
        || !activeCtx_->builtinTimeSlot) {
        activeCtx_->builtinTimeSlot      = &activeCtx_->variables.try_emplace("time").first->second;
        activeCtx_->builtinDeltaTimeSlot = &activeCtx_->variables.try_emplace("deltaTime").first->second;
        activeCtx_->builtinFpsSlot       = &activeCtx_->variables.try_emplace("fps").first->second;
        activeCtx_->builtinSlotsBucketCount = activeCtx_->variables.bucket_count();
    }
    *activeCtx_->builtinTimeSlot      = Value(activeCtx_->currentTime);
    *activeCtx_->builtinDeltaTimeSlot = Value(deltaTime);
    *activeCtx_->builtinFpsSlot       = Value(TimeManager::GetInstance().GetFPS());
    
    // Input is now updated by InputModule via moduleLoader_.UpdateAll()
    
    // Update core subsystems (particles managed by ParticleModule)
    moduleLoader_.UpdateAll(deltaTime);

    // Update sky time-of-day
    Sky::GetInstance().Update(deltaTime);
    
    // Bytecode VM: call fn Update(dt)
    if (compiledScript_ && vm_) {
        vm_->ResetHeap(); // reclaim per-frame heap allocations
        if (compiledScript_->functions.count("Update")) {
            vm_->ClearError();
            vm_->CallFunction(*compiledScript_, "Update", {Value(deltaTime)});
            if (vm_->HasError()) {
                SetError(vm_->GetError());
            }
        }
    }
    
    // Resume active coroutines
    if (compiledScript_ && vm_) {
        // Resume suspended coroutines first (before starting new ones)
        coroutineManager_.ResumeAll();
        auto& active = coroutineManager_.Active();
        for (auto& co : active) {
            if (!co.finished && co.waitFrames == 0) {
                vm_->ClearError();
                vm_->ResumeCoroutine(co);
                if (vm_->HasError()) {
                    SetError(vm_->GetError());
                    break;
                }
            }
        }

        // Start newly queued coroutines (from this frame's Update)
        for (auto& funcName : coroutineManager_.PendingStarts()) {
            if (compiledScript_->functions.count(funcName)) {
                CoroutineState co;
                co.name = funcName;
                vm_->SetSuspendedState(&co);
                vm_->ClearYielded();
                vm_->CallFunction(*compiledScript_, funcName, {});
                if (vm_->HasYielded()) {
                    coroutineManager_.Active().push_back(std::move(co));
                }
                vm_->SetSuspendedState(nullptr);
            }
        }
        coroutineManager_.PendingStarts().clear();
    }

    // Clean up frame-scoped temp objects
    CleanupFrameTemps();
    activeCtx_->tempCounter = sceneCounterBase_;
}

void KoiloScriptEngine::CleanupFrameTemps() {
    for (auto& tempName : activeCtx_->frameTempObjects) {
        // Skip objects that were promoted to persistent
        if (activeCtx_->persistentObjects.count(tempName)) continue;
        auto it = activeCtx_->reflectedObjects.find(tempName);
        if (it != activeCtx_->reflectedObjects.end()) {
            if (it->second.ownsInstance && it->second.instance) {
                // Owning temps: destroy instance
                if (it->second.classDesc && it->second.classDesc->destroy) {
                    it->second.classDesc->destroy(it->second.instance);
                } else {
                    ::operator delete(it->second.instance);
                }
            }
            // Mark entry as dead but keep map node allocated (avoids per-frame alloc/free)
            it->second.instance = nullptr;
            it->second.classDesc = nullptr;
            it->second.ownsInstance = false;
        }
    }
    activeCtx_->frameTempObjects.clear();
}

void KoiloScriptEngine::PromoteToPersistent(const std::string& objectName) {
    activeCtx_->persistentObjects.insert(objectName);
}

void KoiloScriptEngine::CleanupPersistentObject(const std::string& objectName) {
    auto it = activeCtx_->reflectedObjects.find(objectName);
    if (it != activeCtx_->reflectedObjects.end() && it->second.ownsInstance) {
        if (it->second.classDesc && it->second.classDesc->destroy) {
            it->second.classDesc->destroy(it->second.instance);
        } else {
            ::operator delete(it->second.instance);
        }
        it->second.instance = nullptr;
        activeCtx_->reflectedObjects.erase(it);
        ++activeCtx_->reflectedObjectsGen;
    }
    activeCtx_->persistentObjects.erase(objectName);
}

void KoiloScriptEngine::CleanupAllPersistentObjects() {
    for (auto& name : activeCtx_->persistentObjects) {
        auto it = activeCtx_->reflectedObjects.find(name);
        if (it != activeCtx_->reflectedObjects.end() && it->second.ownsInstance) {
            if (it->second.classDesc && it->second.classDesc->destroy) {
                it->second.classDesc->destroy(it->second.instance);
            } else {
                ::operator delete(it->second.instance);
            }
            it->second.instance = nullptr;
            activeCtx_->reflectedObjects.erase(it);
            ++activeCtx_->reflectedObjectsGen;
        }
    }
    activeCtx_->persistentObjects.clear();
}

void KoiloScriptEngine::SetState(const char* stateName) {
    auto it = activeCtx_->states.find(stateName);
    if (it == activeCtx_->states.end()) {
        SetError(std::string("State not found: ") + stateName);
        return;
    }
    
    activeCtx_->currentState = stateName;
    
    // State statements require bytecode compilation (not yet implemented)
    // Legacy STATE blocks are only used in archived examples
}

std::vector<std::string> KoiloScriptEngine::GetObjectIDs() const {
    std::vector<std::string> ids;
    for (const auto& [id, obj] : objects) {
        ids.push_back(id);
    }
    return ids;
}

std::vector<std::string> KoiloScriptEngine::GetStateNames() const {
    std::vector<std::string> names;
    for (const auto& [name, stateNode] : activeCtx_->states) {
        names.push_back(name);
    }
    return names;
}

KoiloMeshLoader* KoiloScriptEngine::GetMesh(const char* name) {
    auto it = loadedMeshes.find(name);
    if (it != loadedMeshes.end()) {
        return it->second.get();
    }
    return nullptr;
}

bool KoiloScriptEngine::Reload() {
    // Save previous state for recovery on failure
    auto prevScript = std::move(compiledScript_);
    auto prevVM = std::move(vm_);
    EngineState prevState = engineState_;

    engineState_ = EngineState::Created;
    hasError = false;

    if (LoadScript(currentScriptPath.c_str()) && BuildScene()) {
        return true;
    }

    // Reload failed - restore previous working state
    std::string reloadError = hasError ? errorMessage : "Unknown reload error";
    compiledScript_ = std::move(prevScript);
    vm_ = std::move(prevVM);
    engineState_ = prevState;
    hasError = true;
    errorMessage = "Reload failed (previous script restored): " + reloadError;
    return false;
}

void KoiloScriptEngine::Reset() {
    compiledScript_.reset();
    vm_.reset();
    engineState_ = EngineState::Created;
    hasError = false;
    errorMessage.clear();
    currentScriptPath.clear();
}

std::string KoiloScriptEngine::ResolveAssetPath(const std::string& assetPath) const {
    // If path is absolute, return as-is
    if (!assetPath.empty() && assetPath[0] == '/') {
        return assetPath;
    }
    
    // Get directory of the script file
    size_t lastSlash = currentScriptPath.find_last_of("/\\");
    if (lastSlash == std::string::npos) {
        // Script is in current directory, asset path is relative to CWD
        return assetPath;
    }
    
    // Resolve asset path relative to script directory
    std::string scriptDir = currentScriptPath.substr(0, lastSlash + 1);
    return scriptDir + assetPath;
}

bool KoiloScriptEngine::HandleInput(const char* inputType, const char* inputId) {
    // Legacy: check CONTROLS block mappings first
    std::string key = std::string(inputType) + ":" + inputId;
    auto it = activeCtx_->controlMap.find(key);
    if (it != activeCtx_->controlMap.end()) {
        const ControlNode* ctrl = it->second;
        
        if (ctrl->action == "set_state" && !ctrl->actionArgs.empty()) {
            SetState(ctrl->actionArgs[0].c_str());
            return true;
        }
        if (ctrl->action == "quit") {
            return true;
        }
        if (ctrl->action == "blink") {
            return true;
        }
        if (ctrl->action == "call" && !ctrl->actionArgs.empty()) {
            if (compiledScript_ && vm_ && compiledScript_->functions.count(ctrl->actionArgs[0])) {
                std::vector<Value> callArgs;
                for (size_t i = 1; i < ctrl->actionArgs.size(); ++i) {
                    callArgs.push_back(Value(ctrl->actionArgs[i]));
                }
                vm_->CallFunction(*compiledScript_, ctrl->actionArgs[0], callArgs);
                if (vm_->HasError()) SetError(vm_->GetError());
            }
            return true;
        }
        
        std::vector<Value> args;
        for (auto& arg : ctrl->actionArgs) {
            args.push_back(Value(arg));
        }
        CallBuiltinFunction(ctrl->action, args);
        return true;
    }
    
    // Code-first: call OnKeyPress/OnKeyRelease(key) or OnPinInput(pin) if defined
    std::string funcName;
    if (std::string(inputType) == "key") funcName = "OnKeyPress";
    else if (std::string(inputType) == "key_release") funcName = "OnKeyRelease";
    else funcName = "OnPinInput";

    // Bytecode VM path
    if (compiledScript_ && vm_) {
        if (compiledScript_->functions.count(funcName)) {
            vm_->CallFunction(*compiledScript_, funcName, {Value(std::string(inputId))});
            if (vm_->HasError()) SetError(vm_->GetError());
            return true;
        }
    }
    
    return false;
}

Value KoiloScriptEngine::ConstructObject(const ClassDesc* classDesc, const std::vector<Value>& args) {
    if (!classDesc) return Value();
    
    // Build per-arg ClassDesc array for type-aware matching (stack buffer)
    const ClassDesc* argDescsBuf[8] = {};
    const ClassDesc** argDescs = argDescsBuf;
    std::vector<const ClassDesc*> argDescsHeap;
    if (args.size() > 8) {
        argDescsHeap.resize(args.size(), nullptr);
        argDescs = argDescsHeap.data();
    }
    for (size_t i = 0; i < args.size(); ++i) {
        if (args[i].type == Value::Type::OBJECT && !args[i].objectName.empty()) {
            auto it = activeCtx_->reflectedObjects.find(args[i].objectName);
            if (it != activeCtx_->reflectedObjects.end()) {
                argDescs[i] = it->second.classDesc;
            }
        }
    }
    
    // Type-aware constructor matching: score each candidate by argument type compatibility
    const ConstructorDesc* matchedCtor = nullptr;
    int bestScore = -1;
    
    for (size_t i = 0; i < classDesc->ctor_count; ++i) {
        const ConstructorDesc& ctor = classDesc->ctors[i];
        if (ctor.arg_types.count != args.size()) continue;
        
        int score = 0;
        bool compatible = true;
        for (size_t a = 0; a < args.size(); ++a) {
            if (argDescs[a] && ctor.arg_types.data[a]) {
                const ClassDesc* paramClass = ClassForType(*ctor.arg_types.data[a]);
                if (paramClass && argDescs[a]) {
                    if (std::strcmp(argDescs[a]->name, paramClass->name) == 0) {
                        score += 2;  // exact type match
                    } else {
                        compatible = false;
                        break;
                    }
                }
            }
        }
        if (compatible && score > bestScore) {
            bestScore = score;
            matchedCtor = &ctor;
        }
    }
    
    // Fallback: first constructor matching arg count (for cases with no type info)
    if (!matchedCtor) {
        for (size_t i = 0; i < classDesc->ctor_count; ++i) {
            const ConstructorDesc& ctor = classDesc->ctors[i];
            if (ctor.arg_types.count == args.size()) {
                matchedCtor = &ctor;
                break;
            }
        }
    }
    
    // Try range-based match (min_argc)
    if (!matchedCtor) {
        for (size_t i = 0; i < classDesc->ctor_count; ++i) {
            const ConstructorDesc& ctor = classDesc->ctors[i];
            size_t minA = ctor.min_argc ? ctor.min_argc : ctor.arg_types.count;
            if (args.size() >= minA && args.size() < ctor.arg_types.count) {
                matchedCtor = &ctor;
                break;
            }
        }
    }
    
    if (!matchedCtor) {
        SetError(std::string("No constructor for ") + classDesc->name + " with " + std::to_string(args.size()) + " args");
        return Value();
    }
    
    // Marshal Value args to typed C++ args, zero-filling defaults
    static ArgMarshaller marshaller;
    marshaller.Clear();
    size_t totalArgc = matchedCtor->arg_types.count;
    
    // Use stack buffer for arg pointers when possible
    void* argPtrsBuf[8] = {};
    void** argPtrsPtr = argPtrsBuf;
    std::vector<void*> argPtrsHeap;

    // Handle zero-fill for default args
    const std::vector<Value>* argsToMarshal = &args;
    std::vector<Value> extendedArgs;
    if (args.size() < totalArgc) {
        extendedArgs = args;
        for (size_t i = extendedArgs.size(); i < totalArgc; ++i) {
            extendedArgs.push_back(Value(0.0f));
        }
        argsToMarshal = &extendedArgs;
    }
    
    if (totalArgc > 0) {
        if (totalArgc > 8) {
            argPtrsHeap.resize(totalArgc);
            argPtrsPtr = argPtrsHeap.data();
        }
        for (size_t i = 0; i < totalArgc; ++i) {
            argPtrsPtr[i] = marshaller.Marshal((*argsToMarshal)[i],
                                                matchedCtor->arg_types.data[i],
                                                activeCtx_->reflectedObjects);
            if (!argPtrsPtr[i]) {
                SetError(std::string("Failed to marshal constructor args for ") + classDesc->name);
                return Value();
            }
        }
    }
    
    void* instance = matchedCtor->invoker(totalArgc > 0 ? argPtrsPtr : nullptr);
    if (!instance) {
        SetError(std::string("Constructor failed for ") + classDesc->name);
        return Value();
    }
    
    // Register as temp reflected object - use snprintf to avoid string concat allocations
    char nameBuf[64];
    std::snprintf(nameBuf, sizeof(nameBuf), "_ctor_%s_%u", classDesc->name, activeCtx_->tempCounter++);
    // Skip names still held by persistent objects to avoid corrupting live references
    while (activeCtx_->persistentObjects.count(nameBuf)) {
        std::snprintf(nameBuf, sizeof(nameBuf), "_ctor_%s_%u", classDesc->name, activeCtx_->tempCounter++);
    }
    std::string tempName(nameBuf);
    // Destroy old owned instance before overwriting (prevents leak with tempCounter recycling)
    auto existIt = activeCtx_->reflectedObjects.find(tempName);
    if (existIt != activeCtx_->reflectedObjects.end() &&
        existIt->second.ownsInstance && existIt->second.instance) {
        if (existIt->second.classDesc && existIt->second.classDesc->destroy) {
            existIt->second.classDesc->destroy(existIt->second.instance);
        } else {
            ::operator delete(existIt->second.instance);
        }
    }
    activeCtx_->reflectedObjects[tempName] = ReflectedObject(instance, classDesc, tempName, true);
    activeCtx_->frameTempObjects.push_back(tempName);
    
    return Value::Object(tempName);
}

// ============================================================================
// Scope Management
// ============================================================================

void KoiloScriptEngine::PushScope() {
    activeCtx_->scopeStack.push_back({});
}

void KoiloScriptEngine::PopScope() {
    if (!activeCtx_->scopeStack.empty()) {
        // Clean up persistent objects owned by activeCtx_->variables leaving scope
        for (auto& [name, val] : activeCtx_->scopeStack.back()) {
            if (val.type == Value::Type::OBJECT && !val.objectName.empty()
                && activeCtx_->persistentObjects.count(val.objectName)) {
                CleanupPersistentObject(val.objectName);
            }
        }
        activeCtx_->scopeStack.pop_back();
    }
}

Value KoiloScriptEngine::GetVariable(const std::string& name) const {
    // Handle 'self' keyword - resolves to active context's bound object
    if (name == "self") {
        ScriptContext& actx = const_cast<KoiloScriptEngine*>(this)->ActiveCtx();
        if (actx.selfObject && actx.selfDesc) {
            return Value::Object("self");
        }
        return Value();
    }
    if (const Value* p = GetVariablePtr(name)) return *p;
    return Value();
}

const Value* KoiloScriptEngine::GetVariablePtr(const std::string& name) const {
    // Search scope stack from innermost to outermost (skipped when empty)
    auto& scopes = activeCtx_->scopeStack;
    for (size_t i = scopes.size(); i-- > 0; ) {
        auto& m = scopes[i];
        auto it = m.find(name);
        if (it != m.end()) return &it->second;
    }
    // Fall back to active-context globals
    auto it = activeCtx_->variables.find(name);
    if (it != activeCtx_->variables.end()) return &it->second;

    // If in an object context, fall back to primary context globals (shared globals)
    if (activeCtx_ != &ctx_) {
        auto pit = ctx_.variables.find(name);
        if (pit != ctx_.variables.end()) return &pit->second;
    }
    return nullptr;
}

void KoiloScriptEngine::SetVariable(const std::string& name, const Value& value) {
    // If assigning a constructed object, promote it to persistent
    if (value.type == Value::Type::OBJECT && !value.objectName.empty()) {
        PromoteToPersistent(value.objectName);
    }


    // Find existing variable in scope stack (innermost-first).
    auto& scopes = activeCtx_->scopeStack;
    for (size_t i = scopes.size(); i-- > 0;) {
        auto& scope = scopes[i];
        if (scope.empty()) continue;
        auto it = scope.find(name);
        if (it != scope.end()) {
            // Clean up old persistent object if being overwritten
            if (it->second.type == Value::Type::OBJECT && !it->second.objectName.empty()
                && it->second.objectName != value.objectName
                && activeCtx_->persistentObjects.count(it->second.objectName)) {
                CleanupPersistentObject(it->second.objectName);
            }
            it->second = value;
            return;
        }
    }
    // Fall back to global activeCtx_->variables. Do a single lookup
    // and dispatch to assign-or-emplace to avoid the duplicate hash
    // performed by operator[] on the existing-key path (the common case).
    auto& vars = activeCtx_->variables;
    auto git = vars.find(name);
    if (git != vars.end()) {
        if (git->second.type == Value::Type::OBJECT && !git->second.objectName.empty()
            && git->second.objectName != value.objectName
            && activeCtx_->persistentObjects.count(git->second.objectName)) {
            CleanupPersistentObject(git->second.objectName);
        }
        git->second = value;
    } else {
        vars.emplace(name, value);
    }
}

Value KoiloScriptEngine::GetScriptVariable(const std::string& name) const {
    return GetVariable(name);
}

void KoiloScriptEngine::SetScriptVariable(const std::string& name, const Value& value) {
    SetVariable(name, value);
}

void KoiloScriptEngine::SetScriptVariable(const std::string& name, double value) {
    SetVariable(name, Value(value));
}

double KoiloScriptEngine::GetScriptVariableNum(const std::string& name) const {
    Value v = GetVariable(name);
    return (v.type == Value::Type::NUMBER) ? v.numberValue : 0.0;
}

void KoiloScriptEngine::SetRenderBackend(std::unique_ptr<IRenderBackend> backend) {
    auto* mod = dynamic_cast<RenderModule*>(moduleLoader_.GetModule("render"));
    if (mod) {
        mod->SetBackend(std::move(backend));
    }
}

void KoiloScriptEngine::DeclareVariable(const std::string& name, const Value& value) {
    // If declaring with a constructed object, promote it to persistent
    if (value.type == Value::Type::OBJECT && !value.objectName.empty()) {
        PromoteToPersistent(value.objectName);
    }
    
    if (!activeCtx_->scopeStack.empty()) {
        activeCtx_->scopeStack.back()[name] = value;
    } else {
        activeCtx_->variables[name] = value;
    }
}

std::string KoiloScriptEngine::FlattenMemberAccessToPath(ExpressionNode* expr) {
    if (!expr) return "";
    switch (expr->type) {
        case ExpressionNode::Type::IDENTIFIER:
            return expr->value;
        case ExpressionNode::Type::MEMBER_ACCESS:
            if (expr->left) {
                return FlattenMemberAccessToPath(expr->left.get()) + "." + expr->value;
            }
            return expr->value;
        default:
            return "";
    }
}

void KoiloScriptEngine::SetError(const std::string& message) {
    hasError = true;
    if (currentLine_ > 0) {
        errorMessage = "Line " + std::to_string(currentLine_) + ": " + message;
    } else {
        errorMessage = message;
    }
    engineState_ = EngineState::Error;
}


// --- High-level convenience API ---

DisplayInfo KoiloScriptEngine::GetDisplayInfo() const {
    DisplayInfo info;
    auto get = [&](const char* key, int fallback) -> int {
        auto it = displayConfig.find(key);
        return (it != displayConfig.end()) ? std::stoi(it->second) : fallback;
    };
    info.width       = get("width", 1280);
    info.height      = get("height", 720);
    info.pixelWidth  = get("pixel_width", 192);
    info.pixelHeight = get("pixel_height", 94);
    info.brightness  = get("brightness", 255);
    auto fpsIt = displayConfig.find("target_fps");
    info.targetFPS = (fpsIt != displayConfig.end()) ? std::stof(fpsIt->second) : 60.0f;
    info.capFPS = (displayConfig.find("cap_fps") != displayConfig.end());
    return info;
}

void KoiloScriptEngine::RenderFrame(Color888* buffer, int width, int height) {
    KL_PROFILE_SCOPE("RenderFrame");
    if (!buffer) return;

    // Clear to black - scene rendering now goes through FrameComposer +
    // SoftwareRHIDevice, so this legacy path only handles overlays.
    std::memset(static_cast<void*>(buffer), 0, width * height * sizeof(Color888));

    auto* camera = GetCamera();

    // Debug draw overlay (grid, axes, lines, etc.) into output buffer
    {
        KL_PROFILE_SCOPE("DebugRender");
        auto& dd = DebugDraw::GetInstance();
        if (dd.IsEnabled()) {
            for (const auto& line : dd.GetLines()) {
                const Color& c = line.color;
                auto clamp255 = [](float v) -> uint8_t {
                    return v < 0.f ? 0 : v > 255.f ? 255 : static_cast<uint8_t>(v);
                };
                Rasterizer::DrawLine3D(
                    line.start, line.end,
                    Color888(clamp255(c.r * 255.0f),
                             clamp255(c.g * 255.0f),
                             clamp255(c.b * 255.0f)),
                    line.depthTest, buffer, width, height);
            }
            DebugRenderer::Render(dd, camera, buffer, width, height,
                                  nullptr, 0, 0,
                                  /*renderLines=*/false);
            // Expiry is now handled centrally in KoiloKernel::EndFrame so
            // that backends not using this software path also get it.
        }
    }

    // Composite user-created canvases
    Canvas2D::CompositeAll(buffer, width, height);

    // Render subsystems via modules (particles=Render, UI=Overlay phases)
    { KL_PROFILE_SCOPE("Subsystems");
      moduleLoader_.RenderAll(buffer, width, height);
    }
}

void KoiloScriptEngine::UpdateUIAnimations(float dt) {
    auto* mod = dynamic_cast<UIModule*>(moduleLoader_.GetModule("ui"));
    if (mod) mod->UpdateAnimations(dt);
}

} // namespace scripting
} // namespace koilo
