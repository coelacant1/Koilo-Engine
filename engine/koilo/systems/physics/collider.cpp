// SPDX-License-Identifier: GPL-3.0-or-later
#include <koilo/systems/physics/collider.hpp>

namespace koilo {

koilo::Collider::Collider(ColliderType type)
    : type(type), isTrigger(false), isEnabled(true), layer(0),
      tag(""), material(), owner(nullptr), localOffset_() {
}

koilo::Collider::~Collider() {
}

void koilo::Collider::SetLayer(int l) {
    if (l >= 0 && l < 32) {
        layer = l;
    }
}

} // namespace koilo
