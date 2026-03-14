// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file icon_ids.hpp
 * @brief Built-in icon identifiers for the Koilo UI system.
 *
 * @date 03/08/2026
 * @author Coela Can't
 */

#pragma once

#include <cstdint>
#include <cstring>

namespace koilo {
namespace ui {

/// Enumeration of built-in UI icons.
/// Each icon is a small glyph rendered from an embedded bitmap atlas.
enum class IconId : uint16_t {
    None = 0, ///< No icon

    // File/folder
    File,       ///< Generic file
    Folder,     ///< Closed folder
    FolderOpen, ///< Open folder

    // Scene objects
    Mesh,     ///< 3D mesh / model
    Material, ///< Surface material
    Texture,  ///< Image texture
    Light,    ///< Light source
    Camera,   ///< Camera
    Script,   ///< Script file

    // Actions
    Plus,     ///< Add / create
    Minus,    ///< Remove / subtract
    Trash,    ///< Delete
    Search,   ///< Search / find
    Settings, ///< Settings / preferences
    Save,     ///< Save to disk
    Undo,     ///< Undo last action
    Redo,     ///< Redo last undone action

    // Visibility/lock
    Eye,      ///< Visible
    EyeOff,   ///< Hidden
    Lock,     ///< Locked
    LockOpen, ///< Unlocked

    // Transport
    Play,  ///< Play animation
    Pause, ///< Pause playback
    Stop,  ///< Stop playback

    // Navigation
    ChevronRight, ///< Right arrow
    ChevronDown,  ///< Down arrow
    ChevronLeft,  ///< Left arrow
    ChevronUp,    ///< Up arrow

    // Transform
    Move,   ///< Translate tool
    Rotate, ///< Rotation tool
    Scale,  ///< Scale tool
    Grid,   ///< Snap-to-grid toggle

    // Misc
    Warning, ///< Warning indicator
    Error,   ///< Error indicator
    Info,    ///< Information indicator
    Check,   ///< Checkmark / success
    Close,   ///< Close / dismiss

    Count ///< Total number of icons (sentinel)
};

/** @brief Returns a short human-readable name for an icon. */
inline const char* IconName(IconId id) {
    switch (id) {
        case IconId::None:         return "none";
        case IconId::File:         return "file";
        case IconId::Folder:       return "folder";
        case IconId::FolderOpen:   return "folder-open";
        case IconId::Mesh:         return "mesh";
        case IconId::Material:     return "material";
        case IconId::Texture:      return "texture";
        case IconId::Light:        return "light";
        case IconId::Camera:       return "camera";
        case IconId::Script:       return "script";
        case IconId::Plus:         return "plus";
        case IconId::Minus:        return "minus";
        case IconId::Trash:        return "trash";
        case IconId::Search:       return "search";
        case IconId::Settings:     return "settings";
        case IconId::Save:         return "save";
        case IconId::Undo:         return "undo";
        case IconId::Redo:         return "redo";
        case IconId::Eye:          return "eye";
        case IconId::EyeOff:       return "eye-off";
        case IconId::Lock:         return "lock";
        case IconId::LockOpen:     return "lock-open";
        case IconId::Play:         return "play";
        case IconId::Pause:        return "pause";
        case IconId::Stop:         return "stop";
        case IconId::ChevronRight: return "chevron-right";
        case IconId::ChevronDown:  return "chevron-down";
        case IconId::ChevronLeft:  return "chevron-left";
        case IconId::ChevronUp:    return "chevron-up";
        case IconId::Move:         return "move";
        case IconId::Rotate:       return "rotate";
        case IconId::Scale:        return "scale";
        case IconId::Grid:         return "grid";
        case IconId::Warning:      return "warning";
        case IconId::Error:        return "error";
        case IconId::Info:         return "info";
        case IconId::Check:        return "check";
        case IconId::Close:        return "close";
        default:                   return "unknown";
    }
}

/** @brief Parse icon name string to IconId. */
inline IconId IconFromName(const char* name) {
    if (!name) return IconId::None;
    // Linear scan is fine for < 40 icons
    for (int i = 0; i < static_cast<int>(IconId::Count); ++i) {
        if (strcmp(name, IconName(static_cast<IconId>(i))) == 0)
            return static_cast<IconId>(i);
    }
    return IconId::None;
}

} // namespace ui
} // namespace koilo
