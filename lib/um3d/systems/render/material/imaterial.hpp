/**
 * @file Material.h
 * @brief Base class for materials defining shading and rendering behavior.
 *
 * The `Material` class serves as an interface for materials that calculate
 * color based on various blending methods and surface parameters.
 *
 * @date 22/12/2024
 * @version 1.1
 * @author Coela Can't
 */

#pragma once

#include "../shader/ishader.hpp"

class IMaterial {
public:
    explicit IMaterial(const IShader* ShaderPtr) : ShaderPtr_(ShaderPtr) {}
    virtual ~IMaterial() = default;

    /** Return associated shader */
    const IShader* GetShader() const { return ShaderPtr_; }

    /** Per‑frame update hook (override if animated) */
    virtual void Update(float /*DeltaTime*/) {}

    /**
     * @brief Typed access helper with UpperCamelCase name.
     */
    template<typename T>
    const T& As() const noexcept {
        static_assert(std::is_base_of_v<IMaterial, T>, "Invalid As<T>() cast");
        return static_cast<const T&>(*this);
    }

private:
    const IShader* ShaderPtr_;  ///< non‑owning
};
