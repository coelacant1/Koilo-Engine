/**
 * @file casthelper.hpp
 * @brief Explicit, zero-overhead helpers for common numeric casts.
 *
 * All helpers are `static inline` - they disappear after optimisation.
 * Using them makes every narrowing / widening conversion visible to the
 * compiler and to reviewers, silencing `-Wnarrowing` while staying type-safe.
 *
 * @date  18/06/2025
 * @author Coela Can't
 */
#pragma once

#include <stdint.h>
#include <math.h>

/**
 * @class CastHelper
 * @brief Collection of explicit cast utilities (narrow / widen).
 */
class CastHelper {
public:
    /**
     * @brief Narrow unsigned value to 8-bit (wrap-around).
     */
    template <typename T>
    static inline uint8_t ToU8(T v){
        return static_cast<uint8_t>(v);
    }

    /**
     * @brief Narrow unsigned value to 16-bit (wrap-around).
     */
    template <typename T>
    static inline uint16_t ToU16(T v){
        return static_cast<uint16_t>(v);
    }

    /**
     * @brief Widen unsigned 16-bit to 32-bit.
     */
    template <typename T>
    static inline uint32_t ToU32(T v){
        return static_cast<uint32_t>(v);
    }

    /**
     * @brief Convert unsigned 16-bit to signed 32-bit.
     */
    template <typename T>
    static inline int32_t ToI32(T v){
        return static_cast<int32_t>(v);
    }

    /**
     * @brief Round float to nearest int32 and cast.
     */
    template <typename T>
    static inline int32_t ToI32(float v){
        return static_cast<int32_t>(roundf(v));
    }

    /**
     * @brief Round float to nearest uint16 and cast (wrap-around if > 65535).
     */
    static inline uint16_t ToU16(float v){
        int32_t iv = static_cast<int32_t>(roundf(v));
        return static_cast<uint16_t>(iv);
    }

    /**
     * @brief Convert unsigned 16-bit to float.
     */
    static inline float ToFloat(uint16_t v){
        return static_cast<float>(v);
    }
};
