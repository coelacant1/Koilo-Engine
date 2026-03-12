// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
/**
 * @file sdl3_input_helper.hpp
 * @brief SDL3 event -> KoiloEngine input routing helper (desktop hosts only).
 *
 * Usage:
 *   while (SDL_PollEvent(&event)) {
 *       if (!koilo::HandleSDL3Event(engine, event))
 *           running = false;  // SDL_EVENT_QUIT or ESC
 *   }
 */

#include <SDL3/SDL.h>
#include <koilo/scripting/koiloscript_engine.hpp>
#include <koilo/systems/input/inputmanager.hpp>
#include <koilo/systems/input/keycodes.hpp>
#include <koilo/systems/ui/ui.hpp>
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
    if (sym >= SDLK_A && sym <= SDLK_Z) return std::string(1, 'a' + (sym - SDLK_A));
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

/// Map UI CursorType to SDL system cursor and apply it.
inline void UpdateSDLCursor(UI* ui) {
    if (!ui) return;
    static ui::CursorType lastCursor = ui::CursorType::Default;
    ui::CursorType cur = ui->GetRequestedCursor();
    if (cur == lastCursor) return;
    lastCursor = cur;

    SDL_SystemCursor sc = SDL_SYSTEM_CURSOR_DEFAULT;
    switch (cur) {
        case ui::CursorType::Pointer:    sc = SDL_SYSTEM_CURSOR_POINTER; break;
        case ui::CursorType::Text:       sc = SDL_SYSTEM_CURSOR_TEXT; break;
        case ui::CursorType::Crosshair:  sc = SDL_SYSTEM_CURSOR_CROSSHAIR; break;
        case ui::CursorType::Move:       sc = SDL_SYSTEM_CURSOR_MOVE; break;
        case ui::CursorType::Grab:       sc = SDL_SYSTEM_CURSOR_MOVE; break;
        case ui::CursorType::Grabbing:   sc = SDL_SYSTEM_CURSOR_MOVE; break;
        case ui::CursorType::EWResize:   sc = SDL_SYSTEM_CURSOR_EW_RESIZE; break;
        case ui::CursorType::NSResize:   sc = SDL_SYSTEM_CURSOR_NS_RESIZE; break;
        case ui::CursorType::NWSEResize: sc = SDL_SYSTEM_CURSOR_NWSE_RESIZE; break;
        case ui::CursorType::NESWResize: sc = SDL_SYSTEM_CURSOR_NESW_RESIZE; break;
        case ui::CursorType::NotAllowed: sc = SDL_SYSTEM_CURSOR_NOT_ALLOWED; break;
        case ui::CursorType::Wait:       sc = SDL_SYSTEM_CURSOR_WAIT; break;
        case ui::CursorType::None:
        default:                         sc = SDL_SYSTEM_CURSOR_DEFAULT; break;
    }
    SDL_Cursor* cursor = SDL_CreateSystemCursor(sc);
    if (cursor) SDL_SetCursor(cursor);
}

/**
 * @brief Route a single SDL3 event into KoiloEngine's input system.
 * @param engine The script engine instance
 * @param event  The SDL3 event to process
 * @return true if the application should continue, false on SDL_EVENT_QUIT or ESC
 *
 * Handles: keyboard, mouse, text input, gamepad. The host only needs:
 *   while (SDL_PollEvent(&e)) { if (!HandleSDL3Event(engine, e)) quit(); }
 */
inline bool HandleSDL3Event(scripting::KoiloScriptEngine& engine, const SDL_Event& event) {
    InputManager* input = engine.GetInputManager();

    switch (event.type) {
        case SDL_EVENT_QUIT:
            return false;

        case SDL_EVENT_KEY_DOWN: {
            SDL_Scancode sc = event.key.scancode;
            if (sc == SDL_SCANCODE_ESCAPE) return false;
            input->GetKeyboard().SetKeyState(SDLScancodeToKeyCode(sc), true);
            std::string keyId = SDLKeycodeToString(event.key.key);
            if (!keyId.empty()) engine.HandleInput("key", keyId.c_str());
            break;
        }

        case SDL_EVENT_KEY_UP: {
            input->GetKeyboard().SetKeyState(
                SDLScancodeToKeyCode(event.key.scancode), false);
            std::string keyId = SDLKeycodeToString(event.key.key);
            if (!keyId.empty()) engine.HandleInput("key_release", keyId.c_str());
            break;
        }

        case SDL_EVENT_TEXT_INPUT:
            input->GetKeyboard().AddTextInput(event.text.text);
            break;

        case SDL_EVENT_MOUSE_MOTION:
            input->GetMouse().SetPosition(event.motion.x, event.motion.y);
            if (UI* ui = engine.GetUI()) {
                ui->HandlePointerMove(event.motion.x, event.motion.y);
                UpdateSDLCursor(ui);
            }
            break;

        case SDL_EVENT_MOUSE_BUTTON_DOWN:
            input->GetMouse().SetButtonState(
                SDLMouseButtonToMouseButton(event.button.button), true);
            if (UI* ui = engine.GetUI()) {
                uint8_t btn = (event.button.button == SDL_BUTTON_LEFT) ? 0
                            : (event.button.button == SDL_BUTTON_RIGHT) ? 1 : 2;
                ui->HandlePointerDown(event.button.x, event.button.y, btn);
            }
            break;

        case SDL_EVENT_MOUSE_BUTTON_UP:
            input->GetMouse().SetButtonState(
                SDLMouseButtonToMouseButton(event.button.button), false);
            if (UI* ui = engine.GetUI()) {
                uint8_t btn = (event.button.button == SDL_BUTTON_LEFT) ? 0
                            : (event.button.button == SDL_BUTTON_RIGHT) ? 1 : 2;
                ui->HandlePointerUp(event.button.x, event.button.y, btn);
            }
            break;

        case SDL_EVENT_MOUSE_WHEEL: {
            input->GetMouse().AddScrollDelta(event.wheel.x, event.wheel.y);
            float mx, my;
            SDL_GetMouseState(&mx, &my);
            if (UI* ui = engine.GetUI())
                ui->HandleScroll(mx, my, event.wheel.y);
            break;
        }

        case SDL_EVENT_GAMEPAD_ADDED:
            SDL_OpenGamepad(event.gdevice.which);
            input->GetGamepad(event.gdevice.which).SetConnected(true);
            break;

        case SDL_EVENT_GAMEPAD_REMOVED:
            input->GetGamepad(event.gdevice.which).SetConnected(false);
            break;

        case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
            input->GetGamepad(event.gbutton.which).SetButtonState(
                static_cast<GamepadButton>(event.gbutton.button), true);
            break;

        case SDL_EVENT_GAMEPAD_BUTTON_UP:
            input->GetGamepad(event.gbutton.which).SetButtonState(
                static_cast<GamepadButton>(event.gbutton.button), false);
            break;

        case SDL_EVENT_GAMEPAD_AXIS_MOTION:
            input->GetGamepad(event.gaxis.which).SetAxisValue(
                static_cast<GamepadAxis>(event.gaxis.axis),
                static_cast<float>(event.gaxis.value) / 32767.0f);
            break;
    }
    return true;
}

} // namespace koilo
