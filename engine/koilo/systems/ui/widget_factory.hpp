// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file widget_factory.hpp
 * @brief Extensible widget factory for custom UI widgets.
 *
 * Modules register IWidgetFactory instances to provide custom widget types.
 * Custom widgets render via a CanvasDrawContext facade, using the same
 * drawing primitives as built-in widgets.
 *
 * @date 03/30/2026
 * @author Coela
 */
#pragma once

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

namespace koilo {
namespace ui {

/**
 * @class CustomWidget
 * @brief Base class for custom widget rendering and state.
 *
 * Subclass this and override Render() to draw via the CanvasDrawContext.
 * The context pointer is passed as void* to avoid circular includes
 * (cast to CanvasDrawContext* inside Render).
 */
class CustomWidget {
public:
    virtual ~CustomWidget() = default;

    /// Called each frame the widget is visible.
    /// @param ctx  Pointer to CanvasDrawContext (cast internally).
    virtual void Render(void* ctx) = 0;

    /// Called when the widget receives a click.
    virtual void OnClick() {}

    /// Called when the widget value changes (if applicable).
    virtual void OnChange() {}

    /// Human-readable type name (must match the factory).
    virtual const char* GetTypeName() const = 0;
};

/**
 * @class IWidgetFactory
 * @brief Interface for creating custom widget instances by type name.
 */
class IWidgetFactory {
public:
    virtual ~IWidgetFactory() = default;

    /// The custom widget type name this factory produces (e.g. "ColorWheel").
    virtual const char* GetTypeName() const = 0;

    /// Create a new instance of the custom widget.
    virtual std::unique_ptr<CustomWidget> Create() = 0;
};

/**
 * @class WidgetTypeRegistry
 * @brief Central registry for custom widget factories.
 *
 * Registered as the "ui.widgets" kernel service.
 */
class WidgetTypeRegistry {
public:
    /// Register a factory. Ownership is taken.
    void Register(std::unique_ptr<IWidgetFactory> factory) {
        if (!factory) return;
        std::string name = factory->GetTypeName();
        factories_[name] = std::move(factory);
    }

    /// Create a custom widget by type name.
    /// @return New instance, or nullptr if the type is not registered.
    std::unique_ptr<CustomWidget> CreateByName(const std::string& name) const {
        auto it = factories_.find(name);
        if (it == factories_.end()) return nullptr;
        return it->second->Create();
    }

    /// Check if a custom widget type is registered.
    bool Has(const std::string& name) const {
        return factories_.find(name) != factories_.end();
    }

    /// List all registered custom widget type names.
    std::vector<std::string> ListTypes() const {
        std::vector<std::string> names;
        names.reserve(factories_.size());
        for (auto& [name, _] : factories_) {
            names.push_back(name);
        }
        return names;
    }

    /// Number of registered factories.
    size_t Count() const { return factories_.size(); }

private:
    std::unordered_map<std::string, std::unique_ptr<IWidgetFactory>> factories_;
};

} // namespace ui
} // namespace koilo
