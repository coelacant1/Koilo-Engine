// materialanimator.hpp
#pragma once

#include <cstdint>
#include "../material/imaterial.hpp"
#include "../material/materialt.hpp"
#include "../material/implementations/combinematerial.hpp"
#include "../../scene/animation/easyeaseanimator.hpp"
#include "../../../core/math/vector3d.hpp"
#include "../../../core/color/rgbcolor.hpp"
#include "../../../registry/reflect_macros.hpp"

/**
 * @file materialanimator.hpp
 * @brief Animated opacity blender: drives a CombineMaterial via per-layer curves.
 *
 * The @c MaterialAnimator holds up to @p materialCount layers and animates each layer’s opacity
 * with an @c EasyEaseAnimator track. Shading is delegated to an internal
 * @c CombineMaterial<materialCount>, and the shader wrapper @c MaterialAnimatorShader forwards
 * to the combiner’s shader.
 */

/**
 * @tparam N Layer count (must match owning MaterialAnimator’s materialCount).
 * @brief Shader that forwards to the internal CombineMaterial of a MaterialAnimator.
 */
template<std::size_t N>
class MaterialAnimatorShader final : public IShader {
public:
    /**
     * @brief Shade by deferring to the MaterialAnimator’s internal CombineMaterial.
     * @param sp Surface properties from the renderer.
     * @param m  Material bound at the call site (expected to be MaterialAnimator<N>).
     * @return Result of the combiner’s shader @c Shade().
     * @warning Uses @c static_cast to @c MaterialAnimator<N>; ensure the bound material matches N.
     */
    RGBColor Shade(const SurfaceProperties& sp, const IMaterial& m) override;

    /* NOTE: MaterialAnimatorShader is a template; verify macros accept template types. */
    PTX_BEGIN_FIELDS(MaterialAnimatorShader)
        /* TODO: PTX_FIELD(MaterialAnimatorShader, member, "Doc", min, max) */
    PTX_END_FIELDS
    
    PTX_BEGIN_METHODS(MaterialAnimatorShader)
        PTX_METHOD_AUTO(MaterialAnimatorShader, Shade, "Shade")
    PTX_END_METHODS
    
    PTX_BEGIN_DESCRIBE(MaterialAnimatorShader)
        /* TODO: PTX_CTOR0(MaterialAnimatorShader) or PTX_CTOR(MaterialAnimatorShader, ...) */
    PTX_END_DESCRIBE(MaterialAnimatorShader)
    
};

/**
 * @brief Empty parameter block placeholder for MaterialAnimator.
 *
 * Kept to mirror the @c MaterialT pattern, even though state is owned by @c MaterialAnimator.
 */
struct MaterialAnimatorParams { 
    PTX_BEGIN_FIELDS(MaterialAnimatorParams)
        /* TODO: PTX_FIELD(MaterialAnimatorParams, member, "Doc", min, max) */
    PTX_END_FIELDS
    
    PTX_BEGIN_METHODS(MaterialAnimatorParams)
        /* TODO: PTX_METHOD_AUTO(MaterialAnimatorParams, Method, "Doc") */
    PTX_END_METHODS
    
    PTX_BEGIN_DESCRIBE(MaterialAnimatorParams)
        /* TODO: PTX_CTOR0(MaterialAnimatorParams) or PTX_CTOR(MaterialAnimatorParams, ...) */
    PTX_END_DESCRIBE(MaterialAnimatorParams)
    
};

/**
 * @tparam materialCount Maximum number of material layers.
 * @brief Stateful material that animates layer opacities and blends via CombineMaterial.
 *
 * Layer 0 is considered the base layer and is clamped to opacity 1.0. Additional layers
 * (1..materialCount-1) can be added with animated opacity tracks. Per-frame, call @ref Update()
 * to advance curves and push opacities into the internal @c CombineMaterial.
 */
template<std::size_t materialCount>
class MaterialAnimator : public IMaterial {
public:
    using ShaderT  = MaterialAnimatorShader<materialCount>;   ///< Forwarding shader type
    using ThisT    = MaterialAnimator<materialCount>;         ///< Self alias
    using Combiner = CombineMaterial<materialCount>;          ///< Underlying combiner
    using Method   = typename Combiner::Method;               ///< Blend method enum

    /** @brief Construct with this animator’s shader bound to IMaterial. */
    MaterialAnimator();

    /**
     * @brief Ensure layer 0 exists and is fully opaque; set its method/material.
     * @param method   Blend method for base layer (index 0).
     * @param material Material pointer for base layer (non-owning).
     * @note On subsequent calls, updates layer 0 in place.
     */
    void SetBaseMaterial(Method method, IMaterial* material);

    /**
     * @brief Add a new animated layer (indices 1..materialCount-1).
     * @param method     Blend method.
     * @param material   Material pointer (non-owning). Ignored if already present.
     * @param frames     Keyframe capacity or duration parameter for the animator.
     * @param minOpacity Initial opacity and lower bound for the animated track [0..1].
     * @param maxOpacity Upper bound for the animated track [0..1].
     */
    void AddMaterial(Method method,
                     IMaterial* material,
                     uint16_t   frames,
                     float      minOpacity,
                     float      maxOpacity);

    /**
     * @brief Append a keyframe to a layer’s opacity curve (by material identity).
     * @param material Target material previously added.
     * @param opacity  Opacity value [0..1] for the new keyframe.
     */
    void AddMaterialFrame(IMaterial& material, float opacity);

    /**
     * @brief Query the current animated opacity for a given material.
     * @param material Target material.
     * @return Current value if found, otherwise 0.0f.
     */
    float GetMaterialOpacity(IMaterial& material) const;

    /** @brief Advance animation curves and push opacities into the combiner. */
    void Update();

    /** @brief Access the underlying combiner (mutable). */
    Combiner&       GetCombiner()       { return combine_; }
    /** @brief Access the underlying combiner (const). */
    const Combiner& GetCombiner() const { return combine_; }

private:
    /** @brief Singleton shader used to construct the IMaterial base. */
    static const IShader* ShaderPtr();

public:
    /**
     * @brief Convenience ctor to satisfy factories that pass nullptr; binds our shader.
     * @note Leaves internal state default-constructed; call SetBaseMaterial/AddMaterial as needed.
     */
    MaterialAnimator(std::nullptr_t) : IMaterial(ShaderPtr()) {}

private:
    // Animation + blending state
    EasyEaseAnimator<materialCount> eEA_{ IEasyEaseAnimator::Cosine }; ///< Opacity animators

    Combiner   combine_;                                   ///< Underlying blender
    float      materialRatios_[materialCount] = {};        ///< Animated opacity values per layer
    IMaterial* dictionary_[materialCount]     = { nullptr };///< Layer->material identity map
    uint8_t    currentMaterials_              = 0;         ///< Number of active layers
    bool       baseMaterialSet_               = false;     ///< Whether layer 0 is initialized

    /* NOTE: MaterialAnimator is a template; verify macros accept template types. */
    PTX_BEGIN_FIELDS(MaterialAnimator)
        /* TODO: PTX_FIELD(MaterialAnimator, member, "Doc", min, max) */
    PTX_END_FIELDS
    
    PTX_BEGIN_METHODS(MaterialAnimator)
        PTX_METHOD_AUTO(MaterialAnimator, SetBaseMaterial, "Set base material"),
        PTX_METHOD_AUTO(MaterialAnimator, AddMaterial, "Add material"),
        PTX_METHOD_AUTO(MaterialAnimator, AddMaterialFrame, "Add material frame"),
        PTX_METHOD_AUTO(MaterialAnimator, GetMaterialOpacity, "Get material opacity"),
        PTX_METHOD_AUTO(MaterialAnimator, Update, "Update"),
        PTX_METHOD_AUTO(MaterialAnimator, GetCombiner, "Get combiner"),
        PTX_METHOD_AUTO(MaterialAnimator, GetCombiner, "Get combiner")
    PTX_END_METHODS
    
    PTX_BEGIN_DESCRIBE(MaterialAnimator)
        PTX_CTOR0(MaterialAnimator),
        PTX_CTOR(MaterialAnimator, std::nullptr_t)
    PTX_END_DESCRIBE(MaterialAnimator)
    
};

#include "materialanimator.tpp"
