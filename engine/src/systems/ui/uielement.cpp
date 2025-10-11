#include <ptx/systems/ui/uielement.hpp>
#include <algorithm>

namespace ptx {

UIElement::UIElement()
    : parent(nullptr), position(0, 0), size(100, 100), pivot(0.5f, 0.5f),
      anchor(Anchor::Center()), rotation(0.0f), scale(1.0f, 1.0f),
      name("UIElement"), visible(true), enabled(true), interactable(true),
      zIndex(0), color(UIColor::White()), alpha(1.0f),
      worldRect(0, 0, 100, 100), rectDirty(true) {
}

UIElement::~UIElement() {
    RemoveAllChildren();
}

// === Hierarchy Management ===

void UIElement::AddChild(std::shared_ptr<UIElement> child) {
    if (child == nullptr) return;

    // Remove from previous parent
    if (child->parent != nullptr) {
        child->parent->RemoveChild(child);
    }

    child->parent = this;
    children.push_back(child);
    child->MarkRectDirty();
}

void UIElement::RemoveChild(std::shared_ptr<UIElement> child) {
    if (child == nullptr) return;

    auto it = std::find(children.begin(), children.end(), child);
    if (it != children.end()) {
        (*it)->parent = nullptr;
        children.erase(it);
    }
}

void UIElement::RemoveAllChildren() {
    for (auto& child : children) {
        if (child) {
            child->parent = nullptr;
        }
    }
    children.clear();
}

std::shared_ptr<UIElement> UIElement::GetChild(size_t index) const {
    if (index < children.size()) {
        return children[index];
    }
    return nullptr;
}

// === Transform ===

void UIElement::SetPosition(const Vector2D& pos) {
    position = pos;
    rectDirty = true;
}

void UIElement::SetSize(const Vector2D& s) {
    size = s;
    rectDirty = true;
}

void UIElement::SetAnchor(const Anchor& a) {
    anchor = a;
    rectDirty = true;
}

void UIElement::SetAlpha(float a) {
    alpha = std::clamp(a, 0.0f, 1.0f);
}

// === World Space Calculations ===

Rect UIElement::GetWorldRect() {
    if (rectDirty) {
        RecalculateWorldRect();
        rectDirty = false;
    }
    return worldRect;
}

bool UIElement::ContainsPoint(const Vector2D& point) {
    Rect rect = GetWorldRect();
    return rect.Contains(point);
}

void UIElement::RecalculateWorldRect() {
    // Get parent rect (or screen size if no parent)
    Rect parentRect(0, 0, 1920, 1080);  // Default screen size
    if (parent != nullptr) {
        parentRect = parent->GetWorldRect();
    }

    // Calculate anchored position
    float anchorMinX = parentRect.x + parentRect.width * anchor.minX;
    float anchorMinY = parentRect.y + parentRect.height * anchor.minY;
    float anchorMaxX = parentRect.x + parentRect.width * anchor.maxX;
    float anchorMaxY = parentRect.y + parentRect.height * anchor.maxY;

    // Calculate size based on anchors
    float width = size.x;
    float height = size.y;

    if (anchor.minX != anchor.maxX) {
        // Stretched horizontally
        width = (anchorMaxX - anchorMinX) + size.x;
    }
    if (anchor.minY != anchor.maxY) {
        // Stretched vertically
        height = (anchorMaxY - anchorMinY) + size.y;
    }

    // Apply scale
    width *= scale.x;
    height *= scale.y;

    // Calculate position with pivot
    float x = anchorMinX + position.x - (width * pivot.x);
    float y = anchorMinY + position.y - (height * pivot.y);

    worldRect = Rect(x, y, width, height);
}

// === Virtual Methods (Default Implementations) ===

void UIElement::Update(float deltaTime) {
    // Update all children
    for (auto& child : children) {
        if (child && child->IsEnabled()) {
            child->Update(deltaTime);
        }
    }
}

void UIElement::Render() {
    // Default render does nothing
    // Derived classes should override this
}

void UIElement::OnMouseEnter() {
    // Override in derived classes
}

void UIElement::OnMouseExit() {
    // Override in derived classes
}

void UIElement::OnClick() {
    // Override in derived classes
}

void UIElement::OnPress() {
    // Override in derived classes
}

void UIElement::OnRelease() {
    // Override in derived classes
}

} // namespace ptx
