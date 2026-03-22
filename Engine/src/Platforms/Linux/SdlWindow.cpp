//
// Created by Plutex on 1/5/26.
//

#include "SdlWindow.h"

#include "backends/imgui_impl_sdl2.h"
#include "PluEngine/Application.h"
#include "PluEngine/Input/InputManager.h"
#include "PluEngine/Input/SDLInputBackend.h"

#ifdef PLU_PLATFORM_LINUX

namespace Plu
{
	#include "SdlWindow.h"
    #include <GL/gl.h>

    void SDLWindow::InitSDL()
    {
        SDL_Init(SDL_INIT_VIDEO);

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
            if (gSDLWindows.Contains(e.window.windowID)) gSDLWindows[e.window.windowID]->OnEventSDL(&e);
        }
    }

    void SDLWindow::SetCursorVisibility(bool visible)
    {
        SDL_ShowCursor(visible ? SDL_ENABLE : SDL_DISABLE);
        SDL_SetRelativeMouseMode(visible ? SDL_FALSE : SDL_TRUE);
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
        mWindow = SDL_CreateWindow(
            mProperties.Title.CStr(),
            SDL_WINDOWPOS_CENTERED,
            SDL_WINDOWPOS_CENTERED,
            mProperties.Width,
            mProperties.Height,
            SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_BORDERLESS
        );

        static SDL_GLContext glContext = nullptr;
        if (!glContext)
            glContext = SDL_GL_CreateContext(mWindow);
        mGLContext = glContext;
        SDL_GL_MakeCurrent(mWindow, mGLContext);
        CreateImGuiContext();

        SDL_SetWindowHitTest(mWindow, HitTestCallback, nullptr);

        SetVSyncEnabled(true);

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

        if (e->type == SDL_QUIT)
            mRunning = false;

        dynamic_cast<SDLInputBackend*>(mApplicationInfo->AppInputManager->GetInputBackend().GetRaw())->FeedEvent(*e);
        if (UpdateImGui) if (ImGui_ImplSDL2_ProcessEvent(e)) return;
        if (e->type == SDL_WINDOWEVENT) {
            switch (e->window.event) {
                case SDL_WINDOWEVENT_RESIZED:
                {
                    mProperties.Width  = e->window.data1;
                    mProperties.Height = e->window.data2;
                    break;
                }
                case SDL_WINDOWEVENT_FOCUS_LOST:
                {
                    break;
                }
                case SDL_WINDOWEVENT_FOCUS_GAINED:
                {
                    break;
                }
                default:
                {
                    break;
                }
            }
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
    }

    int SDLWindow::GetWindowID()
    {
        return mWindowID;
    }

    int SDLWindow::GetWidth()
    {
        int w,h;
        SDL_GetWindowSize(mWindow, &w, &h);
        return w;
    }

    int SDLWindow::GetHeight()
    {
        int w,h;
        SDL_GetWindowSize(mWindow, &w, &h);
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

    bool SDLWindow::IsMinimized()
    {
        return SDL_GetWindowFlags(mWindow) & SDL_WINDOW_MINIMIZED;
    }

    bool SDLWindow::IsMaximized()
    {
        return SDL_GetWindowFlags(mWindow) & SDL_WINDOW_MAXIMIZED;
    }

    bool SDLWindow::IsVSyncEnabled()
    {
        return mVSyncEnabled;
    }

    void SDLWindow::SetVSyncEnabled(bool enabled)
    {
        SDL_GL_SetSwapInterval(enabled ? 1 : 0);
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

    void SDLWindow::SwapBuffer()
    {
        SDL_GL_SwapWindow(mWindow);
    }

    void SDLWindow::SetWindowTitle(String title)
    {
        SDL_SetWindowTitle(mWindow, title.CStr());
    }
}

#endif
