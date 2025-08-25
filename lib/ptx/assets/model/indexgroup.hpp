// indexgroup.hpp
/**
 * @file IndexGroup.h
 * @brief Compact triple of indices (A,B,C) representing a triangle (e.g., OBJ/STL face).
 *
 * Provides small, component-wise arithmetic helpers for index math (add/sub/mul/div) and a
 * string formatter. Designed to be a minimal container for triangle index triplets pointing
 * into an external vertex buffer.
 *
 * @date 22/12/2024
 * @version 1.0
 * @author Coela Can't
 */
#pragma once

#include <stdint.h>
#include "../../core/utils/casthelper.hpp"   // CastHelper::ToU16
#include "../../core/math/mathematics.hpp"  // Mathematics::DoubleToCleanString
#include "../../core/platform/ustring.hpp"  // ptx::UString

/**
 * @class IndexGroup
 * @brief Holds three unsigned 16-bit indices for a triangle (A,B,C).
 *
 * This is intentionally lightweight and POD-like. Arithmetic methods operate component-wise
 * and return a newly constructed IndexGroup. No bounds checking or overflow protection is
 * performed beyond narrowing via CastHelper::ToU16.
 */
class IndexGroup {
public:
    uint16_t A; ///< First index of the triangle.
    uint16_t B; ///< Second index of the triangle.
    uint16_t C; ///< Third index of the triangle.

    /** @brief Default-constructs (0,0,0). */
    IndexGroup();

    /** @brief Copy-constructs from another IndexGroup. */
    IndexGroup(const IndexGroup& indexGroup);

    /**
     * @brief Construct from three indices.
     * @param A First index (A).
     * @param B Second index (B).
     * @param C Third index (C).
     */
    IndexGroup(uint16_t A, uint16_t B, uint16_t C);

    /**
     * @brief Adds two IndexGroups component-wise.
     * @param indexGroup Right-hand operand.
     * @return New IndexGroup {A+rhs.A, B+rhs.B, C+rhs.C} narrowed via ToU16.
     */
    IndexGroup Add(IndexGroup indexGroup);

    /**
     * @brief Subtracts two IndexGroups component-wise.
     * @param indexGroup Right-hand operand.
     * @return New IndexGroup {A-rhs.A, B-rhs.B, C-rhs.C} narrowed via ToU16.
     * @note No underflow checks; integer wrap may occur.
     */
    IndexGroup Subtract(IndexGroup indexGroup);

    /**
     * @brief Multiplies two IndexGroups component-wise.
     * @param indexGroup Right-hand operand.
     * @return New IndexGroup {A*rhs.A, B*rhs.B, C*rhs.C} narrowed via ToU16.
     */
    IndexGroup Multiply(IndexGroup indexGroup);

    /**
     * @brief Divides two IndexGroups component-wise.
     * @param indexGroup Right-hand operand.
     * @return New IndexGroup {A/rhs.A, B/rhs.B, C/rhs.C} narrowed via ToU16.
     * @warning No divide-by-zero checks; caller must ensure rhs components are non-zero.
     */
    IndexGroup Divide(IndexGroup indexGroup);

    /**
     * @brief Convert to a human-readable string.
     * @return UString formatted as "[A, B, C]".
     */
    ptx::UString ToString();
};
