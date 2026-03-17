// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file koiloapplication.hpp
 * @brief Defines the KoiloApplication class for managing KoiloScript-based applications.
 *
 * This class provides the main application loop for KoiloScript-powered applications,
 * managing script loading, scene building, updates, and hot-reload functionality.
 *
 * @date 2026-02-15
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <koilo/platform/iscriptfilereader.hpp>
#include <koilo/scripting/koiloscript_engine.hpp>
#include <koilo/kernel/kernel.hpp>
#include <koilo/systems/scene/scene.hpp>
#include <koilo/systems/scene/mesh.hpp>
#include <string>
#include <map>
#include "registry/reflect_macros.hpp"

namespace koilo {

/**
 * @class KoiloApplication
 * @brief Main application class for KoiloScript-based applications.
 *
 * Manages the complete lifecycle of a KoiloScript application from loading
 * scripts to executing update loops and managing the scene. Owns a
 * KoiloKernel instance that provides message bus, service registry, and
 * memory allocators to all modules.
 */
class KoiloApplication {
private:
    KoiloKernel kernel_;
    platform::IScriptFileReader* fileReader;
    scripting::KoiloScriptEngine* engine;
    Scene* scene;
    
    std::string scriptPath;
    bool hotReloadEnabled;
    long lastModifiedTime;
    
    // Object ID to Mesh mapping
    std::map<std::string, Mesh*> meshMap;
    
    /**
     * @brief Gets the last modification time of the script file.
     * @return Last modification timestamp, or 0 if file doesn't exist.
     */
    long GetFileModificationTime();
    
    /**
     * @brief Clears all meshes from the scene and mesh map.
     */
    void ClearScene();

public:
    /**
     * @brief Constructs a KoiloApplication.
     */
    KoiloApplication();
    
    /**
     * @brief Destructor.
     */
    ~KoiloApplication();
    
    /**
     * @brief Loads a KoiloScript file and builds the scene.
     * @param filepath Path to the .ks script file.
     * @return True if successful, false on error.
     */
    bool LoadScript(const char* filepath);
    
    /**
     * @brief Updates the application state.
     * 
     * Executes the UPDATE block from the script and checks for
     * script file changes if hot reload is enabled.
     * 
     * @param deltaTime Time elapsed since last update (seconds).
     */
    void Update(float deltaTime);
    
    /**
     * @brief Enables or disables hot reload functionality.
     * 
     * When enabled, the application will check for script file changes
     * during Update() and automatically reload.
     * 
     * @param enable True to enable hot reload, false to disable.
     */
    void EnableHotReload(bool enable);
    
    /**
     * @brief Gets the managed scene.
     * @return Pointer to the Scene object.
     */
    Scene* GetScene();
    
    /**
     * @brief Gets the script engine.
     * @return Pointer to the KoiloScriptEngine.
     */
    scripting::KoiloScriptEngine* GetEngine();

    /// Access the kernel (message bus, services, allocators).
    KoiloKernel& GetKernel() { return kernel_; }
    
    /**
     * @brief Gets a mesh by its KoiloScript object ID.
     * @param objectID The object ID from the script.
     * @return Pointer to the Mesh, or nullptr if not found.
     */
    Mesh* GetMesh(const std::string& objectID);
    
    /**
     * @brief Checks if an error occurred.
     * @return True if there's an error.
     */
    bool HasError() const;
    
    /**
     * @brief Gets the last error message.
     * @return Error message string.
     */
    const char* GetError() const;

    KL_BEGIN_FIELDS(KoiloApplication)
        /* No reflected fields. */
    KL_END_FIELDS

    KL_BEGIN_METHODS(KoiloApplication)
        KL_METHOD_AUTO(KoiloApplication, LoadScript, "Load script"),
        KL_METHOD_AUTO(KoiloApplication, Update, "Update"),
        KL_METHOD_AUTO(KoiloApplication, EnableHotReload, "Enable hot reload"),
        KL_METHOD_AUTO(KoiloApplication, GetScene, "Get scene"),
        KL_METHOD_AUTO(KoiloApplication, GetEngine, "Get engine"),
        KL_METHOD_AUTO(KoiloApplication, GetMesh, "Get mesh"),
        KL_METHOD_AUTO(KoiloApplication, HasError, "Has error"),
        KL_METHOD_AUTO(KoiloApplication, GetError, "Get error")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(KoiloApplication)
        KL_CTOR0(KoiloApplication)
    KL_END_DESCRIBE(KoiloApplication)

};

} // namespace koilo
