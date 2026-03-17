// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file message_types.hpp
 * @brief Kernel message type IDs and module addressing constants.
 *
 * @date 10/31/2025
 * @author Coela
 */
#pragma once
#include <cstdint>

namespace koilo {

using MessageType = uint32_t;
using ModuleId    = uint16_t;

constexpr ModuleId MODULE_KERNEL    = 0;
constexpr ModuleId MODULE_BROADCAST = 0xFFFF;

// Built-in kernel message types
constexpr MessageType MSG_NONE              = 0;
constexpr MessageType MSG_MODULE_LOADED     = 1;
constexpr MessageType MSG_MODULE_UNLOADED   = 2;
constexpr MessageType MSG_MODULE_RELOADED   = 3;
constexpr MessageType MSG_SHUTDOWN          = 4;
constexpr MessageType MSG_FRAME_BEGIN       = 5;
constexpr MessageType MSG_FRAME_END         = 6;
constexpr MessageType MSG_SERVICE_REGISTERED   = 7;
constexpr MessageType MSG_SERVICE_UNREGISTERED = 8;

// Asset system messages
constexpr MessageType MSG_ASSET_LOADED      = 9;
constexpr MessageType MSG_ASSET_UNLOADED    = 10;
constexpr MessageType MSG_ASSET_CHANGED     = 11;  // hot-reload

// Scene/ECS messages
constexpr MessageType MSG_SCENE_LOADED      = 12;
constexpr MessageType MSG_ENTITY_CREATED    = 13;
constexpr MessageType MSG_ENTITY_DESTROYED  = 14;

/// User/module message types start here.
constexpr MessageType MSG_USER_BASE = 0x1000;

/// Human-readable name for a message type (for console/debug).
inline const char* MessageTypeName(MessageType type) {
    switch (type) {
        case MSG_NONE:                 return "NONE";
        case MSG_MODULE_LOADED:        return "MODULE_LOADED";
        case MSG_MODULE_UNLOADED:      return "MODULE_UNLOADED";
        case MSG_MODULE_RELOADED:      return "MODULE_RELOADED";
        case MSG_SHUTDOWN:             return "SHUTDOWN";
        case MSG_FRAME_BEGIN:          return "FRAME_BEGIN";
        case MSG_FRAME_END:            return "FRAME_END";
        case MSG_SERVICE_REGISTERED:   return "SERVICE_REGISTERED";
        case MSG_SERVICE_UNREGISTERED: return "SERVICE_UNREGISTERED";
        case MSG_ASSET_LOADED:         return "ASSET_LOADED";
        case MSG_ASSET_UNLOADED:       return "ASSET_UNLOADED";
        case MSG_ASSET_CHANGED:        return "ASSET_CHANGED";
        case MSG_SCENE_LOADED:         return "SCENE_LOADED";
        case MSG_ENTITY_CREATED:       return "ENTITY_CREATED";
        case MSG_ENTITY_DESTROYED:     return "ENTITY_DESTROYED";
        default:                       return "USER";
    }
}

/// Lightweight message header. Payload is a zero-copy pointer
/// (typically into arena-allocated memory).
struct Message {
    MessageType  type    = MSG_NONE;
    ModuleId     source  = MODULE_KERNEL;
    uint32_t     size    = 0;        // payload size in bytes
    const void*  payload = nullptr;  // non-owning pointer
};

/// Helper to construct a typed message with inline payload.
template<typename T>
Message MakeMessage(MessageType type, ModuleId source, const T& data) {
    return Message{type, source, sizeof(T), &data};
}

/// Helper for payloadless signals.
inline Message MakeSignal(MessageType type, ModuleId source = MODULE_KERNEL) {
    return Message{type, source, 0, nullptr};
}

} // namespace koilo
