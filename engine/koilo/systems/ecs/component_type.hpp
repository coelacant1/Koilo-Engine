// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file component_type.hpp
 * @brief Runtime component type descriptor for modular ECS.
 *
 * Provides IComponentType (interface for runtime-registered components)
 * and ComponentType<T> (convenience template that auto-implements from T).
 *
 * @date 03/30/2026
 * @author Coela
 */
#pragma once

#include <cstddef>
#include <cstring>
#include <string>
#include <new>
#include "component.hpp"

namespace koilo {

/**
 * @class IComponentType
 * @brief Describes a component type at runtime for type-erased storage.
 */
class IComponentType {
public:
    virtual ~IComponentType() = default;

    virtual const char* GetName() const = 0;
    virtual size_t      GetSize() const = 0;
    virtual size_t      GetAlignment() const = 0;

    /// Placement-new a default instance into `dest`.
    virtual void Construct(void* dest) const = 0;

    /// Destroy the instance at `ptr` (call destructor, no dealloc).
    virtual void Destruct(void* ptr) const = 0;

    /// Copy `src` into already-constructed `dest`.
    virtual void Copy(void* dest, const void* src) const = 0;

    /// Move `src` into already-constructed `dest`.
    virtual void Move(void* dest, void* src) const = 0;
};

/**
 * @class ComponentType
 * @brief Convenience template that auto-implements IComponentType for T.
 *
 * T must be default-constructible, copy-assignable, and move-assignable.
 */
template<typename T>
class ComponentType : public IComponentType {
public:
    explicit ComponentType(const char* name) : name_(name) {}

    const char* GetName()      const override { return name_; }
    size_t      GetSize()      const override { return sizeof(T); }
    size_t      GetAlignment() const override { return alignof(T); }

    void Construct(void* dest) const override {
        new (dest) T();
    }

    void Destruct(void* ptr) const override {
        static_cast<T*>(ptr)->~T();
    }

    void Copy(void* dest, const void* src) const override {
        *static_cast<T*>(dest) = *static_cast<const T*>(src);
    }

    void Move(void* dest, void* src) const override {
        *static_cast<T*>(dest) = std::move(*static_cast<T*>(src));
    }

private:
    const char* name_;
};

} // namespace koilo
