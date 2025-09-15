#pragma once

#include <type_traits>
#include "../shader/ishader.hpp"
#include "../../../registry/reflect_macros.hpp"

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
     * @param DeltaTime Time step in seconds since last update.
     */
    virtual void Update(float /*DeltaTime*/) {}

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

    PTX_BEGIN_FIELDS(IMaterial)
        /* TODO: PTX_FIELD(IMaterial, member, "Doc", min, max) */
    PTX_END_FIELDS
    
    PTX_BEGIN_METHODS(IMaterial)
        PTX_METHOD_AUTO(IMaterial, GetShader, "Get shader"),
        PTX_METHOD_AUTO(IMaterial, Update, "Update")
    PTX_END_METHODS
    
    PTX_BEGIN_DESCRIBE(IMaterial)
        PTX_CTOR(IMaterial, const IShader *)
    PTX_END_DESCRIBE(IMaterial)
    
};
