#pragma once
#include "InputInfo.h"
#include <array>

// ============================================================
//  PLATFORM INPUT BACKEND  –  abstract interface
//
//  Usage per frame:
//      backend->BeginFrame();          // pump OS events
//      backend->Update();              // advance button states
//      auto& kb  = backend->GetKeyboard();
//      auto& ms  = backend->GetMouse();
//      auto& pad = backend->GetController(0);
//      ...
//      backend->EndFrame();            // flush one-frame states
// ============================================================

static constexpr int kMaxControllers = 4;

class PlatformInputBackend
{
public:
    virtual ~PlatformInputBackend() = default;

    // ---------------------------------------------------------
    //  Lifecycle
    // ---------------------------------------------------------

    /// Called once after construction – initialise platform APIs.
    virtual bool Init()        = 0;

    /// Called once before destruction – release platform resources.
    virtual void Shutdown()    = 0;

    /// Pump OS event queue (call at the START of each frame).
    virtual void BeginFrame()  = 0;

    /// Advance ButtonState machine (Pressed→Held, JustReleased→Released).
    /// Call AFTER BeginFrame, BEFORE reading state.
    virtual void Update()      = 0;

    /// Reset per-frame delta data (scroll, mouse delta, etc.).
    /// Call at the END of each frame.
    virtual void EndFrame()    = 0;

    // ---------------------------------------------------------
    //  State accessors (read-only for consumers)
    // ---------------------------------------------------------

    [[nodiscard]] virtual const KeyboardState&   GetKeyboard()                         const = 0;
    [[nodiscard]] virtual const MouseState&      GetMouse()                            const = 0;

    /// Returns a generic controller slot (0 – kMaxControllers-1).
    [[nodiscard]] virtual const GenericController& GetController(int index)            const = 0;

    /// Convenience: returns the first connected controller, or nullptr.
    [[nodiscard]] const GenericController* GetFirstConnected() const
    {
        for (int i = 0; i < kMaxControllers; ++i)
            if (GetController(i).connected)
                return &GetController(i);
        return nullptr;
    }

    // ---------------------------------------------------------
    //  Rumble
    // ---------------------------------------------------------

    /// Set rumble motors for a controller slot, values in [0, 1].
    virtual void SetRumble(int index, float low, float high) = 0;

    // ---------------------------------------------------------
    //  Mouse mode
    // ---------------------------------------------------------

    /// Lock cursor to window center and hide it (FPS-style).
    virtual void SetMouseCaptured(bool captured) = 0;
    [[nodiscard]] virtual bool IsMouseCaptured() const = 0;

protected:
    // ---------------------------------------------------------
    //  Shared helper: advance a single ButtonState each frame
    // ---------------------------------------------------------
    static void TickState(ButtonState& state, bool isPhysicallyDown)
    {
        switch (state)
        {
            case ButtonState::Released:
                if (isPhysicallyDown) state = ButtonState::Pressed;
                break;
            case ButtonState::Pressed:
                state = isPhysicallyDown ? ButtonState::Held : ButtonState::JustReleased;
                break;
            case ButtonState::Held:
                if (!isPhysicallyDown) state = ButtonState::JustReleased;
                break;
            case ButtonState::JustReleased:
                state = isPhysicallyDown ? ButtonState::Pressed : ButtonState::Released;
                break;
        }
    }
};
