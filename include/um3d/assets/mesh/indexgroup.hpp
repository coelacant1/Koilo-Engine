/**
 * @file IndexGroup.h
 * @brief Defines the IndexGroup class for handling a group of three unsigned integer indices.
 *
 * The IndexGroup class provides functionality to manage and perform arithmetic operations
 * on groups of three indices, such as addition, subtraction, multiplication, and division.
 * It is useful for managing vertex or element indices in various computational contexts.
 *
 * @date 22/12/2024
 * @version 1.0
 * @author Coela Can't
 */

#pragma once

#include <cstdint>
#include "../../helpers/stringhelper.hpp"

/**
 * @class IndexGroup
 * @brief Represents a group of three unsigned integer indices.
 */
class IndexGroup {
public:
    uint16_t A; ///< First index in the group.
    uint16_t B; ///< Second index in the group.
    uint16_t C; ///< Third index in the group.

    /**
     * @brief Default constructor.
     */
    IndexGroup();

    /**
     * @brief Copy constructor.
     * @param indexGroup The IndexGroup to copy from.
     */
    IndexGroup(const IndexGroup& indexGroup);

    /**
     * @brief Parameterized constructor.
     * @param X Value for the first index.
     * @param Y Value for the second index.
     * @param Z Value for the third index.
     */
    IndexGroup(uint16_t X, uint16_t Y, uint16_t Z);

    /**
     * @brief Adds two IndexGroup objects component-wise.
     * @param indexGroup The IndexGroup to add.
     * @return A new IndexGroup with the result of the addition.
     */
    IndexGroup Add(IndexGroup indexGroup);

    /**
     * @brief Subtracts two IndexGroup objects component-wise.
     * @param indexGroup The IndexGroup to subtract.
     * @return A new IndexGroup with the result of the subtraction.
     */
    IndexGroup Subtract(IndexGroup indexGroup);

    /**
     * @brief Multiplies two IndexGroup objects component-wise.
     * @param indexGroup The IndexGroup to multiply.
     * @return A new IndexGroup with the result of the multiplication.
     */
    IndexGroup Multiply(IndexGroup indexGroup);

    /**
     * @brief Divides two IndexGroup objects component-wise.
     * @param indexGroup The IndexGroup to divide.
     * @return A new IndexGroup with the result of the division.
     */
    IndexGroup Divide(IndexGroup indexGroup);

    /**
     * @brief Converts the IndexGroup to a string representation.
     * @return A string representation of the IndexGroup in the format "[A, B, C]".
     */
    string_t ToString();
};
