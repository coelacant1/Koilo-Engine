/**
 * @file resourcemanager.hpp
 * @brief Central resource management system for loading and caching assets.
 *
 * @date 11/10/2025
 * @author Coela
 */

#pragma once

#include <unordered_map>
#include <string>
#include <memory>
#include <functional>
#include <mutex>
#include <vector>
#include <typeindex>
#include "resourcehandle.hpp"
#include "../../registry/reflect_macros.hpp"

namespace ptx {

/**
 * @typedef ResourceLoader
 * @brief Function type for custom resource loaders.
 */
template<typename T>
using ResourceLoader = std::function<std::shared_ptr<T>(const std::string&)>;

/**
 * @class ResourceManager
 * @brief Manages loading, caching, and lifecycle of resources.
 */
class ResourceManager {
private:
    // Singleton instance
    static ResourceManager* instance;

    // Resource cache (type -> path -> resource)
    std::unordered_map<std::type_index, std::unordered_map<std::string, std::shared_ptr<Resource>>> cache;

    // Resource loaders (type -> loader function)
    std::unordered_map<std::type_index, std::function<std::shared_ptr<Resource>(const std::string&)>> loaders;

    // Next resource ID
    uint64_t nextID;
    uint32_t generation;

    // Thread safety
    std::mutex cacheMutex;

    // Hot-reload tracking
    bool hotReloadEnabled;
    std::unordered_map<std::string, time_t> fileTimestamps;

    // Memory tracking
    size_t totalMemoryUsed;
    size_t memoryLimit;

    ResourceManager();

public:
    /**
     * @brief Gets the singleton instance.
     */
    static ResourceManager& GetInstance();

    /**
     * @brief Destructor.
     */
    ~ResourceManager();

    // === Resource Loading ===

    /**
     * @brief Loads a resource from disk.
     * @param path The resource path.
     * @return Handle to the loaded resource.
     */
    template<typename T>
    ResourceHandle<T> Load(const std::string& path);

    /**
     * @brief Loads a resource asynchronously (future implementation).
     * @param path The resource path.
     * @param callback Callback when loaded.
     */
    template<typename T>
    void LoadAsync(const std::string& path, std::function<void(ResourceHandle<T>)> callback);

    /**
     * @brief Unloads a specific resource.
     * @param path The resource path.
     */
    template<typename T>
    void Unload(const std::string& path);

    /**
     * @brief Unloads all resources of a specific type.
     */
    template<typename T>
    void UnloadAll();

    /**
     * @brief Unloads all resources.
     */
    void UnloadAllResources();

    // === Resource Cache ===

    /**
     * @brief Checks if a resource is cached.
     * @param path The resource path.
     */
    template<typename T>
    bool IsCached(const std::string& path) const;

    /**
     * @brief Gets a cached resource without loading.
     * @param path The resource path.
     * @return Handle to cached resource, or null handle if not cached.
     */
    template<typename T>
    ResourceHandle<T> GetCached(const std::string& path);

    /**
     * @brief Gets all cached resources of a type.
     */
    template<typename T>
    std::vector<ResourceHandle<T>> GetAllCached();

    // === Custom Loaders ===

    /**
     * @brief Registers a custom resource loader.
     * @param loader The loader function.
     */
    template<typename T>
    void RegisterLoader(ResourceLoader<T> loader);

    /**
     * @brief Unregisters a resource loader.
     */
    template<typename T>
    void UnregisterLoader();

    // === Hot Reload ===

    /**
     * @brief Enables hot-reloading of resources.
     */
    void EnableHotReload(bool enable) { hotReloadEnabled = enable; }

    /**
     * @brief Checks if hot-reload is enabled.
     */
    bool IsHotReloadEnabled() const { return hotReloadEnabled; }

    /**
     * @brief Checks for modified resources and reloads them.
     */
    void CheckHotReload();

    /**
     * @brief Reloads a specific resource.
     * @param path The resource path.
     */
    template<typename T>
    bool Reload(const std::string& path);

    // === Memory Management ===

    /**
     * @brief Gets total memory used by resources.
     */
    size_t GetTotalMemoryUsed() const { return totalMemoryUsed; }

    /**
     * @brief Sets memory limit for resources.
     * @param limit Memory limit in bytes (0 = unlimited).
     */
    void SetMemoryLimit(size_t limit) { memoryLimit = limit; }

    /**
     * @brief Gets memory limit.
     */
    size_t GetMemoryLimit() const { return memoryLimit; }

    /**
     * @brief Frees unused resources to reclaim memory.
     * @return Amount of memory freed in bytes.
     */
    size_t GarbageCollect();

    // === Statistics ===

    /**
     * @brief Gets the number of cached resources.
     */
    size_t GetCachedResourceCount() const;

    /**
     * @brief Gets the number of cached resources of a specific type.
     */
    template<typename T>
    size_t GetCachedResourceCount() const;

    /**
     * @brief Prints resource statistics to console.
     */
    void PrintStatistics();

private:
    /**
     * @brief Gets the type index for a resource type.
     */
    template<typename T>
    std::type_index GetTypeIndex() const {
        return std::type_index(typeid(T));
    }

    /**
     * @brief Generates a new resource ID.
     */
    uint64_t GenerateID() {
        return ++nextID;
    }

    /**
     * @brief Default loader (calls Resource::Load).
     */
    template<typename T>
    std::shared_ptr<T> DefaultLoader(const std::string& path);

    /**
     * @brief Updates memory tracking.
     */
    void UpdateMemoryTracking();

    PTX_BEGIN_FIELDS(ResourceManager)
        PTX_FIELD(ResourceManager, totalMemoryUsed, "Total memory used", 0, 0),
        PTX_FIELD(ResourceManager, memoryLimit, "Memory limit", 0, 0),
        PTX_FIELD(ResourceManager, hotReloadEnabled, "Hot reload enabled", 0, 1)
    PTX_END_FIELDS

    PTX_BEGIN_METHODS(ResourceManager)
        PTX_METHOD_AUTO(ResourceManager, UnloadAllResources, "Unload all resources"),
        PTX_METHOD_AUTO(ResourceManager, GetTotalMemoryUsed, "Get total memory used"),
        PTX_METHOD_AUTO(ResourceManager, SetMemoryLimit, "Set memory limit"),
        PTX_METHOD_AUTO(ResourceManager, GarbageCollect, "Garbage collect"),
        PTX_METHOD_AUTO(ResourceManager, GetCachedResourceCount, "Get cached resource count"),
        PTX_METHOD_AUTO(ResourceManager, PrintStatistics, "Print statistics"),
        PTX_METHOD_AUTO(ResourceManager, EnableHotReload, "Enable hot reload"),
        PTX_METHOD_AUTO(ResourceManager, CheckHotReload, "Check hot reload")
    PTX_END_METHODS

    PTX_BEGIN_DESCRIBE(ResourceManager)
        // Singleton, no constructors exposed
    PTX_END_DESCRIBE(ResourceManager)
};

// Template implementations

template<typename T>
ResourceHandle<T> ResourceManager::Load(const std::string& path) {
    std::lock_guard<std::mutex> lock(cacheMutex);

    auto typeIdx = GetTypeIndex<T>();

    // Check if already cached
    auto& typeCache = cache[typeIdx];
    auto it = typeCache.find(path);
    if (it != typeCache.end()) {
        // Cast to specific type
        auto resource = std::static_pointer_cast<T>(it->second);
        return ResourceHandle<T>(resource, resource->GetID(), generation);
    }

    // Load new resource
    std::shared_ptr<T> resource;

    // Check for custom loader
    auto loaderIt = loaders.find(typeIdx);
    if (loaderIt != loaders.end()) {
        // Use custom loader (cast is safe because we registered it)
        auto loader = reinterpret_cast<ResourceLoader<T>*>(&loaderIt->second);
        resource = (*loader)(path);
    } else {
        // Use default loader
        resource = DefaultLoader<T>(path);
    }

    if (resource) {
        resource->SetPath(path);
        resource->SetID(GenerateID());

        // Cache the resource
        typeCache[path] = resource;

        // Update memory tracking
        totalMemoryUsed += resource->GetMemorySize();

        return ResourceHandle<T>(resource, resource->GetID(), generation);
    }

    return ResourceHandle<T>();  // Null handle
}

template<typename T>
void ResourceManager::LoadAsync(const std::string& path, std::function<void(ResourceHandle<T>)> callback) {
    // TODO: Implement async loading with thread pool
    // For now, just load synchronously and call callback
    auto handle = Load<T>(path);
    if (callback) {
        callback(handle);
    }
}

template<typename T>
void ResourceManager::Unload(const std::string& path) {
    std::lock_guard<std::mutex> lock(cacheMutex);

    auto typeIdx = GetTypeIndex<T>();
    auto& typeCache = cache[typeIdx];
    auto it = typeCache.find(path);

    if (it != typeCache.end()) {
        auto resource = it->second;
        totalMemoryUsed -= resource->GetMemorySize();
        resource->Unload();
        typeCache.erase(it);
    }
}

template<typename T>
void ResourceManager::UnloadAll() {
    std::lock_guard<std::mutex> lock(cacheMutex);

    auto typeIdx = GetTypeIndex<T>();
    auto& typeCache = cache[typeIdx];

    for (auto& pair : typeCache) {
        auto resource = pair.second;
        totalMemoryUsed -= resource->GetMemorySize();
        resource->Unload();
    }

    typeCache.clear();
}

template<typename T>
bool ResourceManager::IsCached(const std::string& path) const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(cacheMutex));

    auto typeIdx = GetTypeIndex<T>();
    auto cacheIt = cache.find(typeIdx);
    if (cacheIt == cache.end()) return false;

    return cacheIt->second.find(path) != cacheIt->second.end();
}

template<typename T>
ResourceHandle<T> ResourceManager::GetCached(const std::string& path) {
    std::lock_guard<std::mutex> lock(cacheMutex);

    auto typeIdx = GetTypeIndex<T>();
    auto& typeCache = cache[typeIdx];
    auto it = typeCache.find(path);

    if (it != typeCache.end()) {
        auto resource = std::static_pointer_cast<T>(it->second);
        return ResourceHandle<T>(resource, resource->GetID(), generation);
    }

    return ResourceHandle<T>();
}

template<typename T>
std::vector<ResourceHandle<T>> ResourceManager::GetAllCached() {
    std::lock_guard<std::mutex> lock(cacheMutex);

    std::vector<ResourceHandle<T>> handles;
    auto typeIdx = GetTypeIndex<T>();
    auto& typeCache = cache[typeIdx];

    for (auto& pair : typeCache) {
        auto resource = std::static_pointer_cast<T>(pair.second);
        handles.emplace_back(resource, resource->GetID(), generation);
    }

    return handles;
}

template<typename T>
void ResourceManager::RegisterLoader(ResourceLoader<T> loader) {
    std::lock_guard<std::mutex> lock(cacheMutex);

    auto typeIdx = GetTypeIndex<T>();
    loaders[typeIdx] = [loader](const std::string& path) -> std::shared_ptr<Resource> {
        return loader(path);
    };
}

template<typename T>
void ResourceManager::UnregisterLoader() {
    std::lock_guard<std::mutex> lock(cacheMutex);

    auto typeIdx = GetTypeIndex<T>();
    loaders.erase(typeIdx);
}

template<typename T>
bool ResourceManager::Reload(const std::string& path) {
    std::lock_guard<std::mutex> lock(cacheMutex);

    auto typeIdx = GetTypeIndex<T>();
    auto& typeCache = cache[typeIdx];
    auto it = typeCache.find(path);

    if (it != typeCache.end()) {
        return it->second->Reload();
    }

    return false;
}

template<typename T>
size_t ResourceManager::GetCachedResourceCount() const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(cacheMutex));

    auto typeIdx = GetTypeIndex<T>();
    auto cacheIt = cache.find(typeIdx);
    if (cacheIt == cache.end()) return 0;

    return cacheIt->second.size();
}

template<typename T>
std::shared_ptr<T> ResourceManager::DefaultLoader(const std::string& path) {
    auto resource = std::make_shared<T>();
    resource->SetPath(path);

    if (resource->Load()) {
        return resource;
    }

    return nullptr;
}

} // namespace ptx
