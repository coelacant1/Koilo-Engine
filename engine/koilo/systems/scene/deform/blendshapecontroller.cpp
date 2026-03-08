// SPDX-License-Identifier: GPL-3.0-or-later
#include <koilo/systems/scene/deform/blendshapecontroller.hpp>


namespace koilo {

koilo::BlendshapeController::BlendshapeController(IEasyEaseAnimator* animator, std::size_t capacity)
    : animator_(animator)
    , capacity_(capacity) {
    blendshapes_.reserve(capacity);
    dictionaryIds_.reserve(capacity);
}

void koilo::BlendshapeController::SetAnimator(IEasyEaseAnimator* animator) {
    animator_ = animator;
}

bool koilo::BlendshapeController::AddBlendshape(uint16_t dictionaryId, Blendshape* blendshape) {
    if (!blendshape || blendshapes_.size() >= capacity_) {
        return false;
    }

    // Check if dictionary ID already exists
    if (FindIndex(dictionaryId) != -1) {
        return false;
    }

    blendshapes_.push_back(blendshape);
    dictionaryIds_.push_back(dictionaryId);
    return true;
}

bool koilo::BlendshapeController::RemoveBlendshape(uint16_t dictionaryId) {
    int index = FindIndex(dictionaryId);
    if (index == -1) {
        return false;
    }

    blendshapes_.erase(blendshapes_.begin() + index);
    dictionaryIds_.erase(dictionaryIds_.begin() + index);
    return true;
}

std::size_t koilo::BlendshapeController::GetBlendshapeCount() const {
    return blendshapes_.size();
}

std::size_t koilo::BlendshapeController::GetCapacity() const {
    return capacity_;
}

void koilo::BlendshapeController::SetWeight(uint16_t dictionaryId, float weight) {
    int index = FindIndex(dictionaryId);
    if (index != -1) {
        blendshapes_[index]->Weight = weight;
    }
}

float koilo::BlendshapeController::GetWeight(uint16_t dictionaryId) const {
    int index = FindIndex(dictionaryId);
    if (index == -1) {
        return 0.0f;
    }

    // Prefer animator weight if available
    if (animator_) {
        return animator_->GetValue(dictionaryId);
    }

    return blendshapes_[index]->Weight;
}

void koilo::BlendshapeController::ResetWeights() {
    for (auto* blendshape : blendshapes_) {
        if (blendshape) {
            blendshape->Weight = 0.0f;
        }
    }
}

void koilo::BlendshapeController::Update(Mesh* mesh) {
    if (!mesh) {
        return;
    }

    // Reset mesh to original vertices
    mesh->ResetVertices();

    // Apply all blendshapes
    ApplyTo(mesh->GetTriangleGroup());
}

void koilo::BlendshapeController::ApplyTo(ITriangleGroup* triangleGroup) {
    if (!triangleGroup) {
        return;
    }

    for (std::size_t i = 0; i < blendshapes_.size(); ++i) {
        Blendshape* blendshape = blendshapes_[i];
        if (!blendshape) {
            continue;
        }

        // Get weight from animator if available, otherwise use blendshape's weight
        float weight = animator_ ? animator_->GetValue(dictionaryIds_[i]) : blendshape->Weight;

        // Only apply if weight is non-zero
        if (weight != 0.0f) {
            // Temporarily set the weight and apply
            float originalWeight = blendshape->Weight;
            blendshape->Weight = weight;
            blendshape->BlendObject3D(triangleGroup);
            blendshape->Weight = originalWeight;
        }
    }
}

int koilo::BlendshapeController::FindIndex(uint16_t dictionaryId) const {
    for (std::size_t i = 0; i < dictionaryIds_.size(); ++i) {
        if (dictionaryIds_[i] == dictionaryId) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

} // namespace koilo
