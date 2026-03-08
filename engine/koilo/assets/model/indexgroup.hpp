// SPDX-License-Identifier: GPL-3.0-or-later
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
#include <koilo/core/utils/casthelper.hpp>   // CastHelper::ToU16
#include <koilo/core/math/mathematics.hpp>  // Mathematics::DoubleToCleanString
#include <koilo/core/platform/ustring.hpp>  // koilo::UString
#include <koilo/registry/reflect_macros.hpp>


namespace koilo {

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
    uint32_t A; ///< First index of the triangle.
    uint32_t B; ///< Second index of the triangle.
    uint32_t C; ///< Third index of the triangle.

    /** @brief Default-constructs (0,0,0). */
    IndexGroup();

    /** @brief Copy-constructs from another IndexGroup. */
    IndexGroup(const IndexGroup& indexGroup);

    /** @brief Copy-assignment operator. */
    IndexGroup& operator=(const IndexGroup& indexGroup) = default;

    /**
     * @brief Construct from three indices.
     * @param A First index (A).
     * @param B Second index (B).
     * @param C Third index (C).
     */
    IndexGroup(uint32_t A, uint32_t B, uint32_t C);

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
     * @brief Retrieves an index by position (0=A, 1=B, 2=C).
     */
    uint32_t GetIndex(uint8_t idx) const {
        switch (idx) {
            case 0: return A;
            case 1: return B;
            default: return C;
        }
    }

    /**
     * @brief Convert to a human-readable string.
     * @return UString formatted as "[A, B, C]".
     */
    koilo::UString ToString();

    KL_BEGIN_FIELDS(IndexGroup)
        KL_FIELD(IndexGroup, A, "A", 0, 65535),
        KL_FIELD(IndexGroup, B, "B", 0, 65535),
        KL_FIELD(IndexGroup, C, "C", 0, 65535)
    KL_END_FIELDS

    KL_BEGIN_METHODS(IndexGroup)
        KL_METHOD_AUTO(IndexGroup, Add, "Add"),
        KL_METHOD_AUTO(IndexGroup, Subtract, "Subtract"),
        KL_METHOD_AUTO(IndexGroup, Multiply, "Multiply"),
        KL_METHOD_AUTO(IndexGroup, Divide, "Divide"),
        KL_METHOD_AUTO(IndexGroup, GetIndex, "Get index"),
        KL_METHOD_AUTO(IndexGroup, ToString, "To string")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(IndexGroup)
        KL_CTOR0(IndexGroup),
        KL_CTOR(IndexGroup, const IndexGroup &),
        KL_CTOR(IndexGroup, uint16_t, uint16_t, uint16_t)
    KL_END_DESCRIBE(IndexGroup)

};

} // namespace koilo
