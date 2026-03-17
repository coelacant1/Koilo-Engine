// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file timeline.hpp
 * @brief Timeline scrubber widget for the Koilo UI system.
 *
 * Renders a horizontal timeline with frame markers, a playhead,
 * and keyframe indicators. Designed for animation editing.
 *
 * @date 03/08/2026
 * @author Coela Can't
 */

#pragma once

#include <koilo/systems/ui/ui_context.hpp>
#include <koilo/systems/ui/render/draw_list.hpp>
#include <cmath>
#include <vector>
#include <cstdio>
#include "../../registry/reflect_macros.hpp"

namespace koilo {
namespace ui {

/** @class KeyframeMark @brief A keyframe marker at a specific frame. */
struct KeyframeMark {
    int frame;                       ///< Frame number for this keyframe
    Color4 color{255, 200, 60, 255}; ///< Display color for the keyframe marker

    KL_BEGIN_FIELDS(KeyframeMark)
        KL_FIELD(KeyframeMark, frame, "Frame", -2147483648, 2147483647)
    KL_END_FIELDS

    KL_BEGIN_METHODS(KeyframeMark)
        /* No reflected methods. */
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(KeyframeMark)
        /* No reflected ctors. */
    KL_END_DESCRIBE(KeyframeMark)

};

/** @class Timeline @brief Interactive timeline scrubber rendered on a Canvas2D widget. */
class Timeline {
public:
    /** @brief Build the timeline inside a parent widget. */
    int Build(UIContext& ctx, int parentIdx, const char* id, float height = 32.0f);

    /** @brief Set the visible frame range. */
    void SetRange(int startFrame, int endFrame) {
        startFrame_ = startFrame;
        endFrame_ = endFrame;
    }

    /** @brief Set the current playhead frame, clamped to range. */
    void SetCurrentFrame(int frame) {
        currentFrame_ = std::max(startFrame_, std::min(endFrame_, frame));
    }

    /** @brief Get the current playhead frame. */
    int GetCurrentFrame() const { return currentFrame_; }
    /** @brief Get the start of the visible frame range. */
    int GetStartFrame() const { return startFrame_; }
    /** @brief Get the end of the visible frame range. */
    int GetEndFrame() const { return endFrame_; }
    /** @brief Get the playback frames per second. */
    int GetFPS() const { return fps_; }
    /** @brief Set the playback frames per second. */
    void SetFPS(int fps) { fps_ = fps; }

    /** @brief Add a keyframe marker at the given frame. */
    void AddKeyframe(int frame, Color4 color = {255, 200, 60, 255}) {
        keyframes_.push_back({frame, color});
    }

    /** @brief Remove all keyframe markers. */
    void ClearKeyframes() { keyframes_.clear(); }

    /** @brief Handle click/drag to scrub timeline. */
    void HandleInput(float localX, float canvasW, bool pressed);

    /** @brief Get the pool index of the underlying canvas widget. */
    int CanvasIndex() const { return canvasIdx_; }

    /** @brief Get the list of keyframe markers. */
    const std::vector<KeyframeMark>& Keyframes() const { return keyframes_; }

private:
    UIContext* ctx_ = nullptr;               ///< Owning UI context
    int canvasIdx_ = -1;                     ///< Pool index of the canvas widget
    int startFrame_ = 0;                     ///< First visible frame
    int endFrame_ = 120;                     ///< Last visible frame
    int currentFrame_ = 0;                   ///< Current playhead position
    int fps_ = 24;                           ///< Frames per second
    std::vector<KeyframeMark> keyframes_;    ///< Keyframe markers on the timeline

    void Paint(void* rawCtx);

    KL_BEGIN_FIELDS(Timeline)
        /* No reflected fields. */
    KL_END_FIELDS

    KL_BEGIN_METHODS(Timeline)
        KL_METHOD_AUTO(Timeline, SetCurrentFrame, "Set current frame"),
        KL_METHOD_AUTO(Timeline, GetCurrentFrame, "Get current frame"),
        KL_METHOD_AUTO(Timeline, GetStartFrame, "Get start frame"),
        KL_METHOD_AUTO(Timeline, GetEndFrame, "Get end frame"),
        KL_METHOD_AUTO(Timeline, GetFPS, "Get fps"),
        KL_METHOD_AUTO(Timeline, SetFPS, "Set fps"),
        KL_METHOD_AUTO(Timeline, AddKeyframe, "Add keyframe"),
        KL_METHOD_AUTO(Timeline, ClearKeyframes, "Clear keyframes"),
        KL_METHOD_AUTO(Timeline, HandleInput, "Handle input"),
        KL_METHOD_AUTO(Timeline, Keyframes, "Keyframes")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(Timeline)
        /* No reflected ctors. */
    KL_END_DESCRIBE(Timeline)

};

} // namespace ui
} // namespace koilo
