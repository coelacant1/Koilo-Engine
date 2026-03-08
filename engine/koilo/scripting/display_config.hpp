// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <string>
#include <map>
#include "../registry/reflect_macros.hpp"

namespace koilo {
namespace scripting {

/**
 * @brief Display configuration object exposed to KoiloScript as "display".
 * 
 * Scripts configure the display via method calls in Setup():
 *   display.SetType("desktop");
 *   display.SetWidth(1280);
 *   display.SetPixelWidth(192);
 */
class DisplayConfig {
public:
    DisplayConfig() = default;

    void SetType(const std::string& t) { type_ = t; }
    void SetWidth(int w) { width_ = w; }
    void SetHeight(int h) { height_ = h; }
    void SetPixelWidth(int w) { pixelWidth_ = w; }
    void SetPixelHeight(int h) { pixelHeight_ = h; }
    void SetBrightness(int b) { brightness_ = b; }
    void SetTargetFPS(int fps) { targetFPS_ = fps; }
    void SetCapFPS(bool cap) { capFPS_ = cap; }

    const std::string& GetType() const { return type_; }
    int GetWidth() const { return width_; }
    int GetHeight() const { return height_; }
    int GetPixelWidth() const { return pixelWidth_; }
    int GetPixelHeight() const { return pixelHeight_; }
    int GetBrightness() const { return brightness_; }
    int GetTargetFPS() const { return targetFPS_; }
    bool GetCapFPS() const { return capFPS_; }

    // Convert to legacy displayConfig map for backward compat with host code
    std::map<std::string, std::string> ToConfigMap() const {
        std::map<std::string, std::string> cfg;
        if (!type_.empty()) cfg["type"] = type_;
        if (width_ > 0) cfg["width"] = std::to_string(width_);
        if (height_ > 0) cfg["height"] = std::to_string(height_);
        if (pixelWidth_ > 0) cfg["pixel_width"] = std::to_string(pixelWidth_);
        if (pixelHeight_ > 0) cfg["pixel_height"] = std::to_string(pixelHeight_);
        if (brightness_ > 0) cfg["brightness"] = std::to_string(brightness_);
        if (targetFPS_ > 0) cfg["target_fps"] = std::to_string(targetFPS_);
        if (capFPS_) cfg["cap_fps"] = "1";
        return cfg;
    }

private:
    std::string type_ = "desktop";
    int width_ = 1280;
    int height_ = 720;
    int pixelWidth_ = 192;
    int pixelHeight_ = 94;
    int brightness_ = 50;
    int targetFPS_ = 60;
    bool capFPS_ = false;

public:
    KL_BEGIN_FIELDS(DisplayConfig)
    KL_END_FIELDS

    KL_BEGIN_METHODS(DisplayConfig)
        KL_METHOD_AUTO(DisplayConfig, SetType, "SetType"),
        KL_METHOD_AUTO(DisplayConfig, SetWidth, "SetWidth"),
        KL_METHOD_AUTO(DisplayConfig, SetHeight, "SetHeight"),
        KL_METHOD_AUTO(DisplayConfig, SetPixelWidth, "SetPixelWidth"),
        KL_METHOD_AUTO(DisplayConfig, SetPixelHeight, "SetPixelHeight"),
        KL_METHOD_AUTO(DisplayConfig, SetBrightness, "SetBrightness"),
        KL_METHOD_AUTO(DisplayConfig, SetTargetFPS, "SetTargetFPS"),
        KL_METHOD_AUTO(DisplayConfig, SetCapFPS, "SetCapFPS"),
        KL_METHOD_AUTO(DisplayConfig, GetType, "GetType"),
        KL_METHOD_AUTO(DisplayConfig, GetWidth, "GetWidth"),
        KL_METHOD_AUTO(DisplayConfig, GetHeight, "GetHeight"),
        KL_METHOD_AUTO(DisplayConfig, GetPixelWidth, "GetPixelWidth"),
        KL_METHOD_AUTO(DisplayConfig, GetPixelHeight, "GetPixelHeight"),
        KL_METHOD_AUTO(DisplayConfig, GetBrightness, "GetBrightness"),
        KL_METHOD_AUTO(DisplayConfig, GetTargetFPS, "GetTargetFPS"),
        KL_METHOD_AUTO(DisplayConfig, GetCapFPS, "GetCapFPS")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(DisplayConfig)
        KL_CTOR0(DisplayConfig)
    KL_END_DESCRIBE(DisplayConfig)
};

} // namespace scripting
} // namespace koilo
