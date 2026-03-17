// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file sprite.hpp
 * @brief High-level sprite abstraction for 2D rendering.
 *
 * A Sprite is a textured quad with convenience methods for sprite sheet
 * frame selection and animation. It owns a PrimitiveMesh (textured quad)
 * and a KSLMaterial("texture"), and exposes Transform for positioning.
 *
 * Usage from KoiloScript:
 *   var sprite = Sprite(myTexture, 32, 32);
 *   sprite.SetFrame(0, 0, 32, 32);
 *   sprite.GetTransform()->SetPosition(Vector3D(100, 50, 0));
 *   scene.AddMesh(sprite.GetMesh());
 *
 * @date 2026-02-21
 * @author Coela
 */
#pragma once

#include <koilo/assets/image/texture.hpp>
#include <koilo/systems/scene/primitivemesh.hpp>
#include <koilo/core/math/transform.hpp>
#include <koilo/registry/reflect_macros.hpp>
#include <memory>

namespace koilo {

class KSLMaterial;

class Sprite {
public:
    // Create a sprite from a texture with given display size in world units.
    Sprite(Texture* texture, float width, float height);

    // Default constructor (must call Init before use).
    Sprite();
    ~Sprite();

    // Initialize after default construction.
    void Init(Texture* texture, float width, float height);

    // Get the underlying mesh for adding to a Scene.
    Mesh* GetMesh();

    // Get the transform for positioning/scaling.
    Transform* GetTransform();

    // Set sprite sheet frame rectangle (in texture pixels).
    void SetFrame(uint32_t x, uint32_t y, uint32_t w, uint32_t h);

    // Advance to next frame in a horizontal strip.
    // frameWidth/frameHeight define frame size, columns is frames per row.
    void SetFrameIndex(uint32_t index, uint32_t frameWidth, uint32_t frameHeight, uint32_t columns);

    // Set hue shift on the material.
    void SetHueAngle(float degrees);

    // Enable/disable rendering.
    void SetEnabled(bool enabled);
    bool IsEnabled() const;

    // Get the material (for advanced configuration).
    KSLMaterial* GetMaterial();

private:
    PrimitiveMesh quad_;
    std::unique_ptr<KSLMaterial> material_;
    Texture* texture_ = nullptr;
    bool initialized_ = false;

    void Build(Texture* texture, float width, float height);

public:
    KL_BEGIN_FIELDS(Sprite)
    KL_END_FIELDS

    KL_BEGIN_METHODS(Sprite)
        KL_METHOD_AUTO(Sprite, GetMesh, "Get mesh"),
        KL_METHOD_AUTO(Sprite, GetTransform, "Get transform"),
        KL_METHOD_AUTO(Sprite, SetFrame, "Set frame rect"),
        KL_METHOD_AUTO(Sprite, SetFrameIndex, "Set frame by index"),
        KL_METHOD_AUTO(Sprite, SetHueAngle, "Set hue angle"),
        KL_METHOD_AUTO(Sprite, SetEnabled, "Set enabled"),
        KL_METHOD_AUTO(Sprite, IsEnabled, "Is enabled"),
        KL_METHOD_AUTO(Sprite, GetMaterial, "Get material"),
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(Sprite)
        KL_CTOR0(Sprite),
        KL_CTOR(Sprite, Texture*, float, float)
    KL_END_DESCRIBE(Sprite)
};

} // namespace koilo
