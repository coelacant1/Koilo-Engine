// SPDX-License-Identifier: GPL-3.0-or-later
#include <koilo/systems/scene/sprite.hpp>
#include <koilo/systems/render/material/implementations/kslmaterial.hpp>

namespace koilo {

Sprite::Sprite() = default;

Sprite::Sprite(Texture* texture, float width, float height) {
    Build(texture, width, height);
}

Sprite::~Sprite() = default;

void Sprite::Init(Texture* texture, float width, float height) {
    if (initialized_) {
        material_.reset();
    }
    Build(texture, width, height);
}

void Sprite::Build(Texture* texture, float width, float height) {
    quad_.CreateTexturedQuad(width, height);
    texture_ = texture;
    material_ = std::make_unique<KSLMaterial>("texture");
    if (quad_.GetMesh()) {
        quad_.GetMesh()->SetMaterial(material_.get());
    }
    initialized_ = true;
}

Mesh* Sprite::GetMesh() {
    return quad_.GetMesh();
}

Transform* Sprite::GetTransform() {
    Mesh* m = quad_.GetMesh();
    return m ? m->GetTransform() : nullptr;
}

void Sprite::SetFrame(uint32_t x, uint32_t y, uint32_t w, uint32_t h) {
    if (!material_ || !texture_) return;
    float tw = static_cast<float>(texture_->GetWidth());
    float th = static_cast<float>(texture_->GetHeight());
    if (tw <= 0 || th <= 0) return;
    material_->SetFloat("frameX", x / tw);
    material_->SetFloat("frameY", y / th);
    material_->SetFloat("frameW", w / tw);
    material_->SetFloat("frameH", h / th);
}

void Sprite::SetFrameIndex(uint32_t index, uint32_t frameWidth, uint32_t frameHeight, uint32_t columns) {
    if (!material_ || columns == 0) return;
    uint32_t col = index % columns;
    uint32_t row = index / columns;
    SetFrame(col * frameWidth, row * frameHeight, frameWidth, frameHeight);
}

void Sprite::SetHueAngle(float degrees) {
    if (material_) material_->SetFloat("hueAngle", degrees);
}

void Sprite::SetEnabled(bool enabled) {
    Mesh* m = quad_.GetMesh();
    if (!m) return;
    if (enabled) m->Enable();
    else m->Disable();
}

bool Sprite::IsEnabled() const {
    Mesh* m = const_cast<PrimitiveMesh&>(quad_).GetMesh();
    return m ? m->IsEnabled() : false;
}

KSLMaterial* Sprite::GetMaterial() {
    return material_.get();
}

} // namespace koilo
