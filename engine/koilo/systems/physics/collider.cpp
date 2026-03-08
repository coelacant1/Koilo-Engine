#include <ptx/systems/physics/collider.hpp>

namespace ptx {

Collider::Collider(ColliderType type)
    : type(type), isTrigger(false), isEnabled(true), layer(0),
      tag(""), material(), owner(nullptr) {
}

Collider::~Collider() {
}

void Collider::SetLayer(int l) {
    if (l >= 0 && l < 32) {
        layer = l;
    }
}

} // namespace ptx
