// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file tagcomponent.hpp
 * @brief Tag component for ECS (string identifier).
 *
 * @date 11/10/2025
 * @author Coela
 */

#pragma once

#include <string>
#include <koilo/registry/reflect_macros.hpp>

namespace koilo {

/**
 * @struct TagComponent
 * @brief String tag/name for an entity.
 */
struct TagComponent {
    std::string tag;  ///< Entity tag/name

    /**
     * @brief Default constructor.
     */
    TagComponent() : tag("Entity") {}

    /**
     * @brief Constructor with tag.
     */
    TagComponent(const std::string& t) : tag(t) {}

    KL_BEGIN_FIELDS(TagComponent)
        KL_FIELD(TagComponent, tag, "Tag", 0, 0)
    KL_END_FIELDS

    KL_BEGIN_METHODS(TagComponent)
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(TagComponent)
        KL_CTOR0(TagComponent),
        KL_CTOR(TagComponent, std::string)
    KL_END_DESCRIBE(TagComponent)
};

} // namespace koilo
