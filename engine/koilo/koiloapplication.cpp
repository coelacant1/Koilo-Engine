// SPDX-License-Identifier: GPL-3.0-or-later
#include <koilo/koiloapplication.hpp>
#include <koilo/core/time/timemanager.hpp>
#include <sys/stat.h>
#include <cstdio>

namespace koilo {

KoiloApplication::KoiloApplication()
    : hotReloadEnabled(false), lastModifiedTime(0) {
    
    fileReader = platform::CreatePlatformFileReader();
    engine = new scripting::KoiloScriptEngine(fileReader);
    scene = new Scene();
}

KoiloApplication::~KoiloApplication() {
    ClearScene();
    delete scene;
    delete engine;
    delete fileReader;
}

bool KoiloApplication::LoadScript(const char* filepath) {
    scriptPath = filepath;
    
    // Load and parse script
    if (!engine->LoadScript(filepath)) {
        return false;
    }
    
    // Build scene from script
    if (!engine->BuildScene()) {
        return false;
    }
    
    // Clear existing meshes
    ClearScene();
    
    // Note: At this point we would create actual Mesh objects from the
    // script's object definitions. For now, this is a placeholder until
    // we implement asset loading.
    
    // Store modification time for hot reload
    lastModifiedTime = GetFileModificationTime();
    
    return true;
}

void KoiloApplication::Update(float deltaTime) {
    // Check for script changes if hot reload is enabled
    if (hotReloadEnabled) {
        long currentModTime = GetFileModificationTime();
        if (currentModTime > lastModifiedTime) {
            printf("[Hot Reload] Script changed, reloading...\n");
            if (LoadScript(scriptPath.c_str())) {
                printf("[Hot Reload] Reload successful\n");
            } else {
                printf("[Hot Reload] Reload failed: %s\n", engine->GetError());
            }
        }
    }
    
    // Tick central time manager
    TimeManager::GetInstance().Tick(deltaTime);
    
    // Execute UPDATE block from script
    engine->ExecuteUpdate();
    
    // Note: Here we would update actual Mesh transforms from script object properties
    // Example: mesh->GetTransform().SetPosition(scriptObject.position)
}

void KoiloApplication::EnableHotReload(bool enable) {
    hotReloadEnabled = enable;
    if (enable) {
        printf("[Hot Reload] Enabled for %s\n", scriptPath.c_str());
    }
}

Scene* KoiloApplication::GetScene() {
    return scene;
}

scripting::KoiloScriptEngine* KoiloApplication::GetEngine() {
    return engine;
}

Mesh* KoiloApplication::GetMesh(const std::string& objectID) {
    auto it = meshMap.find(objectID);
    if (it != meshMap.end()) {
        return it->second;
    }
    return nullptr;
}

bool KoiloApplication::HasError() const {
    return engine->HasError();
}

const char* KoiloApplication::GetError() const {
    return engine->GetError();
}

void KoiloApplication::ClearScene() {
    // Remove all meshes from scene
    for (auto& [id, mesh] : meshMap) {
        scene->RemoveMesh(mesh);
        delete mesh;
    }
    meshMap.clear();
}

long KoiloApplication::GetFileModificationTime() {
#if defined(__linux__) || defined(__APPLE__)
    struct stat fileInfo;
    if (stat(scriptPath.c_str(), &fileInfo) == 0) {
        return fileInfo.st_mtime;
    }
#elif defined(_WIN32)
    struct _stat fileInfo;
    if (_stat(scriptPath.c_str(), &fileInfo) == 0) {
        return fileInfo.st_mtime;
    }
#endif
    return 0;
}

} // namespace koilo
