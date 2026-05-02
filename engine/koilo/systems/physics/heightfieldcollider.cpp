// SPDX-License-Identifier: GPL-3.0-or-later
#include <koilo/systems/physics/heightfieldcollider.hpp>

namespace koilo {

HeightfieldCollider::HeightfieldCollider() : Collider(ColliderType::Heightfield) {}

HeightfieldCollider::HeightfieldCollider(std::shared_ptr<const HeightfieldData> data)
    : Collider(ColliderType::Heightfield), data_(std::move(data)) {}

HeightfieldCollider::~HeightfieldCollider() = default;

} // namespace koilo
