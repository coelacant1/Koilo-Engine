// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file console_socket.hpp
 * @brief TCP socket server for remote console access.
 *
 * @date 12/23/2025
 * @author Coela
 */
#pragma once
#include <koilo/kernel/console/console_session.hpp>
#include <string>
#include <thread>
#include <atomic>
#include <vector>
#include "../../registry/reflect_macros.hpp"

namespace koilo {

class KoiloKernel;
class CommandRegistry;

/// TCP socket server for remote console access.
/// Accepts connections on a configurable port and executes commands
/// via a ConsoleSession. Output format defaults to JSON for machine parsing.
class ConsoleSocket {
public:
    ConsoleSocket(KoiloKernel& kernel, CommandRegistry& registry);
    ~ConsoleSocket();

    ConsoleSocket(const ConsoleSocket&) = delete;
    ConsoleSocket& operator=(const ConsoleSocket&) = delete;

    /// Start listening on the given port. Returns false on error.
    bool Start(uint16_t port = 9090);

    /// Stop the server and close all connections.
    void Stop();

    bool IsRunning() const { return running_.load(); }
    uint16_t Port() const { return port_; }

private:
    void AcceptLoop();
    void HandleClient(int clientFd);

    KoiloKernel&     kernel_;
    CommandRegistry& registry_;
    int              serverFd_ = -1;
    uint16_t         port_ = 0;
    std::atomic<bool> running_{false};
    std::thread       acceptThread_;
    std::vector<std::thread> clientThreads_;

    KL_BEGIN_FIELDS(ConsoleSocket)
        /* No reflected fields. */
    KL_END_FIELDS

    KL_BEGIN_METHODS(ConsoleSocket)
        KL_METHOD_AUTO(ConsoleSocket, IsRunning, "Is running"),
        KL_METHOD_AUTO(ConsoleSocket, Port, "Port")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(ConsoleSocket)
        /* Non-copyable, requires references - no reflected ctors. */
    KL_END_DESCRIBE(ConsoleSocket)

};

} // namespace koilo
