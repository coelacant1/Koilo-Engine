// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file allocation_tag.hpp
 * @brief Per-allocation tag for tracking and memory budgeting.
 *
 * @date 10/25/2025
 * @author Coela
 */
#pragma once
#include <cstdint>
#include "../../registry/reflect_macros.hpp"

namespace koilo {

/// Tag attached to every kernel allocation for tracking and budgeting.
struct AllocationTag {
    uint16_t moduleId = 0;
    uint16_t tag      = 0;

    KL_BEGIN_FIELDS(AllocationTag)
        KL_FIELD(AllocationTag, moduleId, "Module id", 0, 65535),
        KL_FIELD(AllocationTag, tag, "Tag", 0, 65535)
    KL_END_FIELDS

    KL_BEGIN_METHODS(AllocationTag)
        /* No reflected methods. */
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(AllocationTag)
        /* No reflected ctors. */
    KL_END_DESCRIBE(AllocationTag)

};

/// Common allocation tag categories.
namespace AllocTag {
    constexpr uint16_t General    = 0;
    constexpr uint16_t Frame      = 1;
    constexpr uint16_t Component  = 2;
    constexpr uint16_t Message    = 3;
    constexpr uint16_t Asset      = 4;
    constexpr uint16_t Scratch    = 5;
    constexpr uint16_t UserBase   = 0x1000;
}

} // namespace koilo
