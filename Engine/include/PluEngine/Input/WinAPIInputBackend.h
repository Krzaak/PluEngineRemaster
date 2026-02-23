#pragma once
#include "PluEngine/Core.h"
#ifdef PLU_PLATFORM_WINDOWS

#include "PlatformInputBackend.h"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <Xinput.h>
#pragma comment(lib, "Xinput.lib")

// ============================================================
//  WinAPIInputBackend  –  Windows (WinAPI + XInput) stub
//
//  TODO: This is a skeleton. Fill in WM_* message handling
//        in your window procedure (WndProc) and call
//        FeedMessage() for relevant messages, or hook into
//        Raw Input for lower-latency mouse data.
//
//  Dependencies:  Xinput  (already ships with Windows SDK)
// ============================================================

class WinAPIInputBackend final : public PlatformInputBackend
{
public:
    // =========================================================
    //  Init / Shutdown
    // =========================================================

    bool Init() override
    {
        // Raw Input registration for keyboard + mouse (optional, more accurate)
        RAWINPUTDEVICE rid[2] = {};

        // Keyboard
        rid[0].usUsagePage = 0x01;
        rid[0].usUsage     = 0x06;
        rid[0].dwFlags     = RIDEV_NOLEGACY;   // remove if you need WM_CHAR too
        rid[0].hwndTarget  = nullptr;

        // Mouse
        rid[1].usUsagePage = 0x01;
        rid[1].usUsage     = 0x02;
        rid[1].dwFlags     = 0;
        rid[1].hwndTarget  = nullptr;

        // RegisterRawInputDevices(rid, 2, sizeof(rid[0]));
        // ^ Uncomment when you want Raw Input; leave commented for WM_KEY* path.

        return true;
    }

    void Shutdown() override { /* nothing to clean up */ }

    // =========================================================
    //  Frame lifecycle
    // =========================================================

    void BeginFrame() override
    {
        // Reset per-frame scroll accumulator before processing messages.
        // Actual message pumping should happen in your WndProc / message loop.
        // Call FeedMessage() from WndProc for WM_KEYDOWN, WM_KEYUP,
        // WM_MOUSEMOVE, WM_MOUSEWHEEL, WM_LBUTTONDOWN, etc.
    }

    void Update() override
    {
        // --- Keyboard: query current state via GetAsyncKeyState ---
        for (int k = 0; k < static_cast<int>(Key::Count); ++k)
        {
            int vk = KeyToVirtualKey(static_cast<Key>(k));
            bool down = vk && (GetAsyncKeyState(vk) & 0x8000) != 0;
            TickState(m_keyboard.keys[k], down);
        }

        // --- Mouse buttons ---
        TickState(m_mouse.buttons[static_cast<int>(MouseButton::Left)],
                  (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0);
        TickState(m_mouse.buttons[static_cast<int>(MouseButton::Right)],
                  (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0);
        TickState(m_mouse.buttons[static_cast<int>(MouseButton::Middle)],
                  (GetAsyncKeyState(VK_MBUTTON) & 0x8000) != 0);
        TickState(m_mouse.buttons[static_cast<int>(MouseButton::Extra1)],
                  (GetAsyncKeyState(VK_XBUTTON1) & 0x8000) != 0);
        TickState(m_mouse.buttons[static_cast<int>(MouseButton::Extra2)],
                  (GetAsyncKeyState(VK_XBUTTON2) & 0x8000) != 0);

        // --- Mouse position ---
        POINT pt{};
        GetCursorPos(&pt);
        // ScreenToClient(m_hWnd, &pt);   // uncomment and set m_hWnd for client coords
        m_mouse.deltaX = static_cast<float>(pt.x) - m_mouse.x;
        m_mouse.deltaY = static_cast<float>(pt.y) - m_mouse.y;
        m_mouse.x = static_cast<float>(pt.x);
        m_mouse.y = static_cast<float>(pt.y);

        // --- XInput controllers ---
        for (DWORD i = 0; i < kMaxControllers; ++i)
            UpdateXInputController(i);
    }

    void EndFrame() override
    {
        m_mouse.scrollX = 0.0f;
        m_mouse.scrollY = 0.0f;
    }

    // =========================================================
    //  WndProc hook – call this from your window procedure
    //  for: WM_MOUSEWHEEL, WM_MOUSEHWHEEL
    // =========================================================

    void FeedMessage(UINT msg, WPARAM wParam, LPARAM /*lParam*/)
    {
        if (msg == WM_MOUSEWHEEL)
            m_mouse.scrollY += GET_WHEEL_DELTA_WPARAM(wParam) / static_cast<float>(WHEEL_DELTA);
        else if (msg == WM_MOUSEHWHEEL)
            m_mouse.scrollX += GET_WHEEL_DELTA_WPARAM(wParam) / static_cast<float>(WHEEL_DELTA);
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
    //  Rumble (XInput)
    // =========================================================

    void SetRumble(int index, float low, float high) override
    {
        XINPUT_VIBRATION vib{};
        vib.wLeftMotorSpeed  = static_cast<WORD>(low  * 0xFFFF);
        vib.wRightMotorSpeed = static_cast<WORD>(high * 0xFFFF);
        XInputSetState(static_cast<DWORD>(index), &vib);
        m_controllers[index].rumbleLow  = low;
        m_controllers[index].rumbleHigh = high;
    }

    // =========================================================
    //  Mouse capture
    // =========================================================

    void SetMouseCaptured(bool captured) override
    {
        ShowCursor(!captured);
        // For actual locking, call ClipCursor() with your window rect.
        m_mouseCaptured = captured;
    }

    [[nodiscard]] bool IsMouseCaptured() const override { return m_mouseCaptured; }

private:
    KeyboardState     m_keyboard{};
    MouseState        m_mouse{};
    GenericController m_controllers[kMaxControllers]{};
    bool              m_mouseCaptured = false;
    // HWND           m_hWnd = nullptr;   // set if you need client-space mouse coords

    // =========================================================
    //  XInput controller update
    // =========================================================

    void UpdateXInputController(DWORD index)
    {
        XINPUT_STATE state{};
        GenericController& c = m_controllers[index];

        if (XInputGetState(index, &state) != ERROR_SUCCESS)
        {
            c.connected = false;
            return;
        }

        c.connected   = true;
        c.playerIndex = static_cast<uint8_t>(index);
        strcpy_s(c.name, sizeof(c.name), "Xbox Controller");

        const XINPUT_GAMEPAD& gp = state.Gamepad;

        auto btn = [&](WORD mask) -> bool { return (gp.wButtons & mask) != 0; };

        TickState(c.buttons[static_cast<int>(GenericButton::South)],           btn(XINPUT_GAMEPAD_A));
        TickState(c.buttons[static_cast<int>(GenericButton::East)],            btn(XINPUT_GAMEPAD_B));
        TickState(c.buttons[static_cast<int>(GenericButton::West)],            btn(XINPUT_GAMEPAD_X));
        TickState(c.buttons[static_cast<int>(GenericButton::North)],           btn(XINPUT_GAMEPAD_Y));
        TickState(c.buttons[static_cast<int>(GenericButton::LeftBumper)],      btn(XINPUT_GAMEPAD_LEFT_SHOULDER));
        TickState(c.buttons[static_cast<int>(GenericButton::RightBumper)],     btn(XINPUT_GAMEPAD_RIGHT_SHOULDER));
        TickState(c.buttons[static_cast<int>(GenericButton::LeftStickClick)],  btn(XINPUT_GAMEPAD_LEFT_THUMB));
        TickState(c.buttons[static_cast<int>(GenericButton::RightStickClick)], btn(XINPUT_GAMEPAD_RIGHT_THUMB));
        TickState(c.buttons[static_cast<int>(GenericButton::DPadUp)],          btn(XINPUT_GAMEPAD_DPAD_UP));
        TickState(c.buttons[static_cast<int>(GenericButton::DPadDown)],        btn(XINPUT_GAMEPAD_DPAD_DOWN));
        TickState(c.buttons[static_cast<int>(GenericButton::DPadLeft)],        btn(XINPUT_GAMEPAD_DPAD_LEFT));
        TickState(c.buttons[static_cast<int>(GenericButton::DPadRight)],       btn(XINPUT_GAMEPAD_DPAD_RIGHT));
        TickState(c.buttons[static_cast<int>(GenericButton::Start)],           btn(XINPUT_GAMEPAD_START));
        TickState(c.buttons[static_cast<int>(GenericButton::Select)],          btn(XINPUT_GAMEPAD_BACK));

        // Sticks: SHORT [-32768, 32767] → [-1, 1], Y flipped (up = +1)
        auto normaliseAxis = [](SHORT v) -> float {
            return v >= 0 ? v / 32767.0f : v / 32768.0f;
        };
        c.axes[0] =  normaliseAxis(gp.sThumbLX);
        c.axes[1] =  normaliseAxis(gp.sThumbLY);   // already +up in XInput
        c.axes[2] =  normaliseAxis(gp.sThumbRX);
        c.axes[3] =  normaliseAxis(gp.sThumbRY);

        // Triggers: BYTE [0, 255] → [0, 1]
        c.triggers[0] = gp.bLeftTrigger  / 255.0f;
        c.triggers[1] = gp.bRightTrigger / 255.0f;
    }

    // =========================================================
    //  Key → Virtual Key mapping
    // =========================================================

    static int KeyToVirtualKey(Key key)
    {
        switch (key)
        {
        case Key::A: return 'A'; case Key::B: return 'B'; case Key::C: return 'C';
        case Key::D: return 'D'; case Key::E: return 'E'; case Key::F: return 'F';
        case Key::G: return 'G'; case Key::H: return 'H'; case Key::I: return 'I';
        case Key::J: return 'J'; case Key::K: return 'K'; case Key::L: return 'L';
        case Key::M: return 'M'; case Key::N: return 'N'; case Key::O: return 'O';
        case Key::P: return 'P'; case Key::Q: return 'Q'; case Key::R: return 'R';
        case Key::S: return 'S'; case Key::T: return 'T'; case Key::U: return 'U';
        case Key::V: return 'V'; case Key::W: return 'W'; case Key::X: return 'X';
        case Key::Y: return 'Y'; case Key::Z: return 'Z';
        case Key::Num0: return '0'; case Key::Num1: return '1'; case Key::Num2: return '2';
        case Key::Num3: return '3'; case Key::Num4: return '4'; case Key::Num5: return '5';
        case Key::Num6: return '6'; case Key::Num7: return '7'; case Key::Num8: return '8';
        case Key::Num9: return '9';
        case Key::Numpad0: return VK_NUMPAD0; case Key::Numpad1: return VK_NUMPAD1;
        case Key::Numpad2: return VK_NUMPAD2; case Key::Numpad3: return VK_NUMPAD3;
        case Key::Numpad4: return VK_NUMPAD4; case Key::Numpad5: return VK_NUMPAD5;
        case Key::Numpad6: return VK_NUMPAD6; case Key::Numpad7: return VK_NUMPAD7;
        case Key::Numpad8: return VK_NUMPAD8; case Key::Numpad9: return VK_NUMPAD9;
        case Key::NumpadAdd:      return VK_ADD;
        case Key::NumpadSubtract: return VK_SUBTRACT;
        case Key::NumpadMultiply: return VK_MULTIPLY;
        case Key::NumpadDivide:   return VK_DIVIDE;
        case Key::NumpadEnter:    return VK_RETURN;   // distinguish via scan code if needed
        case Key::NumpadDecimal:  return VK_DECIMAL;
        case Key::NumLock:        return VK_NUMLOCK;
        case Key::F1:  return VK_F1;  case Key::F2:  return VK_F2;
        case Key::F3:  return VK_F3;  case Key::F4:  return VK_F4;
        case Key::F5:  return VK_F5;  case Key::F6:  return VK_F6;
        case Key::F7:  return VK_F7;  case Key::F8:  return VK_F8;
        case Key::F9:  return VK_F9;  case Key::F10: return VK_F10;
        case Key::F11: return VK_F11; case Key::F12: return VK_F12;
        case Key::LeftShift:  return VK_LSHIFT;  case Key::RightShift: return VK_RSHIFT;
        case Key::LeftCtrl:   return VK_LCONTROL; case Key::RightCtrl: return VK_RCONTROL;
        case Key::LeftAlt:    return VK_LMENU;   case Key::RightAlt:   return VK_RMENU;
        case Key::LeftSuper:  return VK_LWIN;    case Key::RightSuper: return VK_RWIN;
        case Key::Up:       return VK_UP;       case Key::Down:     return VK_DOWN;
        case Key::Left:     return VK_LEFT;     case Key::Right:    return VK_RIGHT;
        case Key::Home:     return VK_HOME;     case Key::End:      return VK_END;
        case Key::PageUp:   return VK_PRIOR;    case Key::PageDown: return VK_NEXT;
        case Key::Insert:   return VK_INSERT;   case Key::Delete:   return VK_DELETE;
        case Key::Enter:       return VK_RETURN;
        case Key::Backspace:   return VK_BACK;
        case Key::Tab:         return VK_TAB;
        case Key::Space:       return VK_SPACE;
        case Key::Escape:      return VK_ESCAPE;
        case Key::CapsLock:    return VK_CAPITAL;
        case Key::ScrollLock:  return VK_SCROLL;
        case Key::PrintScreen: return VK_SNAPSHOT;
        case Key::Pause:       return VK_PAUSE;
        case Key::Comma:        return VK_OEM_COMMA;
        case Key::Period:       return VK_OEM_PERIOD;
        case Key::Slash:        return VK_OEM_2;
        case Key::Backslash:    return VK_OEM_5;
        case Key::Semicolon:    return VK_OEM_1;
        case Key::Apostrophe:   return VK_OEM_7;
        case Key::GraveAccent:  return VK_OEM_3;
        case Key::LeftBracket:  return VK_OEM_4;
        case Key::RightBracket: return VK_OEM_6;
        case Key::Minus:        return VK_OEM_MINUS;
        case Key::Equal:        return VK_OEM_PLUS;
        default: return 0;
        }
    }
};

#endif // _WIN32
