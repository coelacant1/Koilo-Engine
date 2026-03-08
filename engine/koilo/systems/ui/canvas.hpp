/**
 * @file canvas.hpp
 * @brief Root UI container that handles rendering and input for UI hierarchy.
 *
 * @date 11/10/2025
 * @author Coela
 */

#pragma once

#include <memory>
#include <vector>
#include "uielement.hpp"
#include "../../core/mathematics/vector2d.hpp"
#include "../../../registry/reflect_macros.hpp"

namespace ptx {

/**
 * @enum RenderMode
 * @brief Rendering mode for the canvas.
 */
enum class RenderMode : uint8_t {
    ScreenSpaceOverlay = 0,   // Renders on top of everything, ignores 3D scene
    ScreenSpaceCamera = 1,    // Renders in screen space but respects camera
    WorldSpace = 2            // Renders as part of the 3D world
};

/**
 * @enum ScaleMode
 * @brief How the canvas scales with screen resolution changes.
 */
enum class ScaleMode : uint8_t {
    ConstantPixelSize = 0,    // UI stays same pixel size
    ScaleWithScreenSize = 1,  // UI scales with screen resolution
    ConstantPhysicalSize = 2  // UI maintains physical size (DPI-aware)
};

/**
 * @class Canvas
 * @brief Root container for UI elements with rendering and input management.
 */
class Canvas : public UIElement {
private:
    // Rendering
    RenderMode renderMode;
    int sortOrder;            // Canvas sort order (for multiple canvases)

    // Scaling
    ScaleMode scaleMode;
    Vector2D referenceResolution;  // Reference resolution for scaling
    float scaleFactor;             // Current scale factor
    float matchWidthOrHeight;      // 0 = match width, 1 = match height, 0.5 = average

    // Screen size
    Vector2D screenSize;

    // Input
    UIElement* hoveredElement;     // Currently hovered element
    UIElement* pressedElement;     // Currently pressed element
    Vector2D lastMousePosition;

public:
    /**
     * @brief Default constructor.
     */
    Canvas();

    /**
     * @brief Virtual destructor.
     */
    virtual ~Canvas();

    // === Rendering ===

    /**
     * @brief Sets the render mode.
     */
    void SetRenderMode(RenderMode mode) { renderMode = mode; }

    /**
     * @brief Gets the render mode.
     */
    RenderMode GetRenderMode() const { return renderMode; }

    /**
     * @brief Sets the sort order.
     */
    void SetSortOrder(int order) { sortOrder = order; }

    /**
     * @brief Gets the sort order.
     */
    int GetSortOrder() const { return sortOrder; }

    // === Scaling ===

    /**
     * @brief Sets the scale mode.
     */
    void SetScaleMode(ScaleMode mode) { scaleMode = mode; }

    /**
     * @brief Gets the scale mode.
     */
    ScaleMode GetScaleMode() const { return scaleMode; }

    /**
     * @brief Sets the reference resolution.
     */
    void SetReferenceResolution(const Vector2D& resolution);

    /**
     * @brief Gets the reference resolution.
     */
    Vector2D GetReferenceResolution() const { return referenceResolution; }

    /**
     * @brief Sets the match width or height factor.
     */
    void SetMatchWidthOrHeight(float match);

    /**
     * @brief Gets the match width or height factor.
     */
    float GetMatchWidthOrHeight() const { return matchWidthOrHeight; }

    /**
     * @brief Gets the current scale factor.
     */
    float GetScaleFactor() const { return scaleFactor; }

    // === Screen ===

    /**
     * @brief Sets the screen size (called when window resizes).
     */
    void SetScreenSize(const Vector2D& size);

    /**
     * @brief Gets the screen size.
     */
    Vector2D GetScreenSize() const { return screenSize; }

    // === Input Handling ===

    /**
     * @brief Processes mouse movement.
     * @param mousePos Mouse position in screen space.
     */
    void ProcessMouseMove(const Vector2D& mousePos);

    /**
     * @brief Processes mouse button press.
     * @param mousePos Mouse position in screen space.
     * @param button Mouse button (0 = left, 1 = right, 2 = middle).
     */
    void ProcessMousePress(const Vector2D& mousePos, int button);

    /**
     * @brief Processes mouse button release.
     * @param mousePos Mouse position in screen space.
     * @param button Mouse button.
     */
    void ProcessMouseRelease(const Vector2D& mousePos, int button);

    // === Rendering ===

    /**
     * @brief Renders the entire canvas hierarchy.
     */
    virtual void Render() override;

    /**
     * @brief Updates the entire canvas hierarchy.
     * @param deltaTime Time since last update.
     */
    virtual void Update(float deltaTime) override;

private:
    /**
     * @brief Raycast to find UI element at position.
     * @param position Mouse position in screen space.
     * @return The topmost element at the position, or nullptr.
     */
    UIElement* RaycastUI(const Vector2D& position);

    /**
     * @brief Recursively raycast through hierarchy.
     * @param element Element to check.
     * @param position Mouse position.
     * @param bestElement Current best element found.
     * @param bestZIndex Current best z-index.
     */
    void RaycastRecursive(UIElement* element, const Vector2D& position,
                          UIElement*& bestElement, int& bestZIndex);

    /**
     * @brief Updates the scale factor based on screen size.
     */
    void UpdateScaleFactor();

    /**
     * @brief Recursively updates all children.
     * @param element Element to update.
     * @param deltaTime Time since last update.
     */
    void UpdateRecursive(UIElement* element, float deltaTime);

    /**
     * @brief Recursively renders all children.
     * @param element Element to render.
     */
    void RenderRecursive(UIElement* element);

    /**
     * @brief Sorts elements by z-index for rendering.
     * @param elements Elements to sort.
     */
    void SortByZIndex(std::vector<std::shared_ptr<UIElement>>& elements);

    PTX_BEGIN_FIELDS(Canvas)
        PTX_FIELD(Canvas, sortOrder, "Sort order", -100, 100),
        PTX_FIELD(Canvas, scaleFactor, "Scale factor", 0.1f, 10.0f),
        PTX_FIELD(Canvas, matchWidthOrHeight, "Match width or height", 0.0f, 1.0f)
    PTX_END_FIELDS

    PTX_BEGIN_METHODS(Canvas)
        PTX_METHOD_AUTO(Canvas, SetRenderMode, "Set render mode"),
        PTX_METHOD_AUTO(Canvas, GetRenderMode, "Get render mode"),
        PTX_METHOD_AUTO(Canvas, SetSortOrder, "Set sort order"),
        PTX_METHOD_AUTO(Canvas, GetSortOrder, "Get sort order"),
        PTX_METHOD_AUTO(Canvas, SetScaleMode, "Set scale mode"),
        PTX_METHOD_AUTO(Canvas, GetScaleMode, "Get scale mode"),
        PTX_METHOD_AUTO(Canvas, SetScreenSize, "Set screen size"),
        PTX_METHOD_AUTO(Canvas, GetScreenSize, "Get screen size"),
        PTX_METHOD_AUTO(Canvas, Update, "Update"),
        PTX_METHOD_AUTO(Canvas, Render, "Render")
    PTX_END_METHODS

    PTX_BEGIN_DESCRIBE(Canvas)
        PTX_CTOR0(Canvas)
    PTX_END_DESCRIBE(Canvas)
};

} // namespace ptx
