#include <ptx/systems/render/post/compositor.hpp>

#include <algorithm>

Compositor::Compositor(uint8_t maxEffects)
    : capacity_(maxEffects),
      effects_(static_cast<size_t>(maxEffects), nullptr),
      enabled_(static_cast<size_t>(maxEffects), false) {}

bool Compositor::AddEffect(Effect* fx, bool enable) {
    if (!fx || count_ >= capacity_) {
        return false;
    }

    effects_[count_] = fx;
    enabled_[count_] = enable;
    ++count_;
    return true;
}

void Compositor::SetEnabled(uint8_t index, bool state) {
    if (index < count_) {
        enabled_[index] = state;
    }
}

void Compositor::Clear() {
    std::fill(effects_.begin(), effects_.end(), nullptr);
    std::fill(enabled_.begin(), enabled_.end(), false);
    count_ = 0;
}

void Compositor::Apply(IPixelGroup* pixelGroup) {
    if (!pixelGroup) {
        return;
    }

    for (uint8_t i = 0; i < count_; ++i) {
        if (enabled_[i] && effects_[i]) {
            effects_[i]->Apply(pixelGroup);
        }
    }
}
