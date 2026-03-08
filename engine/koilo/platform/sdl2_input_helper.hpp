// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
/**
 * @file sdl2_input_helper.hpp
 * @brief SDL2 event -> KoiloEngine input routing helper (desktop hosts only).
 *
 * Include this header in SDL2 host applications to eliminate ~60 lines of
 * event-routing boilerplate. Requires SDL2 headers to be available.
 *
 * Usage:
 *   while (SDL_PollEvent(&event)) {
 *       if (!koilo::HandleSDL2Event(engine, event))
 *           running = false;  // SDL_QUIT or ESC
 *   }
 */

#include <SDL2/SDL.h>
#include <koilo/scripting/koiloscript_engine.hpp>
#include <koilo/systems/input/inputmanager.hpp>
#include <koilo/systems/input/keycodes.hpp>
#include <string>

namespace koilo {

// Map SDL scancode -> KoiloEngine KeyCode.
inline KeyCode SDLScancodeToKeyCode(SDL_Scancode sc) {
    uint16_t code = static_cast<uint16_t>(sc);
    if (code < static_cast<uint16_t>(KeyCode::MaxKeyCode))
        return static_cast<KeyCode>(code);
    return KeyCode::Unknown;
}

// Map SDL mouse button -> KoiloEngine MouseButton.
inline MouseButton SDLMouseButtonToMouseButton(uint8_t sdlButton) {
    switch (sdlButton) {
        case SDL_BUTTON_LEFT:   return MouseButton::Left;
        case SDL_BUTTON_MIDDLE: return MouseButton::Middle;
        case SDL_BUTTON_RIGHT:  return MouseButton::Right;
        case SDL_BUTTON_X1:     return MouseButton::X1;
        case SDL_BUTTON_X2:     return MouseButton::X2;
        default:                return MouseButton::Left;
    }
}

// Map SDL keycode -> string ID for script OnKeyPress dispatch.
inline std::string SDLKeycodeToString(SDL_Keycode sym) {
    if (sym >= SDLK_0 && sym <= SDLK_9) return std::string(1, '0' + (sym - SDLK_0));
    if (sym >= SDLK_a && sym <= SDLK_z) return std::string(1, 'a' + (sym - SDLK_a));
    if (sym == SDLK_MINUS)  return "-";
    if (sym == SDLK_EQUALS) return "=";
    if (sym == SDLK_SPACE)  return "space";
    if (sym == SDLK_ESCAPE) return "escape";
    if (sym == SDLK_UP)     return "up";
    if (sym == SDLK_DOWN)   return "down";
    if (sym == SDLK_LEFT)   return "left";
    if (sym == SDLK_RIGHT)  return "right";
    if (sym == SDLK_RETURN) return "enter";
    if (sym == SDLK_TAB)    return "tab";
    if (sym == SDLK_BACKSPACE) return "backspace";
    if (sym == SDLK_DELETE) return "delete";
    if (sym == SDLK_LSHIFT || sym == SDLK_RSHIFT) return "shift";
    if (sym == SDLK_LCTRL || sym == SDLK_RCTRL)   return "ctrl";
    if (sym >= SDLK_F1 && sym <= SDLK_F12)
        return "f" + std::to_string(sym - SDLK_F1 + 1);
    return "";
}

/**
 * @brief Route a single SDL2 event into KoiloEngine's input system.
 * @param engine The script engine instance
 * @param event  The SDL2 event to process
 * @return true if the application should continue, false on SDL_QUIT or ESC
 *
 * Handles: keyboard, mouse, text input, gamepad. The host only needs:
 *   while (SDL_PollEvent(&e)) { if (!HandleSDL2Event(engine, e)) quit(); }
 */
inline bool HandleSDL2Event(scripting::KoiloScriptEngine& engine, const SDL_Event& event) {
    InputManager* input = engine.GetInputManager();

    switch (event.type) {
        case SDL_QUIT:
            return false;

        case SDL_KEYDOWN: {
            SDL_Scancode sc = event.key.keysym.scancode;
            if (sc == SDL_SCANCODE_ESCAPE) return false;
            input->GetKeyboard().SetKeyState(SDLScancodeToKeyCode(sc), true);
            std::string keyId = SDLKeycodeToString(event.key.keysym.sym);
            if (!keyId.empty()) engine.HandleInput("key", keyId.c_str());
            break;
        }

        case SDL_KEYUP: {
            input->GetKeyboard().SetKeyState(
                SDLScancodeToKeyCode(event.key.keysym.scancode), false);
            std::string keyId = SDLKeycodeToString(event.key.keysym.sym);
            if (!keyId.empty()) engine.HandleInput("key_release", keyId.c_str());
            break;
        }

        case SDL_TEXTINPUT:
            input->GetKeyboard().AddTextInput(event.text.text);
            break;

        case SDL_MOUSEMOTION:
            input->GetMouse().SetPosition(
                static_cast<float>(event.motion.x),
                static_cast<float>(event.motion.y));
            break;

        case SDL_MOUSEBUTTONDOWN:
            input->GetMouse().SetButtonState(
                SDLMouseButtonToMouseButton(event.button.button), true);
            break;

        case SDL_MOUSEBUTTONUP:
            input->GetMouse().SetButtonState(
                SDLMouseButtonToMouseButton(event.button.button), false);
            break;

        case SDL_MOUSEWHEEL:
            input->GetMouse().AddScrollDelta(
                static_cast<float>(event.wheel.x),
                static_cast<float>(event.wheel.y));
            break;

        case SDL_CONTROLLERDEVICEADDED:
            SDL_GameControllerOpen(event.cdevice.which);
            input->GetGamepad(event.cdevice.which).SetConnected(true);
            break;

        case SDL_CONTROLLERDEVICEREMOVED:
            input->GetGamepad(event.cdevice.which).SetConnected(false);
            break;

        case SDL_CONTROLLERBUTTONDOWN:
            input->GetGamepad(event.cbutton.which).SetButtonState(
                static_cast<GamepadButton>(event.cbutton.button), true);
            break;

        case SDL_CONTROLLERBUTTONUP:
            input->GetGamepad(event.cbutton.which).SetButtonState(
                static_cast<GamepadButton>(event.cbutton.button), false);
            break;

        case SDL_CONTROLLERAXISMOTION:
            input->GetGamepad(event.caxis.which).SetAxisValue(
                static_cast<GamepadAxis>(event.caxis.axis),
                static_cast<float>(event.caxis.value) / 32767.0f);
            break;
    }
    return true;
}

} // namespace koilo
