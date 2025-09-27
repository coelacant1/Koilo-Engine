#include <ptx/systems/render/post/effect.hpp>

void Effect::SetRatio(float r) {
    ratio = Mathematics::Constrain(r, 0.0f, 1.0f);
}
