// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file console_module.hpp
 * @brief Development console module providing REPL and structured output.
 *
 * @date 01/09/2026
 * @author Coela
 */
#pragma once
#include <koilo/kernel/module.hpp>
#include <koilo/kernel/console/command_registry.hpp>
#include <koilo/kernel/console/console_session.hpp>
#include <koilo/kernel/console/console_socket.hpp>
#include <koilo/kernel/console/event_bridge.hpp>
#include <koilo/core/interfaces/iconsole_widget.hpp>
#include <memory>
#include "../../registry/reflect_macros.hpp"

namespace koilo {

class UI;
class ConsoleWidget;

/// The development console module. First module built on the kernel.
/// Provides REPL, command system, and structured output for debugging.
class ConsoleModule {
public:
    /// Destructor (defined in .cpp for unique_ptr to incomplete type).
    ~ConsoleModule();

    /// Get the static ModuleDesc for kernel registration.
    static const ModuleDesc& GetModuleDesc();

    /// Get the singleton instance (valid after Init).
    static ConsoleModule* Instance() { return instance_.get(); }

    /// Access the command registry (for other modules to register commands).
    CommandRegistry& Commands() { return commands_; }

    /// Access the default session.
    ConsoleSession& Session() { return *session_; }

    /// Execute a command string. Convenience wrapper.
    ConsoleResult Execute(const std::string& input);

    /// Register all built-in commands.
    void RegisterBuiltinCommands();

    /// Access the event bridge for TCP event subscriptions.
    EventBridge* Events() { return eventBridge_.get(); }

    /// Access the socket server (may be null if not started).
    ConsoleSocket* Socket() { return socket_.get(); }

    /// Start the TCP socket server on the given port.
    bool StartSocket(uint16_t port = 9090);

    /// Stop the TCP socket server.
    void StopSocket();

    /// Access the in-engine console overlay widget.
    IConsoleWidget* Widget();

    /**
     * @brief Build the in-engine console overlay inside the given UI.
     * @param ui UI instance to create widgets in.
     * @param x  Initial X position.
     * @param y  Initial Y position.
     * @param w  Initial width.
     * @param h  Initial height.
     */
    void BuildWidget(UI& ui, float x = 50.0f, float y = 50.0f,
                     float w = 700.0f, float h = 350.0f);

    /**
     * @brief Handle a key event for the console widget.
     *
     * Call from the host input handler. Returns true if the console
     * consumed the event (e.g., toggled visibility or submitted input).
     * @param isReturn True if the Return/Enter key was pressed.
     * @param isToggle True if the console toggle key was pressed (F12/backtick).
     */
    bool HandleKey(bool isReturn, bool isToggle);

    /// Discover ICommandProvider services registered as "commands.*"
    /// and merge their commands into the registry.
    void DiscoverProviders();

private:
    static bool Init(KoiloKernel& kernel);
    static void Tick(float dt);
    static void OnMessage(const Message& msg);
    static void Shutdown();

    CommandRegistry commands_;
    std::unique_ptr<ConsoleSession> session_;
    std::unique_ptr<ConsoleSocket> socket_;
    std::unique_ptr<EventBridge> eventBridge_;
    std::unique_ptr<ConsoleWidget> widget_;
    KoiloKernel* kernel_ = nullptr;
    bool providersDiscovered_ = false;

    static std::unique_ptr<ConsoleModule> instance_;

    KL_BEGIN_FIELDS(ConsoleModule)
        /* No reflected fields. */
    KL_END_FIELDS

    KL_BEGIN_METHODS(ConsoleModule)
        KL_METHOD_AUTO(ConsoleModule, Execute, "Execute")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(ConsoleModule)
        /* No reflected ctors. */
    KL_END_DESCRIBE(ConsoleModule)

};

} // namespace koilo
