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
        TUsePointer<SDLWindow> windowUser = objectManager->CreateObject(SDLWindow::GetStaticClass());
        TOwningPointer<SDLWindow> window = objectManager->GetObjectAsOwner<SDLWindow>(windowUser->GetObjectHandle());
        window->SetWindowProperties(properties);
        window->mApplicationInfo = applicationInfo;
        return window;
#elif defined(PLU_PLATFORM_WINDOWS)
        TUsePointer<WindowsWindow> windowUser = objectManager->CreateObject(WindowsWindow::GetStaticClass());
        TOwningPointer<WindowsWindow> window = objectManager->GetObjectAsOwner<WindowsWindow>(windowUser->GetObjectHandle());
        window->SetWindowProperties(properties);
        window->mApplicationInfo = applicationInfo;
        return window;
#endif
        return nullptr;
    }

    void IWindow::SetCursorVisibility(bool visible)
    {
#ifdef PLU_PLATFORM_LINUX
        SDLWindow::SetCursorVisibility(visible);
#elif defined(PLU_PLATFORM_WINDOWS)
        WindowsWindow::SetCursorVisibility(visible);
#endif
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
        SDL_GLContext glContext = this->GetGLContext();
        ImGui_ImplSDL2_InitForOpenGL(windowHandle, glContext);
        ImGui_ImplOpenGL3_Init("#version 450");
        PLU_CORE_WARN("SDL2 and OpenGL ImGui");
#elif defined(PLU_PLATFORM_WINDOWS)
        HWND windowHandle = static_cast<HWND>(this->GetWindowHandle());
        ImGui_ImplWin32_Init(windowHandle);
        ImGui_ImplOpenGL3_Init("#version 450");
        PLU_CORE_WARN("Windows and OpenGL ImGui");
#endif

        ImGuiStyle& style = ImGui::GetStyle();
#ifdef PLU_PLATFORM_WINDOWS
	    float mainScale = ImGui_ImplWin32_GetDpiScaleForMonitor(::MonitorFromPoint(POINT{0,0},MONITOR_DEFAULTTOPRIMARY));
	    style.FontScaleDpi = mainScale;
	    style.ScaleAllSizes(mainScale);
	    //ImGui_ImplWin32_EnableDpiAwareness();
#endif

        //style.ScaleAllSizes(mainScale);
        style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0,0,0,0);
        style.WindowRounding = 12.0f;
        style.ChildRounding = 12.0f;
        style.FrameRounding = 8.0f;
        style.PopupRounding = 10.0f;
        style.ScrollbarRounding = 12.0f;
        style.GrabRounding = 6.0f;
        style.TabRounding = 8.0f;

        style.WindowBorderSize = 0.0f;
        style.FrameBorderSize = 0.0f;
        style.PopupBorderSize = 0.0f;

        style.WindowPadding = ImVec2(12, 12);
        style.FramePadding = ImVec2(8, 6);

        ImVec4* colors = style.Colors;

        // Szklane, lekko mleczne tła
        colors[ImGuiCol_WindowBg]           = ImVec4(0.12f, 0.12f, 0.12f, 0.60f);
        colors[ImGuiCol_ChildBg]            = ImVec4(0.12f, 0.12f, 0.12f, 0.40f);
        colors[ImGuiCol_PopupBg]            = ImVec4(0.10f, 0.10f, 0.10f, 0.70f);

        // Kolory kontrolne
        colors[ImGuiCol_FrameBg]            = ImVec4(0.20f, 0.20f, 0.20f, 0.30f);
        colors[ImGuiCol_FrameBgHovered]     = ImVec4(0.25f, 0.25f, 0.25f, 0.55f);
        colors[ImGuiCol_FrameBgActive]      = ImVec4(0.30f, 0.30f, 0.30f, 0.75f);

        colors[ImGuiCol_Button]             = ImVec4(0.20f, 0.20f, 0.20f, 0.35f);
        colors[ImGuiCol_ButtonHovered]      = ImVec4(0.30f, 0.30f, 0.30f, 0.65f);
        colors[ImGuiCol_ButtonActive]       = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);

        // Taby / DockSpace
        colors[ImGuiCol_Tab]                = ImVec4(0.20f, 0.20f, 0.20f, 0.60f);
        colors[ImGuiCol_TabHovered]         = ImVec4(0.45f, 0.45f, 0.45f, 0.80f);
        colors[ImGuiCol_TabActive]          = ImVec4(0.35f, 0.35f, 0.35f, 0.85f);

        // Tekst
        colors[ImGuiCol_Text]               = ImVec4(1, 1, 1, 1);
        colors[ImGuiCol_TextDisabled]       = ImVec4(1, 1, 1, 0.40f);

        colors[ImGuiCol_TabDimmedSelected] = ImVec4(0.35f, 0.35f, 0.35f, 1.0f);
        colors[ImGuiCol_TabDimmed] = ImVec4(0.1f, 0.1f, 0.1f, 1.0f);
        colors[ImGuiCol_Header] = ImVec4(0.20f, 0.20f, 0.20f, 1.0f);
        colors[ImGuiCol_HeaderHovered] = ImVec4(0.25f, 0.25f, 0.25f, 1.0f);
        colors[ImGuiCol_HeaderActive] = ImVec4(0.4f, 0.4f, 0.4f, 1.0f);
        colors[ImGuiCol_TitleBg] = ImVec4(0.08f, 0.08f, 0.08f, 1.0f);
        colors[ImGuiCol_TitleBgActive] = ImVec4(0.08f, 0.08f, 0.08f, 1.0f);
        colors[ImGuiCol_SliderGrab] = ImVec4(0.4f, 0.4f, 0.4f, 1.0f);
        colors[ImGuiCol_SliderGrabActive] = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
    }
}
