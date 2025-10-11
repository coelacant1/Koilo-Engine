/**
 * @file tagcomponent.hpp
 * @brief Tag component for ECS (string identifier).
 *
 * @date 11/10/2025
 * @author Coela
 */

#pragma once

#include <string>
#include "../../../registry/reflect_macros.hpp"

namespace ptx {

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

    PTX_BEGIN_FIELDS(TagComponent)
        PTX_FIELD(TagComponent, tag, "Tag", 0, 0)
    PTX_END_FIELDS

    PTX_BEGIN_METHODS(TagComponent)
    PTX_END_METHODS

    PTX_BEGIN_DESCRIBE(TagComponent)
        PTX_CTOR0(TagComponent),
        PTX_CTOR(TagComponent, std::string)
    PTX_END_DESCRIBE(TagComponent)
};

} // namespace ptx
