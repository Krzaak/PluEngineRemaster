//
// Created by Plutex on 12/30/25.
//

#include "PluEngine/Window/Window.h"

#include "backends/imgui_impl_opengl3.h"
#include "backends/imgui_impl_sdl2.h"
#include "backends/imgui_impl_win32.h"
#include "PluEngine/Engine.h"

#ifdef PLU_PLATFORM_WINDOWS
#include "Platforms/Windows/WindowsWindow.h"
#elif defined(PLU_PLATFORM_LINUX)
#include "Platforms/Linux/SdlWindow.h"
#include "Platforms/Linux/GlfwWindow.h"
#endif
#include "PluEngine/Core.h"
#include "PluEngine/Objects/EngineObjectManager.h"

namespace Plu
{
    void IWindow::SetWindowProperties(const WindowProperties &properties)
    {
        mProperties = properties;
    }

    TOwningPointer<IWindow> IWindow::PlutexCreateWindow(const WindowProperties& properties, const TUsePointer<EngineObjectManager>& objectManager, ApplicationInfo *
                                                        applicationInfo)
    {
#ifdef PLU_PLATFORM_LINUX
        TOwningPointer<SDLWindow> window = objectManager->CreateObject(SDLWindow::GetStaticClass());
        window->SetWindowProperties(properties);
        window->mApplicationInfo = applicationInfo;
        return window;
#elif defined(PLU_PLATFORM_WINDOWS)
        TOwningPointer<WindowsWindow> window = objectManager->CreateObject(WindowsWindow::GetStaticClass());
        window->SetWindowProperties(properties);
        window->mApplicationInfo = applicationInfo;
        return window;
#endif
        return nullptr;
    }

    void IWindow::CreateImGuiContext()
    {
        ImGuiContext* ctx = ImGui::CreateContext();
        mImGuiContext = ctx;

        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        //io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

        ImGui::SetCurrentContext(mImGuiContext);

#ifdef PLU_PLATFORM_LINUX
        SDL_Window* windowHandle = static_cast<SDL_Window *>(this->GetWindowHandle());
        SDL_GLContext* glContext = static_cast<SDL_GLContext*>(this->GetGLContext());
        ImGui_ImplSDL2_InitForOpenGL(windowHandle, glContext);
        ImGui_ImplOpenGL3_Init("#version 450");
        PLU_CORE_WARN("SDL2 and OpenGL ImGui");
#elif defined(PLU_PLATFORM_WINDOWS)
        HWND windowHandle = static_cast<HWND>(this->GetWindowHandle());
        ImGui_ImplWin32_Init(windowHandle);
        ImGui_ImplOpenGL3_Init("#version 450");
        PLU_CORE_WARN("Windows and OpenGL ImGui");
#endif
    }
}
