#pragma once

#include <cstdint>          // uint8_t
#include <vector>
#include "../../../registry/reflect_macros.hpp"

#include "effect.hpp"
#include "../core/ipixelgroup.hpp"

/**
 * @file compositor.hpp
 * @brief Runtime-configurable post-processing chain with enable toggles.
 *
 * The compositor stores a fixed maximum number of non-owning @ref Effect pointers and
 * applies them in insertion order to an @ref IPixelGroup. Capacity is supplied at
 * construction time instead of being a compile-time template parameter.
 */

/**
 * @class Compositor
 * @brief Fixed-capacity effect chain for IPixelGroup processing.
 *
 * A compositor maintains an ordered list of @ref Effect instances, each of which can be
 * toggled on or off. Effects are applied sequentially to the supplied pixel group.
 */
class Compositor {
public:
    /**
     * @brief Construct an empty chain with the requested capacity.
     * @param maxEffects Maximum number of effects that can be stored.
     */
    explicit Compositor(uint8_t maxEffects = 8);

    /**
     * @brief Append an effect to the chain.
     * @param fx     Effect pointer (non-owning).
     * @param enable Initial enabled state (default true).
     * @return true if appended; false if capacity is full or @p fx is null.
     */
    bool AddEffect(Effect* fx, bool enable = true);

    /**
     * @brief Enable or disable an effect by index.
     * @param index Chain index in [0, GetCount()).
     * @param state New enabled state.
     */
    void SetEnabled(uint8_t index, bool state);

    /**
     * @brief Remove all effects from the chain.
     * @note Pointers are not deleted; this class does not take ownership.
     */
    void Clear();

    /**
     * @brief Apply all enabled effects in order to the target pixel group.
     * @param pixelGroup Target to process (must be valid for all effects used).
     */
    void Apply(IPixelGroup* pixelGroup);

    /** @brief Current number of effects in the chain. */
    uint8_t GetCount() const { return count_; }

    /** @brief Maximum number of effects that can be stored. */
    uint8_t GetCapacity() const { return capacity_; }

private:
    uint8_t capacity_;                  ///< Maximum number of effects allowed.
    uint8_t count_ = 0;                 ///< Number of active effects in the chain.
    std::vector<Effect*> effects_;      ///< Stored effect pointers.
    std::vector<bool>    enabled_;      ///< Enable flags per effect.

    PTX_BEGIN_FIELDS(Compositor)
        /* No reflected fields. */
    PTX_END_FIELDS

    PTX_BEGIN_METHODS(Compositor)
        PTX_METHOD_AUTO(Compositor, AddEffect, "Add effect"),
        PTX_METHOD_AUTO(Compositor, SetEnabled, "Set enabled"),
        PTX_METHOD_AUTO(Compositor, Clear, "Clear"),
        PTX_METHOD_AUTO(Compositor, Apply, "Apply"),
        PTX_METHOD_AUTO(Compositor, GetCount, "Get count"),
        PTX_METHOD_AUTO(Compositor, GetCapacity, "Get capacity")
    PTX_END_METHODS

    PTX_BEGIN_DESCRIBE(Compositor)
        PTX_CTOR(Compositor, uint8_t)
    PTX_END_DESCRIBE(Compositor)

};
