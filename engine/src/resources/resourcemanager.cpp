#include <ptx/resources/resourcemanager.hpp>
#include <iostream>
#include <sys/stat.h>

namespace ptx {

// Static instance
ResourceManager* ResourceManager::instance = nullptr;

// Constructor
ResourceManager::ResourceManager()
    : nextID(0), generation(0), hotReloadEnabled(false),
      totalMemoryUsed(0), memoryLimit(0) {
}

// Destructor
ResourceManager::~ResourceManager() {
    UnloadAllResources();
}

// Singleton instance
ResourceManager& ResourceManager::GetInstance() {
    if (instance == nullptr) {
        instance = new ResourceManager();
    }
    return *instance;
}

// === Resource Unloading ===

void ResourceManager::UnloadAllResources() {
    std::lock_guard<std::mutex> lock(cacheMutex);

    // Unload all resources in all type caches
    for (auto& typePair : cache) {
        for (auto& resourcePair : typePair.second) {
            if (resourcePair.second) {
                resourcePair.second->Unload();
            }
        }
        typePair.second.clear();
    }

    cache.clear();
    loaders.clear();
    fileTimestamps.clear();
    totalMemoryUsed = 0;

    // Increment generation to invalidate existing handles
    generation++;
}

// === Hot Reload ===

void ResourceManager::CheckHotReload() {
    if (!hotReloadEnabled) return;

    std::lock_guard<std::mutex> lock(cacheMutex);

    // Iterate through all cached resources
    for (auto& typePair : cache) {
        for (auto& resourcePair : typePair.second) {
            const std::string& path = resourcePair.first;
            auto resource = resourcePair.second;

            // Check file modification time
            struct stat fileStat;
            if (stat(path.c_str(), &fileStat) == 0) {
                time_t currentMTime = fileStat.st_mtime;

                // Check if file has been modified
                auto it = fileTimestamps.find(path);
                if (it != fileTimestamps.end()) {
                    if (currentMTime > it->second) {
                        // File modified, reload
                        std::cout << "[ResourceManager] Hot-reloading: " << path << std::endl;

                        size_t oldSize = resource->GetMemorySize();
                        bool success = resource->Reload();

                        if (success) {
                            size_t newSize = resource->GetMemorySize();
                            totalMemoryUsed = totalMemoryUsed - oldSize + newSize;
                            fileTimestamps[path] = currentMTime;
                            std::cout << "[ResourceManager] Hot-reload successful: " << path << std::endl;
                        } else {
                            std::cout << "[ResourceManager] Hot-reload failed: " << path << std::endl;
                        }
                    }
                } else {
                    // First time tracking this file
                    fileTimestamps[path] = currentMTime;
                }
            }
        }
    }
}

// === Memory Management ===

size_t ResourceManager::GarbageCollect() {
    std::lock_guard<std::mutex> lock(cacheMutex);

    size_t freedMemory = 0;

    // Iterate through all type caches
    for (auto& typePair : cache) {
        auto& typeCache = typePair.second;
        auto it = typeCache.begin();

        while (it != typeCache.end()) {
            auto resource = it->second;

            // Check if resource is only held by cache (use_count == 1)
            if (resource.use_count() == 1) {
                size_t memSize = resource->GetMemorySize();
                freedMemory += memSize;
                totalMemoryUsed -= memSize;

                resource->Unload();
                it = typeCache.erase(it);
            } else {
                ++it;
            }
        }
    }

    if (freedMemory > 0) {
        std::cout << "[ResourceManager] Garbage collection freed "
                  << freedMemory << " bytes" << std::endl;
    }

    return freedMemory;
}

void ResourceManager::UpdateMemoryTracking() {
    std::lock_guard<std::mutex> lock(cacheMutex);

    totalMemoryUsed = 0;
    for (auto& typePair : cache) {
        for (auto& resourcePair : typePair.second) {
            totalMemoryUsed += resourcePair.second->GetMemorySize();
        }
    }
}

// === Statistics ===

size_t ResourceManager::GetCachedResourceCount() const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(cacheMutex));

    size_t count = 0;
    for (const auto& typePair : cache) {
        count += typePair.second.size();
    }
    return count;
}

void ResourceManager::PrintStatistics() {
    std::lock_guard<std::mutex> lock(cacheMutex);

    std::cout << "\n=== Resource Manager Statistics ===" << std::endl;
    std::cout << "Total cached resources: " << GetCachedResourceCount() << std::endl;
    std::cout << "Total memory used: " << totalMemoryUsed << " bytes";

    // Convert to MB if large
    if (totalMemoryUsed > 1024 * 1024) {
        float mb = static_cast<float>(totalMemoryUsed) / (1024.0f * 1024.0f);
        std::cout << " (" << mb << " MB)";
    }
    std::cout << std::endl;

    if (memoryLimit > 0) {
        float percentage = (static_cast<float>(totalMemoryUsed) / memoryLimit) * 100.0f;
        std::cout << "Memory limit: " << memoryLimit << " bytes";
        if (memoryLimit > 1024 * 1024) {
            float mb = static_cast<float>(memoryLimit) / (1024.0f * 1024.0f);
            std::cout << " (" << mb << " MB)";
        }
        std::cout << " - " << percentage << "% used" << std::endl;
    } else {
        std::cout << "Memory limit: Unlimited" << std::endl;
    }

    std::cout << "Hot-reload enabled: " << (hotReloadEnabled ? "Yes" : "No") << std::endl;
    std::cout << "Generation: " << generation << std::endl;
    std::cout << "Next ID: " << nextID << std::endl;

    // Print resources by type
    std::cout << "\nResources by type:" << std::endl;
    for (const auto& typePair : cache) {
        std::cout << "  Type " << typePair.first.name() << ": "
                  << typePair.second.size() << " resources" << std::endl;

        // List individual resources
        for (const auto& resourcePair : typePair.second) {
            const std::string& path = resourcePair.first;
            auto resource = resourcePair.second;
            size_t memSize = resource->GetMemorySize();
            int refCount = resource.use_count();

            std::cout << "    - " << path
                      << " (ID: " << resource->GetID()
                      << ", Size: " << memSize << " bytes"
                      << ", Refs: " << refCount
                      << ", Loaded: " << (resource->IsLoaded() ? "Yes" : "No")
                      << ")" << std::endl;
        }
    }

    std::cout << "===================================\n" << std::endl;
}

} // namespace ptx
