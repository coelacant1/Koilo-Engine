#pragma once

template <uint8_t MaxEffects>
Compositor<MaxEffects>::Compositor() {
    for (uint8_t i = 0; i < MaxEffects; ++i) {
        effects[i] = nullptr;
        enabled[i] = false;
    }
}

template <uint8_t MaxEffects>
bool Compositor<MaxEffects>::AddEffect(Effect* fx, bool enable) {
    if (count >= MaxEffects || fx == nullptr) return false;
    effects[count] = fx;
    enabled[count] = enable;
    ++count;
    return true;
}

template <uint8_t MaxEffects>
void Compositor<MaxEffects>::SetEnabled(uint8_t index, bool state) {
    if (index < count) enabled[index] = state;
}

template <uint8_t MaxEffects>
void Compositor<MaxEffects>::Clear() {
    for (uint8_t i = 0; i < MaxEffects; ++i) {
        effects[i] = nullptr;
        enabled[i] = false;
    }
    count = 0;
}

template <uint8_t MaxEffects>
void Compositor<MaxEffects>::Apply(IPixelGroup* pixelGroup) {
    if (!pixelGroup) return;
    for (uint8_t i = 0; i < count; ++i) {
        if (enabled[i] && effects[i]) {
            effects[i]->Apply(pixelGroup);
        }
    }
}
