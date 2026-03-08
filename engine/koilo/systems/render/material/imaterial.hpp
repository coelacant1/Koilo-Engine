// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <type_traits>
#include <koilo/systems/render/shader/ishader.hpp>
#include <koilo/registry/reflect_macros.hpp>


namespace koilo {

/**
 * @file imaterial.hpp
 * @brief Base interface for materials bound to a shader.
 *
 * IMaterial represents a render material that references an associated @ref IShader.
 * It provides an optional per-frame update hook and a typed-cast helper for concrete materials.
 */
class IMaterial {
public:
    /**
     * @brief Construct with an associated shader (non-owning).
     * @param ShaderPtr Pointer to the shader used by this material; must outlive the material.
     */
    explicit IMaterial(const IShader* ShaderPtr) : ShaderPtr_(ShaderPtr) {}

    /** @brief Virtual destructor. */
    virtual ~IMaterial() = default;

    /**
     * @brief Get the associated shader.
     * @return Non-owning pointer to the shader.
     */
    const IShader* GetShader() const { return ShaderPtr_; }

    /**
     * @brief Per-frame update hook (override in animated/time-varying materials).
     */
    virtual void Update() {}

    /**
     * @brief Returns true for KSLMaterial instances (avoids RTTI dynamic_cast).
     */
    virtual bool IsKSL() const { return false; }

    /**
     * @brief Typed access helper using an UpperCamelCase name.
     * @tparam T Concrete material type deriving from IMaterial.
     * @return Reference to *this* as T.
     * @note Uses `static_assert` to enforce `T` derives from `IMaterial`.
     *       Behavior is `static_cast` (no RTTI); ensure the dynamic type matches `T`.
     */
    template<typename T>
    const T& As() const noexcept {
        static_assert(std::is_base_of_v<IMaterial, T>, "Invalid As<T>() cast");
        return static_cast<const T&>(*this);
    }

private:
    const IShader* ShaderPtr_;  ///< Non-owning shader pointer; lifetime managed externally.

    KL_BEGIN_FIELDS(IMaterial)
        /* No reflected fields. */
    KL_END_FIELDS

    KL_BEGIN_METHODS(IMaterial)
        KL_METHOD_AUTO(IMaterial, GetShader, "Get shader"),
        KL_METHOD_AUTO(IMaterial, Update, "Update")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(IMaterial)
        KL_CTOR(IMaterial, const IShader *)
    KL_END_DESCRIBE(IMaterial)

};

} // namespace koilo
