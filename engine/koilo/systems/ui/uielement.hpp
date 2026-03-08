// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file uielement.hpp
 * @brief Base class for all UI elements in the user interface system.
 *
 * @date 11/10/2025
 * @author Coela
 */

#pragma once

#include <vector>
#include <memory>
#include <string>
#include <functional>
#include "../../core/math/vector2d.hpp"
#include "../../registry/reflect_macros.hpp"

namespace koilo {


// Forward declarations
class Canvas;

/**
 * @struct Rect
 * @brief Rectangle structure for UI positioning and sizing.
 */
struct Rect {
    float x, y;        // Position
    float width, height;

    Rect() : x(0), y(0), width(0), height(0) {}
    Rect(float x, float y, float width, float height) : x(x), y(y), width(width), height(height) {}

    bool Contains(float px, float py) const {
        return px >= x && px <= x + width && py >= y && py <= y + height;
    }

    bool Contains(const Vector2D& point) const {
        return Contains(point.X, point.Y);
    }
};

/**
 * @struct Anchor
 * @brief Anchor point for responsive UI positioning.
 */
struct Anchor {
    float minX, minY;  // 0.0 = left/bottom, 1.0 = right/top
    float maxX, maxY;

    Anchor() : minX(0.5f), minY(0.5f), maxX(0.5f), maxY(0.5f) {}
    Anchor(float minX, float minY, float maxX, float maxY)
        : minX(minX), minY(minY), maxX(maxX), maxY(maxY) {}

    // Common anchor presets
    static Anchor TopLeft() { return Anchor(0.0f, 1.0f, 0.0f, 1.0f); }
    static Anchor TopCenter() { return Anchor(0.5f, 1.0f, 0.5f, 1.0f); }
    static Anchor TopRight() { return Anchor(1.0f, 1.0f, 1.0f, 1.0f); }
    static Anchor MiddleLeft() { return Anchor(0.0f, 0.5f, 0.0f, 0.5f); }
    static Anchor Center() { return Anchor(0.5f, 0.5f, 0.5f, 0.5f); }
    static Anchor MiddleRight() { return Anchor(1.0f, 0.5f, 1.0f, 0.5f); }
    static Anchor BottomLeft() { return Anchor(0.0f, 0.0f, 0.0f, 0.0f); }
    static Anchor BottomCenter() { return Anchor(0.5f, 0.0f, 0.5f, 0.0f); }
    static Anchor BottomRight() { return Anchor(1.0f, 0.0f, 1.0f, 0.0f); }
    static Anchor StretchAll() { return Anchor(0.0f, 0.0f, 1.0f, 1.0f); }
};

/**
 * @struct UIColor
 * @brief RGBA color for UI rendering.
 */
struct UIColor {
    float r, g, b, a;

    UIColor() : r(1.0f), g(1.0f), b(1.0f), a(1.0f) {}
    UIColor(float r, float g, float b, float a = 1.0f) : r(r), g(g), b(b), a(a) {}

    static UIColor White() { return UIColor(1, 1, 1, 1); }
    static UIColor Black() { return UIColor(0, 0, 0, 1); }
    static UIColor Transparent() { return UIColor(0, 0, 0, 0); }
    static UIColor Red() { return UIColor(1, 0, 0, 1); }
    static UIColor Green() { return UIColor(0, 1, 0, 1); }
    static UIColor Blue() { return UIColor(0, 0, 1, 1); }
};

/**
 * @class UIElement
 * @brief Base class for all UI elements with hierarchical structure.
 */
class UIElement {
protected:
    // Hierarchy
    UIElement* parent;
    std::vector<std::shared_ptr<UIElement>> children;

    // Transform
    Vector2D position;        // Position relative to anchor
    Vector2D size;            // Size of the element
    Vector2D pivot;           // Pivot point (0-1, default 0.5, 0.5 = center)
    Anchor anchor;            // Anchor to parent
    float rotation;           // Rotation in degrees
    Vector2D scale;           // Scale factor

    // Properties
    std::string name;
    bool visible;
    bool enabled;
    bool interactable;        // Can receive input events
    int zIndex;               // Rendering order (higher = drawn later = on top)

    // Style
    UIColor color;
    float alpha;              // Overall alpha multiplier

    // Cached values
    Rect worldRect;           // Cached world-space rect
    bool rectDirty;           // Whether worldRect needs recalculation

public:
    /**
     * @brief Default constructor.
     */
    UIElement();

    /**
     * @brief Virtual destructor.
     */
    virtual ~UIElement();

    // === Hierarchy Management ===

    /**
     * @brief Adds a child element.
     * @param child The child element to add.
     */
    void AddChild(std::shared_ptr<UIElement> child);

    /**
     * @brief Removes a child element.
     * @param child The child element to remove.
     */
    void RemoveChild(std::shared_ptr<UIElement> child);

    /**
     * @brief Removes all children.
     */
    void RemoveAllChildren();

    /**
     * @brief Gets a child by index.
     * @param index The child index.
     * @return The child element, or nullptr if out of bounds.
     */
    std::shared_ptr<UIElement> GetChild(size_t index) const;

    /**
     * @brief Gets the number of children.
     */
    size_t GetChildCount() const { return children.size(); }

    /**
     * @brief Gets all children.
     */
    const std::vector<std::shared_ptr<UIElement>>& GetChildren() const { return children; }

    /**
     * @brief Gets the parent element.
     */
    UIElement* GetParent() const { return parent; }

    // === Transform ===

    /**
     * @brief Sets the position.
     */
    void SetPosition(const Vector2D& pos);

    /**
     * @brief Gets the position.
     */
    Vector2D GetPosition() const { return position; }

    /**
     * @brief Sets the size.
     */
    void SetSize(const Vector2D& s);

    /**
     * @brief Gets the size.
     */
    Vector2D GetSize() const { return size; }

    /**
     * @brief Sets the anchor.
     */
    void SetAnchor(const Anchor& a);

    /**
     * @brief Gets the anchor.
     */
    Anchor GetAnchor() const { return anchor; }

    /**
     * @brief Sets the pivot point.
     */
    void SetPivot(const Vector2D& p) { pivot = p; rectDirty = true; }

    /**
     * @brief Gets the pivot point.
     */
    Vector2D GetPivot() const { return pivot; }

    /**
     * @brief Sets the rotation in degrees.
     */
    void SetRotation(float rot) { rotation = rot; rectDirty = true; }

    /**
     * @brief Gets the rotation.
     */
    float GetRotation() const { return rotation; }

    /**
     * @brief Sets the scale.
     */
    void SetScale(const Vector2D& s) { scale = s; rectDirty = true; }

    /**
     * @brief Gets the scale.
     */
    Vector2D GetScale() const { return scale; }

    // === Properties ===

    /**
     * @brief Sets the name.
     */
    void SetName(const std::string& n) { name = n; }

    /**
     * @brief Gets the name.
     */
    std::string GetName() const { return name; }

    /**
     * @brief Sets visibility.
     */
    void SetVisible(bool v) { visible = v; }

    /**
     * @brief Checks if visible.
     */
    bool IsVisible() const { return visible; }

    /**
     * @brief Sets enabled state.
     */
    void SetEnabled(bool e) { enabled = e; }

    /**
     * @brief Checks if enabled.
     */
    bool IsEnabled() const { return enabled; }

    /**
     * @brief Sets interactable state.
     */
    void SetInteractable(bool i) { interactable = i; }

    /**
     * @brief Checks if interactable.
     */
    bool IsInteractable() const { return interactable; }

    /**
     * @brief Sets the z-index.
     */
    void SetZIndex(int z) { zIndex = z; }

    /**
     * @brief Gets the z-index.
     */
    int GetZIndex() const { return zIndex; }

    // === Style ===

    /**
     * @brief Sets the color.
     */
    void SetColor(const UIColor& c) { color = c; }

    /**
     * @brief Gets the color.
     */
    UIColor GetColor() const { return color; }

    /**
     * @brief Sets the alpha.
     */
    void SetAlpha(float a);

    /**
     * @brief Gets the alpha.
     */
    float GetAlpha() const { return alpha; }

    // === World Space Calculations ===

    /**
     * @brief Gets the world-space rect.
     */
    Rect GetWorldRect();

    /**
     * @brief Checks if a point is inside this element.
     * @param point The point in screen space.
     */
    bool ContainsPoint(const Vector2D& point);

    // === Virtual Methods ===

    /**
     * @brief Updates the element (called each frame).
     * @param deltaTime Time since last update.
     */
    virtual void Update(float deltaTime);

    /**
     * @brief Renders the element (override in derived classes).
     */
    virtual void Render();

    /**
     * @brief Called when the mouse enters the element.
     */
    virtual void OnMouseEnter();

    /**
     * @brief Called when the mouse exits the element.
     */
    virtual void OnMouseExit();

    /**
     * @brief Called when the element is clicked.
     */
    virtual void OnClick();

    /**
     * @brief Called when the element is pressed.
     */
    virtual void OnPress();

    /**
     * @brief Called when the element is released.
     */
    virtual void OnRelease();

protected:
    /**
     * @brief Recalculates the world rect.
     */
    void RecalculateWorldRect();

    /**
     * @brief Marks the rect as dirty (needs recalculation).
     */
    void MarkRectDirty() { rectDirty = true; }

    KL_BEGIN_FIELDS(UIElement)
        KL_FIELD(UIElement, position, "Position", 0, 0),
        KL_FIELD(UIElement, size, "Size", 0, 0),
        KL_FIELD(UIElement, rotation, "Rotation", 0.0f, 360.0f),
        KL_FIELD(UIElement, scale, "Scale", 0, 0),
        KL_FIELD(UIElement, visible, "Visible", 0, 1),
        KL_FIELD(UIElement, enabled, "Enabled", 0, 1),
        KL_FIELD(UIElement, interactable, "Interactable", 0, 1),
        KL_FIELD(UIElement, zIndex, "Z index", -100, 100),
        KL_FIELD(UIElement, alpha, "Alpha", 0.0f, 1.0f)
    KL_END_FIELDS

    KL_BEGIN_METHODS(UIElement)
        KL_METHOD_AUTO(UIElement, SetPosition, "Set position"),
        KL_METHOD_AUTO(UIElement, GetPosition, "Get position"),
        KL_METHOD_AUTO(UIElement, SetSize, "Set size"),
        KL_METHOD_AUTO(UIElement, GetSize, "Get size"),
        KL_METHOD_AUTO(UIElement, SetVisible, "Set visible"),
        KL_METHOD_AUTO(UIElement, IsVisible, "Is visible"),
        KL_METHOD_AUTO(UIElement, SetEnabled, "Set enabled"),
        KL_METHOD_AUTO(UIElement, IsEnabled, "Is enabled"),
        KL_METHOD_AUTO(UIElement, Update, "Update"),
        KL_METHOD_AUTO(UIElement, Render, "Render")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(UIElement)
        KL_CTOR0(UIElement)
    KL_END_DESCRIBE(UIElement)
};


} // namespace koilo
