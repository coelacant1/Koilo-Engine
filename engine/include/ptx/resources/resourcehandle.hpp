/**
 * @file resourcehandle.hpp
 * @brief Type-safe handle for referencing loaded resources.
 *
 * @date 11/10/2025
 * @author Coela
 */

#pragma once

#include <memory>
#include <string>
#include <cstdint>
#include "../../registry/reflect_macros.hpp"

namespace ptx {

/**
 * @class Resource
 * @brief Base class for all resources.
 */
class Resource {
protected:
    std::string path;
    uint64_t id;
    bool loaded;
    size_t memorySize;

public:
    /**
     * @brief Default constructor.
     */
    Resource() : path(""), id(0), loaded(false), memorySize(0) {}

    /**
     * @brief Virtual destructor.
     */
    virtual ~Resource() {}

    /**
     * @brief Gets the resource path.
     */
    std::string GetPath() const { return path; }

    /**
     * @brief Sets the resource path.
     */
    void SetPath(const std::string& p) { path = p; }

    /**
     * @brief Gets the resource ID.
     */
    uint64_t GetID() const { return id; }

    /**
     * @brief Sets the resource ID.
     */
    void SetID(uint64_t i) { id = i; }

    /**
     * @brief Checks if resource is loaded.
     */
    bool IsLoaded() const { return loaded; }

    /**
     * @brief Sets loaded state.
     */
    void SetLoaded(bool l) { loaded = l; }

    /**
     * @brief Gets memory size in bytes.
     */
    size_t GetMemorySize() const { return memorySize; }

    /**
     * @brief Sets memory size.
     */
    void SetMemorySize(size_t size) { memorySize = size; }

    /**
     * @brief Loads the resource from disk (override in derived classes).
     */
    virtual bool Load() { return false; }

    /**
     * @brief Unloads the resource from memory (override in derived classes).
     */
    virtual void Unload() {}

    /**
     * @brief Reloads the resource (hot-reload).
     */
    virtual bool Reload() {
        Unload();
        return Load();
    }

    PTX_BEGIN_FIELDS(Resource)
        PTX_FIELD(Resource, path, "Path", 0, 0),
        PTX_FIELD(Resource, id, "ID", 0, 0),
        PTX_FIELD(Resource, loaded, "Loaded", 0, 1),
        PTX_FIELD(Resource, memorySize, "Memory size", 0, 0)
    PTX_END_FIELDS

    PTX_BEGIN_METHODS(Resource)
        PTX_METHOD_AUTO(Resource, GetPath, "Get path"),
        PTX_METHOD_AUTO(Resource, IsLoaded, "Is loaded"),
        PTX_METHOD_AUTO(Resource, Load, "Load"),
        PTX_METHOD_AUTO(Resource, Unload, "Unload"),
        PTX_METHOD_AUTO(Resource, Reload, "Reload")
    PTX_END_METHODS

    PTX_BEGIN_DESCRIBE(Resource)
        PTX_CTOR0(Resource)
    PTX_END_DESCRIBE(Resource)
};

/**
 * @class ResourceHandle
 * @brief Type-safe handle for referencing resources.
 */
template<typename T>
class ResourceHandle {
private:
    std::shared_ptr<T> resource;
    uint64_t id;
    uint32_t generation;

public:
    /**
     * @brief Default constructor (null handle).
     */
    ResourceHandle() : resource(nullptr), id(0), generation(0) {}

    /**
     * @brief Constructor with resource pointer.
     */
    explicit ResourceHandle(std::shared_ptr<T> res, uint64_t id = 0, uint32_t gen = 0)
        : resource(res), id(id), generation(gen) {}

    /**
     * @brief Copy constructor.
     */
    ResourceHandle(const ResourceHandle& other) = default;

    /**
     * @brief Assignment operator.
     */
    ResourceHandle& operator=(const ResourceHandle& other) = default;

    /**
     * @brief Checks if handle is valid (points to a resource).
     */
    bool IsValid() const {
        return resource != nullptr;
    }

    /**
     * @brief Checks if handle is null.
     */
    bool IsNull() const {
        return resource == nullptr;
    }

    /**
     * @brief Gets the resource pointer.
     * @return Pointer to resource, or nullptr if invalid.
     */
    T* Get() const {
        return resource.get();
    }

    /**
     * @brief Gets the shared pointer.
     */
    std::shared_ptr<T> GetShared() const {
        return resource;
    }

    /**
     * @brief Gets the resource ID.
     */
    uint64_t GetID() const {
        return id;
    }

    /**
     * @brief Gets the generation counter.
     */
    uint32_t GetGeneration() const {
        return generation;
    }

    /**
     * @brief Resets the handle to null.
     */
    void Reset() {
        resource.reset();
        id = 0;
        generation = 0;
    }

    /**
     * @brief Arrow operator for convenient access.
     */
    T* operator->() const {
        return resource.get();
    }

    /**
     * @brief Dereference operator.
     */
    T& operator*() const {
        return *resource;
    }

    /**
     * @brief Bool conversion operator.
     */
    explicit operator bool() const {
        return IsValid();
    }

    /**
     * @brief Equality operator.
     */
    bool operator==(const ResourceHandle& other) const {
        return resource == other.resource && id == other.id;
    }

    /**
     * @brief Inequality operator.
     */
    bool operator!=(const ResourceHandle& other) const {
        return !(*this == other);
    }
};

/**
 * @class WeakResourceHandle
 * @brief Weak handle that doesn't keep resource alive.
 */
template<typename T>
class WeakResourceHandle {
private:
    std::weak_ptr<T> resource;
    uint64_t id;
    uint32_t generation;

public:
    /**
     * @brief Default constructor.
     */
    WeakResourceHandle() : id(0), generation(0) {}

    /**
     * @brief Constructor from strong handle.
     */
    WeakResourceHandle(const ResourceHandle<T>& handle)
        : resource(handle.GetShared()), id(handle.GetID()), generation(handle.GetGeneration()) {}

    /**
     * @brief Locks the weak handle to create a strong handle.
     * @return Strong handle if resource still exists, null handle otherwise.
     */
    ResourceHandle<T> Lock() const {
        auto shared = resource.lock();
        if (shared) {
            return ResourceHandle<T>(shared, id, generation);
        }
        return ResourceHandle<T>();
    }

    /**
     * @brief Checks if the resource is still alive.
     */
    bool IsExpired() const {
        return resource.expired();
    }

    /**
     * @brief Gets the resource ID.
     */
    uint64_t GetID() const {
        return id;
    }

    /**
     * @brief Resets the weak handle.
     */
    void Reset() {
        resource.reset();
        id = 0;
        generation = 0;
    }
};

} // namespace ptx
