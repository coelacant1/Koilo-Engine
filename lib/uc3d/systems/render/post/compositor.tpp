#pragma once

template <size_t MaxEffects>
Compositor<MaxEffects>::Compositor() : list{ nullptr }, count(0) {}

template <size_t MaxEffects>
bool Compositor<MaxEffects>::Add(Effect* fx){
    if (count >= MaxEffects || fx == nullptr) return false;
    list[count++] = fx;
    return true;
}

template <size_t MaxEffects>
void Compositor<MaxEffects>::Clear(){
    count = 0;
}

template <size_t MaxEffects>
void Compositor<MaxEffects>::Process(IPixelGroup* pixelGroup){
    for (size_t i = 0; i < count; ++i) {
        list[i]->Apply(pixelGroup);
    }
}

