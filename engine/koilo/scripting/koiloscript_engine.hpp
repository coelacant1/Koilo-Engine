// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <koilo/core/interfaces/iscript_bridge.hpp>
#include <koilo/platform/iscriptfilereader.hpp>
#include <koilo/scripting/koiloscript_lexer.hpp>
#include <koilo/scripting/koiloscript_parser.hpp>
#include <koilo/scripting/koiloscript_ast.hpp>
#include <koilo/scripting/reflection_bridge.hpp>
#include <koilo/scripting/display_config.hpp>
#include <koilo/scripting/script_context.hpp>
#include <koilo/scripting/bytecode.hpp>
#include <koilo/scripting/bytecode_vm.hpp>
#include <koilo/scripting/signal_registry.hpp>
#include <koilo/assets/koilomesh_loader.hpp>
#include <koilo/debug/debugdraw.hpp>
#include <koilo/systems/ecs/script_entity_manager.hpp>
#include <koilo/systems/world/script_world_manager.hpp>
#include <koilo/kernel/module_loader.hpp>
#include <koilo/scripting/coroutine.hpp>
#include <string>
#include <map>
#include <unordered_map>
#include <set>
#include <memory>
#include <vector>

namespace koilo {

class MorphableMesh;  // Forward declaration for automatic scene composition
class PhysicsWorld;
class UI;
class ParticleSystem;
class ScriptAIManager;
class ScriptAudioManager;
class Sky;
class KoiloKernel;
class IRenderBackend;
class InputManager;
class PixelGroup;
class Camera;
class Scene;

namespace scripting {

// Type-safe display configuration extracted from script's display block.
struct DisplayInfo {
    int width = 1280;           ///< Window width in pixels
    int height = 720;           ///< Window height in pixels
    int pixelWidth = 192;       ///< Render target width
    int pixelHeight = 94;       ///< Render target height
    float targetFPS = 60.0f;    ///< Target frame rate
    bool capFPS = false;        ///< Whether to enforce frame rate cap
    int brightness = 255;       ///< LED brightness (0-255)
};

/**
 * @brief KoiloScript execution engine
 * 
 * Main engine that:
 * 1. Loads scripts from files (platform-agnostic)
 * 2. Builds C++ scene from AST
 * 3. Executes UPDATE block every frame
 * 4. Provides built-in functions (sin, cos, lerp, etc.)
 * 5. Manages states
 * 
 * Usage:
 *   KoiloScriptEngine engine(fileReader);
 *   if (engine.LoadScript("animation.ks")) {
 *       engine.BuildScene();
 *       
 *       while (running) {
 *           engine.ExecuteUpdate();
 *       }
 *   }
 */
class KoiloScriptEngine : public IScriptBridge {
public:
    // Engine lifecycle state - enforces correct call order.
    enum class EngineState { Created, ScriptLoaded, SceneBuilt, Running, Error };

    /**
     * @brief Scene object data (for objects without className)
     */
    struct ObjectData {
        std::string id;
        std::string modelRef;
        std::unordered_map<std::string, Value> properties;
    };
    
    /**
     * @brief Construct engine with platform-specific file reader
     * @param fileReader Platform file reader (desktop or SD card)
     * @param loadDefaultModules If false, no modules are registered (minimal core-only mode)
     */
    explicit KoiloScriptEngine(platform::IScriptFileReader* fileReader, bool loadDefaultModules = true);
    ~KoiloScriptEngine();

    /// Set the kernel reference. Must be called before BuildScene().
    void SetKernel(KoiloKernel* kernel) { kernel_ = kernel; }
    KoiloKernel* GetKernel() const { return kernel_; }
    
    /**
     * @brief Load script from file
     * @param filepath Path to .ks script file
     * @return true if loaded and parsed successfully
     */
    bool LoadScript(const char* filepath);
    
    /**
     * @brief Build scene from parsed AST
     * 
     * Instantiates C++ objects based on script.
     * Must be called after LoadScript() and before ExecuteUpdate().
     * 
     * @return true if scene built successfully
     */
    bool BuildScene();
    
    /**
     * @brief Execute UPDATE block
     * 
     * Runs all statements in UPDATE block.
     * Call this every frame.
     * 
     * Pulls delta time from TimeManager.
     */
    void ExecuteUpdate();
    
    /**
     * @brief Set current state
     * @param stateName Name of state to activate
     */
    void SetState(const char* stateName);
    
    /**
     * @brief Check if engine has errors
     */
    bool HasError() const { return hasError; }
    
    /**
     * @brief Get error message
     */
    const char* GetError() const { return errorMessage.c_str(); }
    
    /**
     * @brief Get engine lifecycle state.
     */
    EngineState GetState() const { return engineState_; }
    
    /**
     * @brief Get display configuration
     */
    const std::map<std::string, std::string>& GetDisplayConfig() const { return displayConfig; }
    
    /**
     * @brief Get type-safe display info (populated after BuildScene).
     */
    DisplayInfo GetDisplayInfo() const;
    
    /**
     * @brief Render a complete frame: clear -> rasterize -> effects -> particles -> UI.
     * @param buffer Output pixel buffer (pixelWidth × pixelHeight Color888 entries)
     * @param width  Render target width
     * @param height Render target height
     *
     * Replaces the 5-step manual render pipeline. Each subsystem is skipped
     * if not active (e.g., no particles configured = no particle render).
     * Individual system accessors remain available for custom pipelines.
     */
    void RenderFrame(Color888* buffer, int width, int height);

    /** @brief GPU-only render: draws to the backend FBO without ReadPixels.
     *  Use with OpenGLRenderBackend::BlitToScreen() for zero-copy present. */
    void RenderFrameGPU();

    /** @brief Render UI overlay via GPU. Call after scene render, before swap.
     *  Backend-agnostic: auto-initializes GPU renderer on first call. */
    void RenderUIOverlay(int viewportW, int viewportH, float dt);

    /** @brief Update UI animations (software path, called separately). */
    void UpdateUIAnimations(float dt);
    
    /**
     * @brief Get loaded assets (legacy - returns empty in code-first mode)
     */
    const std::map<std::string, std::string>& GetAssets() const {
        static const std::map<std::string, std::string> empty;
        return empty;
    }
    
    /**
     * @brief Get loaded mesh by name
     * @param name Asset name from ASSETS block
     * @return Pointer to loaded mesh, or nullptr if not found
     */
    KoiloMeshLoader* GetMesh(const char* name);
    
    /**
     * @brief Get reflected object by name
     * @param name Object ID from SCENE block
     * @return Pointer to the C++ object instance, or nullptr if not found
     * 
     * Example:
     * @code
     * Light* light = engine.GetReflectedObject<Light>("mainLight");
     * @endcode
     */
    template<typename T>
    T* GetReflectedObject(const char* name) {
        auto it = ctx_.reflectedObjects.find(name);
        if (it == ctx_.reflectedObjects.end()) return nullptr;
        return static_cast<T*>(it->second.instance);
    }
    
    /**
     * @brief Get reflected object metadata
     * @param name Object ID from SCENE block
     * @return Pointer to ReflectedObject wrapper, or nullptr if not found
     */
    const ReflectedObject* GetReflectedObjectInfo(const char* name) const {
        auto it = ctx_.reflectedObjects.find(name);
        if (it == ctx_.reflectedObjects.end()) return nullptr;
        return &it->second;
    }
    
    /**
     * @brief Get object IDs
     */
    std::vector<std::string> GetObjectIDs() const;
    
    /**
     * @brief Get scene object data by ID
     * @param id Object ID from SCENE block
     * @return Pointer to ObjectData, or nullptr if not found
     */
    const ObjectData* GetObjectData(const char* id) const {
        auto it = objects.find(id);
        if (it == objects.end()) return nullptr;
        return &it->second;
    }
    
    /**
     * @brief Get available state names
     */
    std::vector<std::string> GetStateNames() const;
    
    /**
     * @brief Get current state name
     */
    const std::string& GetCurrentState() const { return ctx_.currentState; }
    
    /**
     * @brief Get the scene created from SCENE block camera/objects
     * @return Scene pointer, or nullptr if no camera defined in script
     */
    Scene* GetScene() const;
    
    /**
     * @brief Get the physics world (via physics module)
     * @return PhysicsWorld pointer, or nullptr if not loaded
     */
    PhysicsWorld* GetPhysicsWorld();
    
    InputManager* GetInputManager();
    UI* GetUI();
    ParticleSystem* GetParticleSystem();
    ScriptEntityManager* GetEntities() { return &scriptEntities_; }
    ScriptAIManager* GetAI();
    ScriptAudioManager* GetAudio();
    ScriptWorldManager* GetWorld() { return &scriptWorld_; }
    DebugDraw& GetDebugDraw() { return DebugDraw::GetInstance(); }
    
    // Access the module loader for external ELF modules
    ModuleLoader& GetModuleLoader() { return moduleLoader_; }

    // Access the coroutine manager
    CoroutineManager& GetCoroutineManager() { return coroutineManager_; }
    
    /**
     * @brief Get the camera created from SCENE block
     * @return Camera pointer, or nullptr if no camera defined in script
     */
    Camera* GetCamera() const;
    
    /**
     * @brief Get the pixel group created from DISPLAY dimensions
     * @return PixelGroup pointer, or nullptr if no camera defined in script
     */
    PixelGroup* GetPixelGroup() const;
    
    /**
     * @brief Get a state property value (e.g., color from state block)
     * @param stateName State name
     * @param propertyName Property name (e.g., "color")
     * @return Property value, or Value::NONE if not found
     */
    Value GetStateProperty(const char* /*stateName*/, const char* /*propertyName*/) { return Value(); }
    
    /**
     * @brief Get material instance by name
     * @param name Material name from ASSETS block
     * @return Pointer to material object, or nullptr
     */
    void* GetMaterialInstance(const char* name) const {
        auto it = materialInstances_.find(name);
        if (it != materialInstances_.end()) return it->second;
        return nullptr;
    }
    
    /**
     * @brief Handle input event from host
     * @param inputType "key" or "pin"
     * @param inputId Key/pin identifier (e.g., "1", "b", "q")
     * @return true if a matching control was found and executed
     */
    bool HandleInput(const char* inputType, const char* inputId);
    
    /**
     * @brief Get control mappings for host to display or process
     */
    const std::vector<std::unique_ptr<ControlNode>>& GetControls() const { return ctx_.ast.controls; }
    
    /**
     * @brief Get transition animation config
     */
    const std::vector<std::unique_ptr<TransitionNode>>& GetTransitions() const { return ctx_.ast.transitions; }
    
    /**
     * @brief Get auto-animation config
     */
    const std::vector<std::unique_ptr<AutoAnimNode>>& GetAutoAnims() const { return ctx_.ast.autoAnims; }
    
    /**
     * @brief Register a C++ object with the script engine
     * @param id Identifier for the object (e.g., "face", "camera")
     * @param instance Pointer to the C++ object
     * @param classDesc Reflection metadata for the object's class
     * 
     * This allows scripts to access C++ objects via assignment statements.
     * Example: After registering a MorphableMesh as "face", scripts can do:
     *   face.SetMorphWeight("Anger", 1.0)
     */
    void RegisterObject(const char* id, void* instance, const ClassDesc* classDesc) {
        ctx_.reflectedObjects[id] = ReflectedObject(instance, classDesc, id, false);
    }
    
    /**
     * @brief Hot-reload script (desktop only)
     * 
     * Reloads script from file and rebuilds scene.
     * Useful for live editing.
     * 
     * @return true if reloaded successfully
     */
    bool Reload();
    
    /**
     * @brief Set a global script variable from host code
     * @param name Variable name
     * @param value Value to set
     */
    void SetGlobal(const std::string& name, const Value& value) {
        ctx_.variables[name] = value;
    }
    
    /**
     * @brief Get a global script variable from host code
     * @param name Variable name
     * @return Variable value, or NONE if not found
     */
    Value GetGlobal(const std::string& name) const {
        auto it = ctx_.variables.find(name);
        return (it != ctx_.variables.end()) ? it->second : Value();
    }
    
    // Access signal registry for connect/disconnect/emit
    SignalRegistry& GetSignalRegistry() { return signalRegistry_; }
    
    // Set render backend (default: SoftwareRenderBackend)
    void SetRenderBackend(std::unique_ptr<IRenderBackend> backend);
    
    // Get current render backend (via RenderModule)
    IRenderBackend* GetRenderBackend();
    
    // Register an object as a script-accessible global (used by modules)
    void RegisterGlobal(const char* name, const char* className, void* instance) override;
    
    /**
     * @brief Call a script function by name from host code
     * @param name Function name (e.g., "setAngry", "onKeyPress")
     * @param args Arguments to pass to the function
     * @return Return value from the function, or NONE if function doesn't exist
     * @note Returns NONE silently if function is not defined - this is intentional
     *       since optional callbacks (OnKeyPress, etc.) may not be present.
     */
    Value CallFunction(const std::string& name, const std::vector<Value>& args = {}) {
        if (compiledScript_ && vm_) {
            if (compiledScript_->functions.count(name)) {
                vm_->ClearError();
                Value result = vm_->CallFunction(*compiledScript_, name, args);
                if (vm_->HasError()) SetError(vm_->GetError());
                return result;
            }
        }
        return Value();
    }
    
    /**
     * @brief Get the active script context (primary context in single-file mode)
     */
    ScriptContext& GetContext() { return ctx_; }
    const ScriptContext& GetContext() const { return ctx_; }
    
    /**
     * @brief Get number of active script contexts (1 = single-file mode)
     */
    std::size_t GetContextCount() const { return 1 + objectContexts_.size(); }
    
    // Get a script variable value by name (for C ABI module access).
    Value GetScriptVariable(const std::string& name) const;
    
    // Set a script variable by name (for C ABI module access).
    void SetScriptVariable(const std::string& name, const Value& value);

    // IScriptBridge overrides (thin wrappers around Value-based API)
    void SetScriptVariable(const std::string& name, double value) override;
    double GetScriptVariableNum(const std::string& name) const override;
    
private:
    friend class BytecodeVM;
    
    // Import processing
    bool ProcessImports(ScriptAST& ast, const std::string& basePath,
                        std::set<std::string>& importedFiles);
    
    // --- Engine-global state ---
    platform::IScriptFileReader* fileReader;
    std::string currentScriptPath;
    bool hasError;
    std::string errorMessage;
    EngineState engineState_ = EngineState::Created;
    int currentLine_ = 0;
    
    // Primary script context (all per-script state lives here)
    ScriptContext ctx_;
    
    // Object script contexts (multi-context mode)
    std::vector<std::unique_ptr<ScriptContext>> objectContexts_;
    ScriptContext* activeCtx_ = nullptr;  // Currently executing context (nullptr = primary)
    
    // Display configuration
    std::map<std::string, std::string> displayConfig;
    DisplayConfig displayConfigObj_;
    
    // Asset cache (shared across all contexts)
    std::map<std::string, std::shared_ptr<KoiloMeshLoader>> loadedMeshes;
    
    // Scene data (legacy ObjectData for backward compatibility)
    std::map<std::string, ObjectData> objects;
    
    // Rendering objects created from script (scene/camera now in SceneModule)
    ScriptEntityManager scriptEntities_;
    ScriptWorldManager scriptWorld_;
    std::map<std::string, void*> materialInstances_;
    std::vector<MorphableMesh*> ownedMeshes_;
    
    int sceneCounterBase_ = 0;

    // External module loader (for user/third-party ELF modules)
    ModuleLoader moduleLoader_;
    KoiloKernel* kernel_ = nullptr;
    CoroutineManager coroutineManager_;
    
    // Bytecode VM
    std::unique_ptr<CompiledScript> compiledScript_;
    std::unique_ptr<BytecodeVM> vm_;
    SignalRegistry signalRegistry_;
    
    // Member access path cache struct (used by setter/getter paths)
    struct SplitPath {
        std::string baseName;
        std::vector<std::string> fieldChain;  // remaining dot-separated parts
    };

    // Member access
    Value EvaluateMemberAccess(const std::string& path);
    std::string FlattenMemberAccessToPath(ExpressionNode* expr);
    void SetMemberValue(const std::string& path, const Value& value);
    void SetReflectedMemberValue(const std::string& objectId, const SplitPath& sp, const Value& value);
    
    // Built-in functions
    Value CallBuiltinFunction(const std::string& name, const std::vector<Value>& args);
    Value CallReflectedMethod(const std::string& path, const std::vector<Value>& args);
    Value CallReflectedMethodDirect(void* instance, const ClassDesc* classDesc,
                                     const std::string& methodName, int argc,
                                     const std::vector<Value>& args);
    
    // Built-in type method dispatch (arrays and tables)
    Value DispatchArrayMethod(Value* arrPtr, const std::string& methodName, const std::vector<Value>& args);
    Value DispatchTableMethod(std::unordered_map<std::string, Value>* table, const std::string& methodName, const std::vector<Value>& args);
    Value ConstructObject(const ClassDesc* classDesc, const std::vector<Value>& args);
    
    // Scope management
    void PushScope();
    void PopScope();
    Value GetVariable(const std::string& name) const;
    void SetVariable(const std::string& name, const Value& value);
    void DeclareVariable(const std::string& name, const Value& value);
    
    // Constants
    static constexpr int MAX_CALL_DEPTH = 64;
    static constexpr int MAX_LOOP_ITERATIONS = 10000;
    
    // Temp object lifecycle
    void CleanupFrameTemps();
    
    // Persistent object lifecycle
    void PromoteToPersistent(const std::string& objectName);
    void CleanupPersistentObject(const std::string& objectName);
    void CleanupAllPersistentObjects();
    
    // Scene building
    void BuildCamera();
    void RegisterSceneGlobal();
    void RegisterInputGlobal();
    void RegisterDebugGlobal();
    void RegisterEntitiesGlobal();
    void RegisterWorldGlobal();
    void RegisterStaticGlobals();
    void RegisterDefaultModules();
    void RegisterEngineServices();
    void UnregisterEngineServices();
    
    // Path resolution
    std::string ResolveAssetPath(const std::string& assetPath) const;
    
    // Member access path cache
    std::unordered_map<std::string, SplitPath> pathCache_;
    const SplitPath& SplitMemberPath(const std::string& path);
    
    // Error handling
    void SetError(const std::string& message);
    
    // Active context helper (returns activeCtx_ if set, else &ctx_)
    ScriptContext& ActiveCtx() { return activeCtx_ ? *activeCtx_ : ctx_; }
};

} // namespace scripting
} // namespace koilo
