//
// Created by Plutex on 1/5/26.
//

#include "SdlWindow.h"

#include "imgui_impl_sdl3.h"
#include "PluEngine/Application.h"
#include "PluEngine/Input/InputManager.h"
#include "PluEngine/Input/SDLInputBackend.h"
#include "PluEngine/Threading/ThreadAffinity.h"

#ifdef PLU_PLATFORM_LINUX

namespace Plu
{
	#include "SdlWindow.h"
    #include <GL/gl.h>

    void SDLWindow::InitSDL()
    {
        SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_GAMEPAD);

        // OpenGL 3.3 Core
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
                            SDL_GL_CONTEXT_PROFILE_CORE);
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    }

    GameHashMap<int, SDLWindow*> gSDLWindows;

    void SDLWindow::HandleSDLEvents()
    {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            // In SDL3 every windowed event struct keeps windowID at the same offset (right after the
            // common header), so drop events no longer need special-casing - e.window.windowID and
            // e.drop.windowID alias the same field.
            Uint32 windowID = e.window.windowID;

            if (gSDLWindows.Contains(windowID)) {
                gSDLWindows[windowID]->OnEventSDL(&e);
            } else {
                if (e.type == SDL_EVENT_QUIT) {
                    for (auto window : gSDLWindows) {
                        window.second->Close();
                    }
                }
            }
        }
    }

    void SDLWindow::SetCursorVisibility(bool visible)
    {
        if (visible) SDL_ShowCursor(); else SDL_HideCursor();
        // SDL3 dropped the global relative-mouse toggle; it's now per-window.
        SDL_SetWindowRelativeMouseMode(mWindow, !visible);
    }

    SDLWindow::SDLWindow()
    = default;

    SDLWindow::~SDLWindow()
    {
        Shutdown();
    }

#define MOUSE_GRAB_PADDING 10

    SDL_HitTestResult HitTestCallback(SDL_Window *Window, const SDL_Point *Area, void *Data)
    {
        int Width, Height;
        SDL_GetWindowSize(Window, &Width, &Height);

        SDLWindow** windowFind = gSDLWindows.Find(SDL_GetWindowID(Window));
        if (!windowFind) return SDL_HITTEST_DRAGGABLE;
        SDLWindow* window = *windowFind;

        if (window->ImGuiItemHovered) {
            return SDL_HITTEST_NORMAL;
        }

        if(Area->y < MOUSE_GRAB_PADDING)
        {
            if(Area->x < MOUSE_GRAB_PADDING)
            {
                return SDL_HITTEST_RESIZE_TOPLEFT;
            }
            else if(Area->x > Width - MOUSE_GRAB_PADDING)
            {
                return SDL_HITTEST_RESIZE_TOPRIGHT;
            }
            else
            {
                return SDL_HITTEST_RESIZE_TOP;
            }
        }
        else if(Area->y > Height - MOUSE_GRAB_PADDING)
        {
            if(Area->x < MOUSE_GRAB_PADDING)
            {
                return SDL_HITTEST_RESIZE_BOTTOMLEFT;
            }
            else if(Area->x > Width - MOUSE_GRAB_PADDING)
            {
                return SDL_HITTEST_RESIZE_BOTTOMRIGHT;
            }
            else
            {
                return SDL_HITTEST_RESIZE_BOTTOM;
            }
        }
        else if(Area->x < MOUSE_GRAB_PADDING)
        {
            return SDL_HITTEST_RESIZE_LEFT;
        }
        else if(Area->x > Width - MOUSE_GRAB_PADDING)
        {
            return SDL_HITTEST_RESIZE_RIGHT;
        }
        else if(Area->y < 30)
        {
            return SDL_HITTEST_DRAGGABLE;
        }

        return SDL_HITTEST_NORMAL; // SDL_HITTEST_NORMAL <- Windows behaviour
    }

    void SDLWindow::Init()
    {
        // SDL3 creates windows shown by default and dropped the x/y args from SDL_CreateWindow;
        // create hidden, center explicitly, then SDL_ShowWindow() below reveals it in place.
        // HIGH_PIXEL_DENSITY makes the GL drawable match the display's native pixels (not the
        // scaled-down logical size), so 3D + ImGui render crisp on HiDPI instead of being upscaled
        // by the compositor. Paired with GetWidth/GetHeight returning pixels (SDL_GetWindowSizeInPixels).
        SDL_WindowFlags flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN |
                                SDL_WINDOW_HIGH_PIXEL_DENSITY;
        if (mProperties.Borderless) {
            flags |= SDL_WINDOW_BORDERLESS;
        }
        mWindow = SDL_CreateWindow(
            mProperties.Title.CStr(),
            mProperties.Width,
            mProperties.Height,
            flags
        );
        SDL_SetWindowPosition(mWindow, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);

        static SDL_GLContext glContext = nullptr;
        if (!glContext)
            glContext = SDL_GL_CreateContext(mWindow);
        mGLContext = glContext;
        SDL_GL_MakeCurrent(mWindow, mGLContext);
        CreateImGuiContext();

        SDL_SetWindowHitTest(mWindow, HitTestCallback, nullptr);

        SetVSyncEnabled(false);

        mWindowID = SDL_GetWindowID(mWindow);
        PLU_CORE_INFO("New Window created with ID {}", mWindowID);
        mRunning = true;

        gSDLWindows.Insert(mWindowID, this);

        SDL_ShowWindow(mWindow);
    }

    void SDLWindow::OnUpdate(float deltaTime)
    {
    }

    void SDLWindow::OnEventSDL(SDL_Event *e)
    {
        ImGui::SetCurrentContext(mImGuiContext);

        if (e->type == SDL_EVENT_QUIT)
            mRunning = false;

        dynamic_cast<SDLInputBackend*>(mApplicationInfo->AppInputManager->GetInputBackend().GetRaw())->FeedEvent(*e);
        if (UpdateImGui) if (ImGui_ImplSDL3_ProcessEvent(e)) return;
        if (e->type == SDL_EVENT_DROP_FILE) {
            // In SDL3 drop.data is a const char* owned by SDL - copy it, do NOT SDL_free it.
            Path droppedPath(e->drop.data);
            DispatchEvent("FileDropped", &droppedPath);
            PLU_CORE_TRACE("Drop File, {}", droppedPath.CStr());
        }
        if (e->type == SDL_EVENT_DROP_BEGIN) {
            // drop.data is NULL on begin/complete. Fires when a drag carrying a payload enters the
            // window, before any actual drop - the only hook for "hovering with something to drop" UI.
            DispatchEvent("FileDragEntered", nullptr);
            PLU_CORE_TRACE("Drop Begin");
        }
        if (e->type == SDL_EVENT_DROP_COMPLETE) {
            DispatchEvent("FileDragEnded", nullptr);
            PLU_CORE_TRACE("Drop End");
        }

        // SDL3 replaced the SDL_WINDOWEVENT umbrella with distinct per-action event types.
        switch (e->type) {
            case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
            {
                // Deferred: don't flip mRunning here. The app may want to confirm (unsaved
                // assets) before actually closing - see Application::Run()'s subscription and
                // Close() below, which is what actually flips mRunning.
                DispatchEvent("WindowCloseRequested", nullptr);
                break;
            }
            case SDL_EVENT_WINDOW_RESIZED:
            {
                mProperties.Width  = e->window.data1;
                mProperties.Height = e->window.data2;
                break;
            }
            case SDL_EVENT_WINDOW_FOCUS_LOST:
            case SDL_EVENT_WINDOW_FOCUS_GAINED:
            default:
                break;
        }
    }

    void SDLWindow::Shutdown()
    {
        if (mWindow)
        {
            SDL_DestroyWindow(mWindow);
            mWindow = nullptr;
        }
    }

    bool SDLWindow::IsRunning()
    {
        return mRunning;
    }

    void SDLWindow::Close()
    {
        mRunning = false;
        DispatchEvent("WindowClosed", &mWindowID);
    }

    int SDLWindow::GetWindowID()
    {
        return mWindowID;
    }

    int SDLWindow::GetWidth()
    {
        // Pixel size (not logical points): the render path uses this for the GL viewport, main
        // framebuffer and blit - they must match the HIGH_PIXEL_DENSITY drawable to stay crisp.
        int w,h;
        SDL_GetWindowSizeInPixels(mWindow, &w, &h);
        return w;
    }

    int SDLWindow::GetHeight()
    {
        int w,h;
        SDL_GetWindowSizeInPixels(mWindow, &w, &h);
        return h;
    }

    void SDLWindow::Minimize()
    {
        SDL_MinimizeWindow(mWindow);
    }

    void SDLWindow::Maximize()
    {
        SDL_MaximizeWindow(mWindow);
    }

    bool SDLWindow::IsWindowMinimized()
    {
        return SDL_GetWindowFlags(mWindow) & SDL_WINDOW_MINIMIZED;
    }

    bool SDLWindow::IsWindowMaximized()
    {
        return SDL_GetWindowFlags(mWindow) & SDL_WINDOW_MAXIMIZED;
    }

    bool SDLWindow::IsVSyncEnabled()
    {
        return mVSyncEnabled;
    }

    void SDLWindow::SetVSyncEnabled(bool enabled)
    {
        if (IsOnMainThread()) {
            mRequestedVSync = enabled;
            return;
        }
        SDL_GL_SetSwapInterval(enabled ? 1 : 0);
        if (const char* msg = SDL_GetError()) {
            if (strlen(msg) != 0) {
                PLU_CORE_ERROR("SDL error: {}", msg);
            }
        }
        mVSyncEnabled = enabled;
    }

    void* SDLWindow::GetWindowHandle()
    {
        return mWindow;
    }

    void * SDLWindow::GetGLContext()
    {
        return mGLContext;
    }

    void SDLWindow::MakeGLContextCurrent()
    {
        SDL_GL_MakeCurrent(mWindow, mGLContext);
    }

    void SDLWindow::ReleaseGLContext()
    {
        SDL_GL_MakeCurrent(mWindow, nullptr);
    }

    void SDLWindow::SwapBuffer()
    {
        SDL_GL_SwapWindow(mWindow);
        if (mRequestedVSync != mVSyncEnabled && !IsOnMainThread()) {
            SetVSyncEnabled(mRequestedVSync);
        }
    }

    void SDLWindow::SetWindowTitle(String title)
    {
        SDL_SetWindowTitle(mWindow, title.CStr());
    }

    void SDLWindow::SetCursorPosition(IVec2 pos)
    {
        SDL_WarpMouseInWindow(mWindow, pos.x, pos.y);
    }

    IVec2 SDLWindow::GetCursorPosition()
    {
        float x,y;
        SDL_GetMouseState(&x, &y);
        return {static_cast<int>(x), static_cast<int>(y)};
    }

    bool SDLWindow::HasWindowFocus()
    {
        SDL_WindowFlags flags = SDL_GetWindowFlags(mWindow);
        // We *don't* want to check mouse focus:
        // SDL_WINDOW_INPUT_FOCUS - input is going to the window
        // SDL_WINDOW_MOUSE_FOCUS - mouse is hovered over the window, regardless of window focus
        return (flags & SDL_WINDOW_INPUT_FOCUS) != 0;
    }
}

#endif
