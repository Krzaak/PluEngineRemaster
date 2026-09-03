#pragma once
// Compile only on Linux (or wherever SDL backend is desired)
#include "PluEngine/Core.h"
#if defined(PLU_PLATFORM_LINUX) || defined(SDL_INPUT_BACKEND_FORCE)

#include "PlatformInputBackend.h"
#include <SDL3/SDL.h>
#include <array>
#include <cstring>   // memset


// ============================================================
//  SDLInputBackend
//
//  Dependencies:  SDL3  (link with SDL3::SDL3)
//  Notes:
//    • Call BeginFrame() inside your SDL event loop so the
//      backend can consume SDL_Event items from the queue.
//    • If you already have an event loop, call
//      FeedEvent(event) manually instead of BeginFrame().
//    • The SDL3 Gamepad API (SDL_Gamepad) is used for
//      recognised controllers.
// ============================================================

class SDLInputBackend final : public PlatformInputBackend
{
public:
    // =========================================================
    //  Init / Shutdown
    // =========================================================

    virtual bool Init() override
    {
        PLU_CORE_TRACE("Init SDL Input Backend");

        SDL_SetGamepadEventsEnabled(true);

        // Open gamepads already connected at startup. SDL_GetGamepads returns the instance IDs of
        // the currently connected gamepads (already filtered to gamepad-class devices).
        int count = 0;
        if (SDL_JoystickID* ids = SDL_GetGamepads(&count))
        {
            for (int i = 0; i < count && i < kMaxControllers; ++i)
                OpenController(ids[i]);
            SDL_free(ids);
        }

        return true;
    }

    void Shutdown() override
    {
        for (int i = 0; i < kMaxControllers; ++i)
            CloseController(i);

        SDL_QuitSubSystem(SDL_INIT_EVENTS | SDL_INIT_GAMEPAD);
    }

    // =========================================================
    //  Frame lifecycle
    // =========================================================

    void BeginFrame() override
    {
        SDL_Event e;
        while (SDL_PollEvent(&e))
            FeedEvent(e);
    }

    void Update() override
    {
        // --- Keyboard ---
        int numKeys = 0;
        // SDL3 changed the keyboard state array element type from Uint8 to bool.
        const bool* sdlKeys = SDL_GetKeyboardState(&numKeys);

        for (int k = 0; k < static_cast<int>(Key::Count); ++k)
        {
            SDL_Scancode sc = KeyToScancode(static_cast<Key>(k));
            bool down = (sc != SDL_SCANCODE_UNKNOWN) && sdlKeys[sc];
            ButtonState before = m_keyboard.keys[k];
            TickState(m_keyboard.keys[k], down);
            if (before != m_keyboard.keys[k]) {
                if (GetInputSink() && NotifyGameAboutInput) {
                    GetInputSink().OnKeyboardKey(static_cast<Key>(k), m_keyboard.keys[k]);
                }
            }
        }

        // --- Mouse buttons ---
        Uint32 mask = SDL_GetMouseState(nullptr, nullptr);

        auto checkForButtonChange = [this](MouseButton button, ButtonState* before) -> void
        {
            if (*before != m_mouse.buttons[static_cast<int>(button)]) {
                if (GetInputSink() && NotifyGameAboutInput) {
                    GetInputSink().OnMouseKey(button, m_mouse.buttons[static_cast<int>(button)]);
                }
            }
            *before = m_mouse.buttons[static_cast<int>(button)];
        };

        ButtonState before = m_mouse.buttons[static_cast<int>(MouseButton::Left)];
        TickState(m_mouse.buttons[static_cast<int>(MouseButton::Left)],
                  (mask & SDL_BUTTON_LMASK) != 0);
        checkForButtonChange(MouseButton::Left, &before);
        TickState(m_mouse.buttons[static_cast<int>(MouseButton::Right)],
                  (mask & SDL_BUTTON_RMASK) != 0);
        checkForButtonChange(MouseButton::Right, &before);
        TickState(m_mouse.buttons[static_cast<int>(MouseButton::Middle)],
                  (mask & SDL_BUTTON_MMASK) != 0);
        checkForButtonChange(MouseButton::Middle, &before);
        TickState(m_mouse.buttons[static_cast<int>(MouseButton::Extra1)],
                  (mask & SDL_BUTTON_X1MASK) != 0);
        checkForButtonChange(MouseButton::Extra1, &before);
        TickState(m_mouse.buttons[static_cast<int>(MouseButton::Extra2)],
                  (mask & SDL_BUTTON_X2MASK) != 0);
        checkForButtonChange(MouseButton::Extra2, &before);

        MouseState mouseBefore = m_mouse;

        // --- Mouse position & delta ---
        // SDL3 mouse state coordinates/deltas are floats now.
        float mx, my, relX, relY;
        SDL_GetMouseState(&mx, &my);
        SDL_GetRelativeMouseState(&relX, &relY);
        m_mouse.x      = mx;
        m_mouse.y      = my;
        m_mouse.deltaX = relX;
        m_mouse.deltaY = relY;

        if (mouseBefore != m_mouse) {
            if (GetInputSink() && NotifyGameAboutInput) {
                GetInputSink().OnMouse(m_mouse);
            }
        }

        // --- Controllers ---
        for (int i = 0; i < kMaxControllers; ++i)
            UpdateController(i);
    }

    void EndFrame() override
    {
        // Reset per-frame accumulators
        m_mouse.scrollX = 0.0f;
        m_mouse.scrollY = 0.0f;
    }

    // =========================================================
    //  Manual event injection (use when you own the event loop)
    // =========================================================

    void FeedEvent(const SDL_Event& e)
    {
        switch (e.type)
        {
        case SDL_EVENT_MOUSE_WHEEL:
            m_mouse.scrollX += e.wheel.x;
            m_mouse.scrollY += e.wheel.y;
            break;

        case SDL_EVENT_GAMEPAD_ADDED:
            // In SDL3 gdevice.which is the joystick instance ID (not a device index).
            OpenController(e.gdevice.which);
            break;

        case SDL_EVENT_GAMEPAD_REMOVED:
        {
            int slot = InstanceIDToSlot(e.gdevice.which);
            if (slot >= 0) CloseController(slot);
            break;
        }
        default:
            break;
        }
    }

    // =========================================================
    //  Accessors
    // =========================================================

    [[nodiscard]] const KeyboardState&    GetKeyboard()           const override { return m_keyboard; }
    [[nodiscard]] const MouseState&       GetMouse()              const override { return m_mouse;    }
    [[nodiscard]] const GenericController& GetController(int idx) const override
    {
        return m_controllers[idx < kMaxControllers ? idx : 0];
    }

    // =========================================================
    //  Rumble
    // =========================================================

    void SetRumble(int index, float low, float high) override
    {
        if (index < 0 || index >= kMaxControllers) return;
        if (!m_sdlControllers[index])               return;

        Uint16 lo = static_cast<Uint16>(low  * 0xFFFF);
        Uint16 hi = static_cast<Uint16>(high * 0xFFFF);
        SDL_RumbleGamepad(m_sdlControllers[index], lo, hi, 0);
        m_controllers[index].rumbleLow  = low;
        m_controllers[index].rumbleHigh = high;
    }

    // =========================================================
    //  Mouse capture
    // =========================================================

    void SetMouseCaptured(bool captured) override
    {
        // SDL3 dropped the global relative-mouse toggle; it's per-window now. Apply it to the
        // window that currently owns keyboard focus (the one the player is interacting with).
        if (SDL_Window* focus = SDL_GetKeyboardFocus())
            SDL_SetWindowRelativeMouseMode(focus, captured);
        m_mouseCaptured = captured;
    }

    [[nodiscard]] bool IsMouseCaptured() const override { return m_mouseCaptured; }

    [[nodiscard]] bool IsMouseCentered() const override
    {
        return mMouseCentered;
    }
    void SetMouseCentered(bool centered) override
    {
        mMouseCentered = centered;
    }

private:
    // =========================================================
    //  Internal state
    // =========================================================

    KeyboardState    m_keyboard{};
    MouseState       m_mouse{};
    GenericController m_controllers[kMaxControllers]{};
    SDL_Gamepad* m_sdlControllers[kMaxControllers]{};
    bool             m_mouseCaptured = false;
    bool mMouseCentered = false;

    // =========================================================
    //  Controller helpers
    // =========================================================

    void OpenController(SDL_JoystickID instanceId)
    {
        if (!SDL_IsGamepad(instanceId)) return;

        SDL_Gamepad* gc = SDL_OpenGamepad(instanceId);
        if (!gc) return;

        // Find a free slot
        for (int slot = 0; slot < kMaxControllers; ++slot)
        {
            if (!m_sdlControllers[slot])
            {
                m_sdlControllers[slot] = gc;
                m_controllers[slot].connected    = true;
                m_controllers[slot].playerIndex  = static_cast<uint8_t>(slot);

                const char* name = SDL_GetGamepadName(gc);
                if (name)
                    SDL_strlcpy(m_controllers[slot].name, name,
                                sizeof(m_controllers[slot].name));
                return;
            }
        }
        // No free slot – close immediately
        SDL_CloseGamepad(gc);
    }

    void CloseController(int slot)
    {
        if (m_sdlControllers[slot])
        {
            SDL_CloseGamepad(m_sdlControllers[slot]);
            m_sdlControllers[slot] = nullptr;
        }
        m_controllers[slot] = GenericController{};   // reset to default
    }

    int InstanceIDToSlot(SDL_JoystickID id) const
    {
        for (int i = 0; i < kMaxControllers; ++i)
        {
            if (!m_sdlControllers[i]) continue;
            if (SDL_GetGamepadID(m_sdlControllers[i]) == id)
                return i;
        }
        return -1;
    }

    void UpdateController(int slot)
    {
        SDL_Gamepad* gc = m_sdlControllers[slot];
        GenericController&  c  = m_controllers[slot];

        if (!gc) { c.connected = false; return; }

        c.connected = true;

        // --- Face buttons (South/East/West/North) ---
        auto btn = [&](SDL_GamepadButton b) -> bool {
            return SDL_GetGamepadButton(gc, b);
        };

        TickState(c.buttons[static_cast<int>(GenericButton::South)],          btn(SDL_GAMEPAD_BUTTON_SOUTH));
        TickState(c.buttons[static_cast<int>(GenericButton::East)],           btn(SDL_GAMEPAD_BUTTON_EAST));
        TickState(c.buttons[static_cast<int>(GenericButton::West)],           btn(SDL_GAMEPAD_BUTTON_WEST));
        TickState(c.buttons[static_cast<int>(GenericButton::North)],          btn(SDL_GAMEPAD_BUTTON_NORTH));
        TickState(c.buttons[static_cast<int>(GenericButton::LeftBumper)],     btn(SDL_GAMEPAD_BUTTON_LEFT_SHOULDER));
        TickState(c.buttons[static_cast<int>(GenericButton::RightBumper)],    btn(SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER));
        TickState(c.buttons[static_cast<int>(GenericButton::LeftStickClick)], btn(SDL_GAMEPAD_BUTTON_LEFT_STICK));
        TickState(c.buttons[static_cast<int>(GenericButton::RightStickClick)],btn(SDL_GAMEPAD_BUTTON_RIGHT_STICK));
        TickState(c.buttons[static_cast<int>(GenericButton::DPadUp)],         btn(SDL_GAMEPAD_BUTTON_DPAD_UP));
        TickState(c.buttons[static_cast<int>(GenericButton::DPadDown)],       btn(SDL_GAMEPAD_BUTTON_DPAD_DOWN));
        TickState(c.buttons[static_cast<int>(GenericButton::DPadLeft)],       btn(SDL_GAMEPAD_BUTTON_DPAD_LEFT));
        TickState(c.buttons[static_cast<int>(GenericButton::DPadRight)],      btn(SDL_GAMEPAD_BUTTON_DPAD_RIGHT));
        TickState(c.buttons[static_cast<int>(GenericButton::Start)],          btn(SDL_GAMEPAD_BUTTON_START));
        TickState(c.buttons[static_cast<int>(GenericButton::Select)],         btn(SDL_GAMEPAD_BUTTON_BACK));
        TickState(c.buttons[static_cast<int>(GenericButton::Home)],           btn(SDL_GAMEPAD_BUTTON_GUIDE));

        // --- Axes: normalise Sint16 → [-1, 1] ---
        auto axis = [&](SDL_GamepadAxis a) -> float {
            Sint16 raw = SDL_GetGamepadAxis(gc, a);
            return raw >= 0 ? raw / 32767.0f : raw / 32768.0f;
        };

        c.axes[0] =  axis(SDL_GAMEPAD_AXIS_LEFTX);
        c.axes[1] = -axis(SDL_GAMEPAD_AXIS_LEFTY);   // flip Y: up = +1
        c.axes[2] =  axis(SDL_GAMEPAD_AXIS_RIGHTX);
        c.axes[3] = -axis(SDL_GAMEPAD_AXIS_RIGHTY);

        // --- Triggers: Sint16 [0, 32767] → [0, 1] ---
        Sint16 lt = SDL_GetGamepadAxis(gc, SDL_GAMEPAD_AXIS_LEFT_TRIGGER);
        Sint16 rt = SDL_GetGamepadAxis(gc, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER);
        c.triggers[0] = lt / 32767.0f;
        c.triggers[1] = rt / 32767.0f;
    }

    // =========================================================
    //  Key → SDL_Scancode mapping
    // =========================================================

    static SDL_Scancode KeyToScancode(Key key)
    {
        switch (key)
        {
        // Alphabet
        case Key::A: return SDL_SCANCODE_A; case Key::B: return SDL_SCANCODE_B;
        case Key::C: return SDL_SCANCODE_C; case Key::D: return SDL_SCANCODE_D;
        case Key::E: return SDL_SCANCODE_E; case Key::F: return SDL_SCANCODE_F;
        case Key::G: return SDL_SCANCODE_G; case Key::H: return SDL_SCANCODE_H;
        case Key::I: return SDL_SCANCODE_I; case Key::J: return SDL_SCANCODE_J;
        case Key::K: return SDL_SCANCODE_K; case Key::L: return SDL_SCANCODE_L;
        case Key::M: return SDL_SCANCODE_M; case Key::N: return SDL_SCANCODE_N;
        case Key::O: return SDL_SCANCODE_O; case Key::P: return SDL_SCANCODE_P;
        case Key::Q: return SDL_SCANCODE_Q; case Key::R: return SDL_SCANCODE_R;
        case Key::S: return SDL_SCANCODE_S; case Key::T: return SDL_SCANCODE_T;
        case Key::U: return SDL_SCANCODE_U; case Key::V: return SDL_SCANCODE_V;
        case Key::W: return SDL_SCANCODE_W; case Key::X: return SDL_SCANCODE_X;
        case Key::Y: return SDL_SCANCODE_Y; case Key::Z: return SDL_SCANCODE_Z;
        // Digits
        case Key::Num0: return SDL_SCANCODE_0; case Key::Num1: return SDL_SCANCODE_1;
        case Key::Num2: return SDL_SCANCODE_2; case Key::Num3: return SDL_SCANCODE_3;
        case Key::Num4: return SDL_SCANCODE_4; case Key::Num5: return SDL_SCANCODE_5;
        case Key::Num6: return SDL_SCANCODE_6; case Key::Num7: return SDL_SCANCODE_7;
        case Key::Num8: return SDL_SCANCODE_8; case Key::Num9: return SDL_SCANCODE_9;
        // Numpad
        case Key::Numpad0: return SDL_SCANCODE_KP_0; case Key::Numpad1: return SDL_SCANCODE_KP_1;
        case Key::Numpad2: return SDL_SCANCODE_KP_2; case Key::Numpad3: return SDL_SCANCODE_KP_3;
        case Key::Numpad4: return SDL_SCANCODE_KP_4; case Key::Numpad5: return SDL_SCANCODE_KP_5;
        case Key::Numpad6: return SDL_SCANCODE_KP_6; case Key::Numpad7: return SDL_SCANCODE_KP_7;
        case Key::Numpad8: return SDL_SCANCODE_KP_8; case Key::Numpad9: return SDL_SCANCODE_KP_9;
        case Key::NumpadAdd:      return SDL_SCANCODE_KP_PLUS;
        case Key::NumpadSubtract: return SDL_SCANCODE_KP_MINUS;
        case Key::NumpadMultiply: return SDL_SCANCODE_KP_MULTIPLY;
        case Key::NumpadDivide:   return SDL_SCANCODE_KP_DIVIDE;
        case Key::NumpadEnter:    return SDL_SCANCODE_KP_ENTER;
        case Key::NumpadDecimal:  return SDL_SCANCODE_KP_PERIOD;
        case Key::NumLock:        return SDL_SCANCODE_NUMLOCKCLEAR;
        // Function keys
        case Key::F1:  return SDL_SCANCODE_F1;  case Key::F2:  return SDL_SCANCODE_F2;
        case Key::F3:  return SDL_SCANCODE_F3;  case Key::F4:  return SDL_SCANCODE_F4;
        case Key::F5:  return SDL_SCANCODE_F5;  case Key::F6:  return SDL_SCANCODE_F6;
        case Key::F7:  return SDL_SCANCODE_F7;  case Key::F8:  return SDL_SCANCODE_F8;
        case Key::F9:  return SDL_SCANCODE_F9;  case Key::F10: return SDL_SCANCODE_F10;
        case Key::F11: return SDL_SCANCODE_F11; case Key::F12: return SDL_SCANCODE_F12;
        // Modifiers
        case Key::LeftShift:  return SDL_SCANCODE_LSHIFT;
        case Key::RightShift: return SDL_SCANCODE_RSHIFT;
        case Key::LeftCtrl:   return SDL_SCANCODE_LCTRL;
        case Key::RightCtrl:  return SDL_SCANCODE_RCTRL;
        case Key::LeftAlt:    return SDL_SCANCODE_LALT;
        case Key::RightAlt:   return SDL_SCANCODE_RALT;
        case Key::LeftSuper:  return SDL_SCANCODE_LGUI;
        case Key::RightSuper: return SDL_SCANCODE_RGUI;
        // Navigation
        case Key::Up:       return SDL_SCANCODE_UP;
        case Key::Down:     return SDL_SCANCODE_DOWN;
        case Key::Left:     return SDL_SCANCODE_LEFT;
        case Key::Right:    return SDL_SCANCODE_RIGHT;
        case Key::Home:     return SDL_SCANCODE_HOME;
        case Key::End:      return SDL_SCANCODE_END;
        case Key::PageUp:   return SDL_SCANCODE_PAGEUP;
        case Key::PageDown: return SDL_SCANCODE_PAGEDOWN;
        case Key::Insert:   return SDL_SCANCODE_INSERT;
        case Key::Delete:   return SDL_SCANCODE_DELETE;
        // Special
        case Key::Enter:       return SDL_SCANCODE_RETURN;
        case Key::Backspace:   return SDL_SCANCODE_BACKSPACE;
        case Key::Tab:         return SDL_SCANCODE_TAB;
        case Key::Space:       return SDL_SCANCODE_SPACE;
        case Key::Escape:      return SDL_SCANCODE_ESCAPE;
        case Key::CapsLock:    return SDL_SCANCODE_CAPSLOCK;
        case Key::ScrollLock:  return SDL_SCANCODE_SCROLLLOCK;
        case Key::PrintScreen: return SDL_SCANCODE_PRINTSCREEN;
        case Key::Pause:       return SDL_SCANCODE_PAUSE;
        // Punctuation
        case Key::Comma:        return SDL_SCANCODE_COMMA;
        case Key::Period:       return SDL_SCANCODE_PERIOD;
        case Key::Slash:        return SDL_SCANCODE_SLASH;
        case Key::Backslash:    return SDL_SCANCODE_BACKSLASH;
        case Key::Semicolon:    return SDL_SCANCODE_SEMICOLON;
        case Key::Apostrophe:   return SDL_SCANCODE_APOSTROPHE;
        case Key::GraveAccent:  return SDL_SCANCODE_GRAVE;
        case Key::LeftBracket:  return SDL_SCANCODE_LEFTBRACKET;
        case Key::RightBracket: return SDL_SCANCODE_RIGHTBRACKET;
        case Key::Minus:        return SDL_SCANCODE_MINUS;
        case Key::Equal:        return SDL_SCANCODE_EQUALS;

        default: return SDL_SCANCODE_UNKNOWN;
        }
    }
};

#endif // __linux__
