#include <ptx/systems/ui/canvas.hpp>
#include <algorithm>
#include <cmath>

namespace ptx {

Canvas::Canvas()
    : renderMode(RenderMode::ScreenSpaceOverlay), sortOrder(0),
      scaleMode(ScaleMode::ScaleWithScreenSize), referenceResolution(1920, 1080),
      scaleFactor(1.0f), matchWidthOrHeight(0.5f), screenSize(1920, 1080),
      hoveredElement(nullptr), pressedElement(nullptr), lastMousePosition(0, 0) {

    // Set canvas size to screen size
    SetSize(screenSize);
}

Canvas::~Canvas() {
}

// === Scaling ===

void Canvas::SetReferenceResolution(const Vector2D& resolution) {
    referenceResolution = resolution;
    UpdateScaleFactor();
}

void Canvas::SetMatchWidthOrHeight(float match) {
    matchWidthOrHeight = std::clamp(match, 0.0f, 1.0f);
    UpdateScaleFactor();
}

void Canvas::SetScreenSize(const Vector2D& size) {
    screenSize = size;
    SetSize(size);
    UpdateScaleFactor();
}

void Canvas::UpdateScaleFactor() {
    if (scaleMode == ScaleMode::ConstantPixelSize) {
        scaleFactor = 1.0f;
    } else if (scaleMode == ScaleMode::ScaleWithScreenSize) {
        float widthScale = screenSize.x / referenceResolution.x;
        float heightScale = screenSize.y / referenceResolution.y;

        // Interpolate between width and height scale based on matchWidthOrHeight
        scaleFactor = widthScale * (1.0f - matchWidthOrHeight) + heightScale * matchWidthOrHeight;
    } else if (scaleMode == ScaleMode::ConstantPhysicalSize) {
        // TODO: Implement DPI-aware scaling
        scaleFactor = 1.0f;
    }
}

// === Input Handling ===

void Canvas::ProcessMouseMove(const Vector2D& mousePos) {
    lastMousePosition = mousePos;

    // Raycast to find element under mouse
    UIElement* element = RaycastUI(mousePos);

    // Handle hover changes
    if (element != hoveredElement) {
        if (hoveredElement != nullptr) {
            hoveredElement->OnMouseExit();
        }

        hoveredElement = element;

        if (hoveredElement != nullptr) {
            hoveredElement->OnMouseEnter();
        }
    }
}

void Canvas::ProcessMousePress(const Vector2D& mousePos, int button) {
    // Raycast to find element under mouse
    UIElement* element = RaycastUI(mousePos);

    if (element != nullptr && element->IsInteractable()) {
        pressedElement = element;
        pressedElement->OnPress();
    }
}

void Canvas::ProcessMouseRelease(const Vector2D& mousePos, int button) {
    if (pressedElement != nullptr) {
        pressedElement->OnRelease();

        // Check if release happened over the same element (click)
        UIElement* element = RaycastUI(mousePos);
        if (element == pressedElement) {
            pressedElement->OnClick();
        }

        pressedElement = nullptr;
    }
}

UIElement* Canvas::RaycastUI(const Vector2D& position) {
    UIElement* bestElement = nullptr;
    int bestZIndex = -10000;

    RaycastRecursive(this, position, bestElement, bestZIndex);

    return bestElement;
}

void Canvas::RaycastRecursive(UIElement* element, const Vector2D& position,
                              UIElement*& bestElement, int& bestZIndex) {
    if (element == nullptr || !element->IsVisible() || !element->IsEnabled()) {
        return;
    }

    // Check if point is inside this element
    if (element->ContainsPoint(position)) {
        if (element->IsInteractable() && element->GetZIndex() >= bestZIndex) {
            bestElement = element;
            bestZIndex = element->GetZIndex();
        }
    }

    // Check children
    const auto& children = element->GetChildren();
    for (const auto& child : children) {
        if (child) {
            RaycastRecursive(child.get(), position, bestElement, bestZIndex);
        }
    }
}

// === Rendering and Update ===

void Canvas::Update(float deltaTime) {
    // Update base element
    UIElement::Update(deltaTime);

    // Update all children recursively
    UpdateRecursive(this, deltaTime);
}

void Canvas::UpdateRecursive(UIElement* element, float deltaTime) {
    if (element == nullptr || !element->IsEnabled()) {
        return;
    }

    element->Update(deltaTime);

    const auto& children = element->GetChildren();
    for (const auto& child : children) {
        if (child) {
            UpdateRecursive(child.get(), deltaTime);
        }
    }
}

void Canvas::Render() {
    if (!IsVisible()) return;

    // Sort children by z-index before rendering
    auto sortedChildren = children;
    SortByZIndex(sortedChildren);

    // Render base element
    UIElement::Render();

    // Render all children recursively
    for (const auto& child : sortedChildren) {
        if (child) {
            RenderRecursive(child.get());
        }
    }
}

void Canvas::RenderRecursive(UIElement* element) {
    if (element == nullptr || !element->IsVisible()) {
        return;
    }

    element->Render();

    // Sort children by z-index
    auto children = element->GetChildren();
    SortByZIndex(children);

    for (const auto& child : children) {
        if (child) {
            RenderRecursive(child.get());
        }
    }
}

void Canvas::SortByZIndex(std::vector<std::shared_ptr<UIElement>>& elements) {
    std::sort(elements.begin(), elements.end(),
        [](const std::shared_ptr<UIElement>& a, const std::shared_ptr<UIElement>& b) {
            return a->GetZIndex() < b->GetZIndex();
        });
}

} // namespace ptx
