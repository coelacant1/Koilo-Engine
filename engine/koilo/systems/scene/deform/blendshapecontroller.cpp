#include <ptx/systems/scene/deform/blendshapecontroller.hpp>

#include <utility>

BlendshapeController::BlendshapeController(IEasyEaseAnimator* eEA, std::size_t maxBlendshapes)
: eEA_(eEA)
, capacity_(maxBlendshapes)
, dictionary_(maxBlendshapes, 0)
, positionOffsets_(maxBlendshapes)
, scaleOffsets_(maxBlendshapes)
, rotationOffsets_(maxBlendshapes) {
}

BlendshapeController::Index BlendshapeController::FindIndex(uint16_t dictionaryValue) const {
    for (Index i = 0; i < currentBlendshapes_; ++i) {
        if (dictionary_[i] == dictionaryValue) {
            return i;
        }
    }
    return kInvalidIndex;
}

void BlendshapeController::AddBlendshape(uint16_t dictionaryValue, Vector3D positionOffset) {
    AddBlendshape(dictionaryValue, std::move(positionOffset), Vector3D(1.0f, 1.0f, 1.0f), Vector3D());
}

void BlendshapeController::AddBlendshape(uint16_t dictionaryValue, Vector3D positionOffset, Vector3D scaleOffset) {
    AddBlendshape(dictionaryValue, std::move(positionOffset), std::move(scaleOffset), Vector3D());
}

void BlendshapeController::AddBlendshape(uint16_t dictionaryValue,
                                         Vector3D positionOffset,
                                         Vector3D scaleOffset,
                                         Vector3D rotationOffset) {
    if (capacity_ == 0 || currentBlendshapes_ >= capacity_) {
        return;
    }

    if (FindIndex(dictionaryValue) != kInvalidIndex) {
        return;
    }

    const Index idx = currentBlendshapes_;
    dictionary_[idx]       = dictionaryValue;
    positionOffsets_[idx]  = std::move(positionOffset);
    scaleOffsets_[idx]     = std::move(scaleOffset);
    rotationOffsets_[idx]  = std::move(rotationOffset);
    ++currentBlendshapes_;
}

void BlendshapeController::SetBlendshapePositionOffset(uint16_t dictionaryValue, Vector3D positionOffset) {
    const Index index = FindIndex(dictionaryValue);
    if (index == kInvalidIndex) {
        return;
    }
    positionOffsets_[index] = std::move(positionOffset);
}

void BlendshapeController::SetBlendshapeScaleOffset(uint16_t dictionaryValue, Vector3D scaleOffset) {
    const Index index = FindIndex(dictionaryValue);
    if (index == kInvalidIndex) {
        return;
    }
    scaleOffsets_[index] = std::move(scaleOffset);
}

void BlendshapeController::SetBlendshapeRotationOffset(uint16_t dictionaryValue, Vector3D rotationOffset) {
    const Index index = FindIndex(dictionaryValue);
    if (index == kInvalidIndex) {
        return;
    }
    rotationOffsets_[index] = std::move(rotationOffset);
}

Vector3D BlendshapeController::GetPositionOffset() {
    Vector3D positionOffset;

    if (!eEA_) {
        return positionOffset;
    }

    for (Index i = 0; i < currentBlendshapes_; ++i) {
        const float weight = eEA_->GetValue(dictionary_[i]);
        if (weight > 0.0f) {
            positionOffset += positionOffsets_[i] * weight;
        }
    }

    return positionOffset;
}

Vector3D BlendshapeController::GetScaleOffset() {
    Vector3D scaleOffset(1.0f, 1.0f, 1.0f);

    if (!eEA_) {
        return scaleOffset;
    }

    std::size_t count = 0;

    for (Index i = 0; i < currentBlendshapes_; ++i) {
        const float weight = eEA_->GetValue(dictionary_[i]);
        if (weight > 0.0f) {
            scaleOffset = scaleOffset * Vector3D::LERP(Vector3D(1.0f, 1.0f, 1.0f), scaleOffsets_[i], weight);
            ++count;
        }
    }

    if (count == 0) {
        return Vector3D(1.0f, 1.0f, 1.0f);
    }

    return scaleOffset;
}

Vector3D BlendshapeController::GetRotationOffset() {
    Vector3D rotationOffset;

    if (!eEA_) {
        return rotationOffset;
    }

    for (Index i = 0; i < currentBlendshapes_; ++i) {
        const float weight = eEA_->GetValue(dictionary_[i]);
        if (weight > 0.0f) {
            rotationOffset += rotationOffsets_[i] * weight;
        }
    }

    return rotationOffset;
}
