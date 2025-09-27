#pragma once

#include <cstdint>          // uint8_t
#include "effect.hpp"
#include "../core/ipixelgroup.hpp"

/**
 * @file compositor.hpp
 * @brief Fixed-capacity post-processing chain with enable toggles (no dynamic allocation).
 *
 * Holds up to @p MaxEffects effect pointers and applies them in insertion order to an
 * @ref IPixelGroup target. Storage is a static array; no std::vector or heap growth.
 *
 * @tparam MaxEffects Compile-time maximum number of effects (e.g., 8, 16).
 */

/**
 * @class Compositor
 * @brief Fixed-capacity effect chain for IPixelGroup processing.
 *
 * The chain stores non-owning pointers to @ref Effect instances. Each effect can be
 * individually enabled/disabled. Effects are applied in the order they were added.
 */
template <uint8_t MaxEffects>
class Compositor {
private:
    Effect* effects[MaxEffects];  ///< Non-owning effect pointers in insertion order
    bool    enabled[MaxEffects];  ///< Per-effect enable flags
    uint8_t count = 0;            ///< Number of effects currently in the chain

public:
    /** @brief Construct an empty chain (all slots unset/disabled). */
    Compositor();

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
    uint8_t GetCount() const { return count; }

};

// Template implementation
#include "compositor.tpp"
