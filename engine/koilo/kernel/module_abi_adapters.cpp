// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file module_abi_adapters.cpp
 * @brief C ABI v3 adapter implementations.
 *
 * Bridges C descriptor structs from external modules to C++ subsystem
 * interfaces (CommandRegistry, InputListenerRegistry, ComponentRegistry,
 * WidgetTypeRegistry).
 *
 * @date 03/30/2026
 * @author Coela
 */
#include "module_abi_adapters.hpp"
#include "kernel.hpp"
#include "service_registry.hpp"
#include "console/command_registry.hpp"
#include "console/console_result.hpp"
#include "../systems/input/input_listener.hpp"
#include "../systems/ecs/component_type.hpp"
#include "../systems/ecs/component_registry.hpp"
#include "../systems/ui/widget_factory.hpp"
#include "../kernel/logging/log.hpp"

#include <cstring>
#include <string>
#include <vector>

namespace koilo {

// ---- Command Adapter ----

int AbiRegisterCommand(void* engine, const KoiloCommandDesc* desc) {
    if (!engine || !desc || !desc->name || !desc->handler) return -1;

    auto* kernel = static_cast<KoiloKernel*>(engine);
    auto* registry = kernel->Services().Get<CommandRegistry>("console.commands");
    if (!registry) {
        KL_WARN("ABI", "Cannot register command '%s': CommandRegistry not available", desc->name);
        return -2;
    }

    // Capture C function pointer for lambda
    auto cHandler = desc->handler;
    std::string cmdName = desc->name;

    CommandDef def;
    def.name = cmdName;
    def.description = desc->help ? desc->help : "";
    def.usage = cmdName;
    def.handler = [cHandler](KoiloKernel&, const std::vector<std::string>& args) -> ConsoleResult {
        // Convert vector<string> to const char** array
        std::vector<const char*> cArgs;
        cArgs.reserve(args.size());
        for (const auto& a : args) cArgs.push_back(a.c_str());

        int rc = cHandler(cArgs.data(), static_cast<int>(cArgs.size()));
        return (rc == 0) ? ConsoleResult::Ok("OK")
                         : ConsoleResult::Error("Error (code " + std::to_string(rc) + ")");
    };

    registry->Register(std::move(def));
    KL_LOG("ABI", "Registered command '%s' via ABI v3", cmdName.c_str());
    return 0;
}

// ---- Input Listener Adapter ----

class AbiInputListener : public IInputListener {
public:
    AbiInputListener(const KoiloInputListenerDesc& d)
        : name_(d.name ? d.name : "abi_listener")
        , priority_(d.priority)
        , onKey_(d.on_key)
        , onMouseButton_(d.on_mouse_button)
        , onMouseMove_(d.on_mouse_move)
        , onScroll_(d.on_scroll) {}

    const char* GetName() const override { return name_.c_str(); }
    int GetPriority() const override { return priority_; }

    bool OnKeyEvent(KeyEvent& e) override {
        if (!onKey_) return false;
        return onKey_(static_cast<int>(e.key),
                      static_cast<int>(e.action),
                      (e.shift ? 1 : 0) | (e.ctrl ? 2 : 0) | (e.alt ? 4 : 0)) != 0;
    }
    bool OnMouseButtonEvent(MouseButtonEvent& e) override {
        if (!onMouseButton_) return false;
        return onMouseButton_(static_cast<int>(e.button),
                              static_cast<int>(e.action),
                              e.position.X, e.position.Y) != 0;
    }
    bool OnMouseMoveEvent(MouseMoveEvent& e) override {
        if (!onMouseMove_) return false;
        return onMouseMove_(e.position.X, e.position.Y, e.delta.X, e.delta.Y) != 0;
    }
    bool OnScrollEvent(ScrollEvent& e) override {
        if (!onScroll_) return false;
        return onScroll_(e.delta.X, e.delta.Y) != 0;
    }

private:
    std::string name_;
    int priority_;
    int (*onKey_)(int, int, int);
    int (*onMouseButton_)(int, int, float, float);
    int (*onMouseMove_)(float, float, float, float);
    int (*onScroll_)(float, float);
};

int AbiRegisterInputListener(void* engine, const KoiloInputListenerDesc* desc) {
    if (!engine || !desc || !desc->name) return -1;

    auto* kernel = static_cast<KoiloKernel*>(engine);
    auto* registry = kernel->Services().Get<InputListenerRegistry>("input.listeners");
    if (!registry) {
        KL_WARN("ABI", "Cannot register input listener '%s': InputListenerRegistry not available", desc->name);
        return -2;
    }

    auto* listener = new AbiInputListener(*desc);
    registry->Register(listener);
    KL_LOG("ABI", "Registered input listener '%s' via ABI v3", desc->name);
    return 0;
}

// ---- Component Type Adapter ----

class AbiComponentType : public IComponentType {
public:
    AbiComponentType(const KoiloComponentDesc& d)
        : name_(d.name ? d.name : "unknown")
        , size_(d.size)
        , alignment_(d.alignment > 0 ? d.alignment : 1)
        , construct_(d.construct)
        , destruct_(d.destruct)
        , copy_(d.copy) {}

    const char* GetName() const override { return name_.c_str(); }
    size_t GetSize() const override { return size_; }
    size_t GetAlignment() const override { return alignment_; }

    void Construct(void* dest) const override {
        if (construct_) construct_(dest);
        else std::memset(dest, 0, size_);
    }
    void Destruct(void* ptr) const override {
        if (destruct_) destruct_(ptr);
    }
    void Copy(void* dest, const void* src) const override {
        if (copy_) copy_(dest, src);
        else std::memcpy(dest, src, size_);
    }
    void Move(void* dest, void* src) const override {
        Copy(dest, src);
    }

private:
    std::string name_;
    uint32_t size_;
    uint32_t alignment_;
    void (*construct_)(void*);
    void (*destruct_)(void*);
    void (*copy_)(void*, const void*);
};

int AbiRegisterComponent(void* engine, const KoiloComponentDesc* desc) {
    if (!engine || !desc || !desc->name || desc->size == 0) return -1;

    auto* kernel = static_cast<KoiloKernel*>(engine);
    auto* registry = kernel->Services().Get<ComponentRegistry>("ecs.components");
    if (!registry) {
        KL_WARN("ABI", "Cannot register component '%s': ComponentRegistry not available", desc->name);
        return -2;
    }

    auto type = std::make_unique<AbiComponentType>(*desc);
    registry->Register(std::move(type));
    KL_LOG("ABI", "Registered component '%s' (%u bytes) via ABI v3", desc->name, desc->size);
    return 0;
}

// ---- Widget Type Adapter ----

class AbiCustomWidget : public ui::CustomWidget {
public:
    AbiCustomWidget(const char* name,
                    void* state,
                    void (*destroy)(void*),
                    void (*render)(void*, void*),
                    void (*onClick)(void*))
        : typeName_(name), state_(state), destroy_(destroy), render_(render), onClick_(onClick) {}

    ~AbiCustomWidget() override {
        if (destroy_ && state_) destroy_(state_);
    }

    void Render(void* ctx) override {
        if (render_ && state_) render_(state_, ctx);
    }
    void OnClick() override {
        if (onClick_ && state_) onClick_(state_);
    }
    const char* GetTypeName() const override { return typeName_.c_str(); }

private:
    std::string typeName_;
    void* state_;
    void (*destroy_)(void*);
    void (*render_)(void*, void*);
    void (*onClick_)(void*);
};

class AbiWidgetFactory : public ui::IWidgetFactory {
public:
    AbiWidgetFactory(const KoiloWidgetTypeDesc& d)
        : name_(d.name ? d.name : "unknown")
        , create_(d.create)
        , destroy_(d.destroy)
        , render_(d.render)
        , onClick_(d.on_click) {}

    const char* GetTypeName() const override { return name_.c_str(); }

    std::unique_ptr<ui::CustomWidget> Create() override {
        void* state = create_ ? create_() : nullptr;
        return std::make_unique<AbiCustomWidget>(
            name_.c_str(), state, destroy_, render_, onClick_);
    }

private:
    std::string name_;
    void* (*create_)(void);
    void  (*destroy_)(void*);
    void  (*render_)(void*, void*);
    void  (*onClick_)(void*);
};

int AbiRegisterWidgetType(void* engine, const KoiloWidgetTypeDesc* desc) {
    if (!engine || !desc || !desc->name) return -1;

    auto* kernel = static_cast<KoiloKernel*>(engine);
    auto* registry = kernel->Services().Get<ui::WidgetTypeRegistry>("ui.widgets");
    if (!registry) {
        KL_WARN("ABI", "Cannot register widget type '%s': WidgetTypeRegistry not available", desc->name);
        return -2;
    }

    auto factory = std::make_unique<AbiWidgetFactory>(*desc);
    registry->Register(std::move(factory));
    KL_LOG("ABI", "Registered widget type '%s' via ABI v3", desc->name);
    return 0;
}

// ---- Render Pass Adapter (stub - render graph not yet implemented) ----

int AbiRegisterRenderPass(void* engine, const KoiloRenderPassDesc* desc) {
    if (!engine || !desc || !desc->name) return -1;
    KL_WARN("ABI", "register_render_pass('%s') not yet implemented (render graph pending)", desc->name);
    return -3; // Not yet available
}

} // namespace koilo
