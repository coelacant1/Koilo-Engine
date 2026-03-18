// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file event_bridge.hpp
 * @brief Bridges MessageBus events to subscribed TCP console clients.
 *
 * Each connected TCP client can subscribe to specific message types.
 * When a matching message fires on the kernel MessageBus, a JSON-line
 * event is pushed to all subscribed clients.
 */
#pragma once
#include <koilo/kernel/message_bus.hpp>
#include <koilo/kernel/message_types.hpp>
#include <cstring>
#include <mutex>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>

#ifdef _WIN32
    #include <winsock2.h>
    #ifndef MSG_NOSIGNAL
        #define MSG_NOSIGNAL 0
    #endif
#else
    #include <sys/socket.h>
    #ifndef MSG_NOSIGNAL
        #define MSG_NOSIGNAL 0
    #endif
#endif

namespace koilo {

/// Thread-local token identifying the current TCP client during command execution.
/// Set by ConsoleSocket::HandleClient, read by subscribe/unsubscribe commands.
inline thread_local uint32_t g_eventBridgeToken = 0;

/// Bridges kernel MessageBus events to subscribed TCP console clients.
class EventBridge {
public:
    using ClientToken = uint32_t;

    EventBridge() = default;

    /// Register a new client. Returns a token for subscription management.
    ClientToken RegisterClient(int fd) {
        std::lock_guard<std::mutex> lock(mutex_);
        ClientToken token = nextToken_++;
        clients_[token] = ClientEntry{fd, {}};
        return token;
    }

    /// Unregister a client and remove all its subscriptions.
    void UnregisterClient(ClientToken token) {
        std::lock_guard<std::mutex> lock(mutex_);
        clients_.erase(token);
    }

    /// Subscribe the client to a message type.
    void Subscribe(ClientToken token, MessageType type) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = clients_.find(token);
        if (it != clients_.end())
            it->second.subscriptions.insert(type);
    }

    /// Unsubscribe the client from a message type.
    void Unsubscribe(ClientToken token, MessageType type) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = clients_.find(token);
        if (it != clients_.end())
            it->second.subscriptions.erase(type);
    }

    /// Unsubscribe the client from all message types.
    void UnsubscribeAll(ClientToken token) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = clients_.find(token);
        if (it != clients_.end())
            it->second.subscriptions.clear();
    }

    /// Get the set of message types a client is subscribed to.
    std::set<MessageType> GetSubscriptions(ClientToken token) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = clients_.find(token);
        if (it != clients_.end())
            return it->second.subscriptions;
        return {};
    }

    /// Called by MessageBus handler - push event to all subscribed clients.
    void OnMessage(const Message& msg) {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto& [token, entry] : clients_) {
            if (entry.subscriptions.count(msg.type)) {
                std::ostringstream ss;
                ss << "{\"type\":\"event\""
                   << ",\"event\":\"" << MessageTypeName(msg.type) << "\""
                   << ",\"event_id\":" << msg.type
                   << ",\"source\":" << msg.source
                   << ",\"size\":" << msg.size
                   << "}\n";
                std::string json = ss.str();
                ::send(entry.fd, json.c_str(),
                       static_cast<int>(json.size()), MSG_NOSIGNAL);
            }
        }
    }

    /// Connect to a MessageBus - subscribe to all events and forward.
    void ConnectToMessageBus(MessageBus& bus) {
        subId_ = bus.SubscribeAll([this](const Message& msg) {
            OnMessage(msg);
        });
    }

    /// Disconnect from MessageBus.
    void DisconnectFromMessageBus(MessageBus& bus) {
        if (subId_ != 0) {
            bus.Unsubscribe(subId_);
            subId_ = 0;
        }
    }

private:
    struct ClientEntry {
        int fd = -1;
        std::set<MessageType> subscriptions;
    };

    std::mutex mutex_;
    std::unordered_map<ClientToken, ClientEntry> clients_;
    ClientToken nextToken_ = 1;
    MessageBus::SubscriptionId subId_ = 0;
};

/// Resolve a human-readable event name to a MessageType.
/// Returns MSG_NONE if unrecognized.
inline MessageType ParseEventName(const std::string& name) {
    if (name == "module.loaded"   || name == "MODULE_LOADED")   return MSG_MODULE_LOADED;
    if (name == "module.unloaded" || name == "MODULE_UNLOADED") return MSG_MODULE_UNLOADED;
    if (name == "module.reloaded" || name == "MODULE_RELOADED") return MSG_MODULE_RELOADED;
    if (name == "shutdown"        || name == "SHUTDOWN")        return MSG_SHUTDOWN;
    if (name == "frame.begin"     || name == "FRAME_BEGIN")     return MSG_FRAME_BEGIN;
    if (name == "frame.end"       || name == "FRAME_END")       return MSG_FRAME_END;
    if (name == "service.registered"   || name == "SERVICE_REGISTERED")   return MSG_SERVICE_REGISTERED;
    if (name == "service.unregistered" || name == "SERVICE_UNREGISTERED") return MSG_SERVICE_UNREGISTERED;
    if (name == "asset.loaded"    || name == "ASSET_LOADED")    return MSG_ASSET_LOADED;
    if (name == "asset.unloaded"  || name == "ASSET_UNLOADED")  return MSG_ASSET_UNLOADED;
    if (name == "asset.changed"   || name == "ASSET_CHANGED")   return MSG_ASSET_CHANGED;
    if (name == "scene.loaded"    || name == "SCENE_LOADED")    return MSG_SCENE_LOADED;
    if (name == "entity.created"  || name == "ENTITY_CREATED")  return MSG_ENTITY_CREATED;
    if (name == "entity.destroyed"|| name == "ENTITY_DESTROYED")return MSG_ENTITY_DESTROYED;
    return MSG_NONE;
}

/// Get list of all supported event names for help/autocomplete.
inline std::vector<std::string> AllEventNames() {
    return {
        "module.loaded", "module.unloaded", "module.reloaded",
        "shutdown", "frame.begin", "frame.end",
        "service.registered", "service.unregistered",
        "asset.loaded", "asset.unloaded", "asset.changed",
        "scene.loaded", "entity.created", "entity.destroyed"
    };
}

} // namespace koilo
