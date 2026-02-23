#pragma once
#include <cstdint>

#include "PluEngine/Core.h"

// ============================================================
//  KEY ENUM
// ============================================================

enum class Key : UInt16
{
    // --- Alphabet ---
    A, B, C, D, E, F, G, H, I, J, K, L, M,
    N, O, P, Q, R, S, T, U, V, W, X, Y, Z,

    // --- Digits (row) ---
    Num0, Num1, Num2, Num3, Num4,
    Num5, Num6, Num7, Num8, Num9,

    // --- Numpad ---
    Numpad0, Numpad1, Numpad2, Numpad3, Numpad4,
    Numpad5, Numpad6, Numpad7, Numpad8, Numpad9,
    NumpadAdd, NumpadSubtract, NumpadMultiply, NumpadDivide,
    NumpadEnter, NumpadDecimal, NumLock,

    // --- Function keys ---
    F1, F2, F3, F4, F5, F6,
    F7, F8, F9, F10, F11, F12,

    // --- Modifiers ---
    LeftShift, RightShift,
    LeftCtrl,  RightCtrl,
    LeftAlt,   RightAlt,
    LeftSuper, RightSuper,      // Win / Cmd

    // --- Navigation ---
    Up, Down, Left, Right,
    Home, End, PageUp, PageDown,
    Insert, Delete,

    // --- Special ---
    Enter, Backspace, Tab, Space, Escape,
    CapsLock, ScrollLock,
    PrintScreen, Pause,

    // --- Punctuation / Symbols ---
    Comma, Period, Slash, Backslash,
    Semicolon, Apostrophe, GraveAccent,
    LeftBracket, RightBracket,
    Minus, Equal,

    Count,
    Unknown = 0xFFFF
};

// ============================================================
//  BUTTON STATE
// ============================================================

enum class ButtonState : UInt8
{
    Released = 0,   // not held, was not pressed last frame
    Pressed,        // just pressed this frame
    Held,           // held for more than one frame
    JustReleased    // released this frame
};

// ============================================================
//  MOUSE
// ============================================================

enum class MouseButton : UInt8
{
    Left = 0,
    Right,
    Middle,
    Extra1,
    Extra2,
    Count
};

struct MouseState
{
    // Position in window pixels (top-left = 0,0)
    float x            = 0.0f;
    float y            = 0.0f;

    // Delta since last frame
    float deltaX       = 0.0f;
    float deltaY       = 0.0f;

    // Scroll wheel (vertical / horizontal)
    float scrollX      = 0.0f;
    float scrollY      = 0.0f;

    // Per-button state
    ButtonState buttons[static_cast<int>(MouseButton::Count)] = {};

    // Helpers
    [[nodiscard]] bool IsDown(MouseButton btn) const
    {
        auto s = buttons[static_cast<int>(btn)];
        return s == ButtonState::Pressed || s == ButtonState::Held;
    }

    [[nodiscard]] bool JustPressed(MouseButton btn) const
    {
        return buttons[static_cast<int>(btn)] == ButtonState::Pressed;
    }

    [[nodiscard]] bool JustReleased(MouseButton btn) const
    {
        return buttons[static_cast<int>(btn)] == ButtonState::JustReleased;
    }
};

// ============================================================
//  XBOX CONTROLLER
// ============================================================

enum class XboxButton : UInt8
{
    A = 0,
    B,
    X,
    Y,
    LeftBumper,
    RightBumper,
    LeftStickClick,
    RightStickClick,
    DPadUp,
    DPadDown,
    DPadLeft,
    DPadRight,
    Start,
    Back,       // also called "View" / "Select"
    Guide,      // Xbox-button in the middle
    Count
};

struct XboxController
{
    // Whether a physical controller is connected
    bool connected       = false;

    // Index (0–3 for local co-op)
    UInt8 playerIndex  = 0;

    // Face / shoulder / D-pad / system buttons
    ButtonState buttons[static_cast<int>(XboxButton::Count)] = {};

    // Thumbsticks – range [-1, 1] per axis
    float leftStickX     = 0.0f;
    float leftStickY     = 0.0f;
    float rightStickX    = 0.0f;
    float rightStickY    = 0.0f;

    // Triggers – range [0, 1]
    float leftTrigger    = 0.0f;
    float rightTrigger   = 0.0f;

    // Rumble – range [0, 1]  (write to set vibration)
    float rumbleLow      = 0.0f;
    float rumbleHigh     = 0.0f;

    // Helpers
    [[nodiscard]] bool IsDown(XboxButton btn) const
    {
        auto s = buttons[static_cast<int>(btn)];
        return s == ButtonState::Pressed || s == ButtonState::Held;
    }

    [[nodiscard]] bool JustPressed(XboxButton btn) const
    {
        return buttons[static_cast<int>(btn)] == ButtonState::Pressed;
    }

    [[nodiscard]] bool JustReleased(XboxButton btn) const
    {
        return buttons[static_cast<int>(btn)] == ButtonState::JustReleased;
    }
};

// ============================================================
//  GENERIC / GAMEPAD CONTROLLER
// ============================================================

enum class GenericButton : UInt8
{
    // South / East / West / North (layout-agnostic face buttons)
    South = 0,  // typically A / Cross
    East,       // typically B / Circle
    West,       // typically X / Square
    North,      // typically Y / Triangle

    LeftBumper,
    RightBumper,
    LeftStickClick,
    RightStickClick,
    DPadUp,
    DPadDown,
    DPadLeft,
    DPadRight,
    Start,
    Select,
    Home,
    Count
};

struct GenericController
{
    bool    connected    = false;
    UInt8 playerIndex  = 0;

    // Optional human-readable name (filled by platform layer)
    char name[64]        = "Unknown Controller";

    // Buttons
    ButtonState buttons[static_cast<int>(GenericButton::Count)] = {};

    // Up to 4 analog axes – range [-1, 1]
    // Convention: axis[0]=leftX, axis[1]=leftY, axis[2]=rightX, axis[3]=rightY
    static constexpr int kMaxAxes = 8;
    float axes[kMaxAxes] = {};

    // Up to 4 triggers – range [0, 1]
    static constexpr int kMaxTriggers = 4;
    float triggers[kMaxTriggers] = {};

    // Rumble – range [0, 1]
    float rumbleLow      = 0.0f;
    float rumbleHigh     = 0.0f;

    // Helpers
    [[nodiscard]] bool IsDown(GenericButton btn) const
    {
        auto s = buttons[static_cast<int>(btn)];
        return s == ButtonState::Pressed || s == ButtonState::Held;
    }

    [[nodiscard]] bool JustPressed(GenericButton btn) const
    {
        return buttons[static_cast<int>(btn)] == ButtonState::Pressed;
    }

    [[nodiscard]] bool JustReleased(GenericButton btn) const
    {
        return buttons[static_cast<int>(btn)] == ButtonState::JustReleased;
    }
};

// ============================================================
//  KEYBOARD STATE  (thin wrapper over Key enum)
// ============================================================

struct KeyboardState
{
    ButtonState keys[static_cast<int>(Key::Count)] = {};

    [[nodiscard]] bool IsDown(Key key) const
    {
        auto s = keys[static_cast<int>(key)];
        return s == ButtonState::Pressed || s == ButtonState::Held;
    }

    [[nodiscard]] bool JustPressed(Key key) const
    {
        return keys[static_cast<int>(key)] == ButtonState::Pressed;
    }

    [[nodiscard]] bool JustReleased(Key key) const
    {
        return keys[static_cast<int>(key)] == ButtonState::JustReleased;
    }
};
