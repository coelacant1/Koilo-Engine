/**
 * @file compositor.hpp
 * @brief Fixed-capacity screen-space effect chain.
 *
 * @date  18/06/2025
 * @author Coela Can't
 */
#pragma once

#include <cstddef>
#include "effect.hpp"

/**
 * @class Compositor
 * @brief Holds up to `MaxEffects` post-effects and runs them in order each frame.
 * @tparam Max maximum number of effects accepted at run-time.
 *
 */
template <size_t MaxEffects>
class Compositor {
public:
    /** @brief Default constructor (empty stack). */
    Compositor();

    /**
     * @brief Add effect pointer to stack.
     * @param fx pointer to Effect (must stay valid for lifetime of compositor)
     * @return true if added, false if stack already full
     */
    bool Add(Effect* fx);

    /** @brief Remove all effects from stack. */
    void Clear();

    /**
     * @brief Apply every effect to framebuffer in insertion order.
     * @param pixelGroup target framebuffer
     */
    void Process(IPixelGroup* pixelGroup);

private:
    Effect* list[MaxEffects];   ///< static pointer array
    unsigned char  count;       ///< current number of effects
};

#include "compositor.tpp"