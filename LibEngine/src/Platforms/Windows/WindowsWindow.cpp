//
// Created by Plutex on 31.12.2025.
//

#include "WindowsWindow.h"

#include "PluEngine/Application.h"
#include "PluEngine/Log.h"

#ifdef PLU_PLATFORM_WINDOWS


#include <dwmapi.h>
#include <windowsx.h>

#include "imgui.h"
#include "imgui_internal.h"
#include "glad/glad_wgl.h"
#include "PluEngine/Input/InputManager.h"
#include "PluEngine/Input/WinAPIInputBackend.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandlerEx(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, ImGuiIO& io);

namespace Plu {

    GameHashMap<uintptr_t, WindowsWindow*> windows;

    LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
        WindowsWindow** windowFind = windows.Find((uintptr_t)hwnd);
        if (!windowFind)
        {
            PLU_CORE_ERROR("Invalid window handle");
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
        }
        WindowsWindow* window = *windowFind;
        if (window->UpdateImGui)
        {
            if (LRESULT imgui = ImGui_ImplWin32_WndProcHandlerEx(hwnd, uMsg, wParam, lParam, window->GetImGuiContext()->IO))
            {
                return imgui;
            }
        }
        dynamic_cast<WinAPIInputBackend*>(window->mApplicationInfo->AppInputManager->GetInputBackend().GetRaw())->FeedMessage(uMsg, wParam, lParam);
        switch (uMsg) {
        case WM_CLOSE:
            window->Close();
            return 0;
        case WM_SIZE:
            {
                if (!window) return 0;
                return DefWindowProc(hwnd, uMsg, wParam, lParam);
            }
        case WM_NCHITTEST:
            {
                if (window->ImGuiItemHovered)
                {
                    return HTCLIENT;
                }
                POINT cursor = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
                ScreenToClient(hwnd, &cursor);

                RECT rc;
                GetClientRect(hwnd, &rc);

                const int borderSize = 8; // strefa resize przy krawędziach

                // Resize zones
                if (cursor.y < borderSize) {
                    if (cursor.x < borderSize)       return HTTOPLEFT;
                    if (cursor.x > rc.right - borderSize) return HTTOPRIGHT;
                    return HTTOP;
                }
                if (cursor.y > rc.bottom - borderSize) {
                    if (cursor.x < borderSize)       return HTBOTTOMLEFT;
                    if (cursor.x > rc.right - borderSize) return HTBOTTOMRIGHT;
                    return HTBOTTOM;
                }
                if (cursor.x < borderSize)  return HTLEFT;
                if (cursor.x > rc.right - borderSize) return HTRIGHT;

                // Titlebar zone (np. 32px wysokości)
                const int titlebarHeight = 32;
                if (cursor.y < titlebarHeight)
                    return HTCAPTION; // Windows obsłuży przeciąganie i double-click maximize

                return HTCLIENT;
            }
        case WM_CREATE:
            SetWindowPos(hwnd, nullptr, 0, 0, 0, 0,
                SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
            return 0;

        case WM_NCPAINT:
            return 0;

        case WM_NCCALCSIZE:
            if (wParam == TRUE)
            {
                if (IsMaximized(hwnd))
                {
                    MONITORINFO mi = { sizeof(mi) };
                    GetMonitorInfo(MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST), &mi);
                    reinterpret_cast<NCCALCSIZE_PARAMS*>(lParam)->rgrc[0] = mi.rcWork;
                }
                return 0;
            }
            break;

        case WM_ACTIVATE:
            SetWindowPos(hwnd, nullptr, 0, 0, 0, 0,
                SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
            return 0;

        case WM_NCACTIVATE:
            return DefWindowProc(hwnd, uMsg, wParam, -1);

        case WM_GETMINMAXINFO:
            {
                // Bez tego WS_POPUP maksymalizuje się na cały ekran (pod pasek zadań)
                LPMINMAXINFO lpMMI = reinterpret_cast<LPMINMAXINFO>(lParam);
                HMONITOR hMonitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
                MONITORINFO mi = { sizeof(mi) };
                GetMonitorInfo(hMonitor, &mi);

                // Pozycja i rozmiar zmaksymalizowanego okna = obszar roboczy monitora
                lpMMI->ptMaxPosition.x = mi.rcWork.left - mi.rcMonitor.left;
                lpMMI->ptMaxPosition.y = mi.rcWork.top  - mi.rcMonitor.top;
                lpMMI->ptMaxSize.x     = mi.rcWork.right  - mi.rcWork.left;
                lpMMI->ptMaxSize.y     = mi.rcWork.bottom - mi.rcWork.top;
                return 0;
            }


        }
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }

    Plu::WindowsWindow::WindowsWindow()
    {
        mIsRunning = false;
        mHandle = nullptr;
        mGLContext = nullptr;
        mHDC = nullptr;
        mhIcon = nullptr;
    }

    WindowsWindow::~WindowsWindow() {
    }

    void WindowsWindow::Close()
    {
        PostQuitMessage(0);
        mIsRunning = false;
    }

    void WindowsWindow::Maximize()
    {
        ShowWindow(static_cast<HWND>(mHandle), SW_MAXIMIZE);
    }

    void WindowsWindow::Minimize()
    {
        ShowWindow(static_cast<HWND>(mHandle), SW_MINIMIZE);
    }


    void* WindowsWindow::GetWindowHandle()
    {
        return mHandle;
    }

    bool WindowsWindow::IsVSyncEnabled()
    {
        return mVSyncEnabled;
    }

    void WindowsWindow::SetVSyncEnabled(bool enable)
    {
        mVSyncEnabled = enable;

        typedef BOOL(WINAPI* PFNWGLSWAPINTERVALEXTPROC)(int);
        static PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT =
            (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");

        if (wglSwapIntervalEXT)
            wglSwapIntervalEXT(enable ? 1 : 0);
    }

    void WindowsWindow::SetWindowTitle(String title)
    {
        SetWindowTextA(mHandle, title.CStr());
    }


    void* WindowsWindow::GetGLContext()
    {
        return static_cast<void*>(mGLContext);
    }

    void WindowsWindow::SpawnConsoleWindow()
    {
        if (!AllocConsole())
            return; // konsola już istnieje lub błąd

        FILE* fp_out, * fp_err, * fp_in;
        freopen_s(&fp_out, "CONOUT$", "w", stdout);
        freopen_s(&fp_err, "CONOUT$", "w", stderr);
        freopen_s(&fp_in, "CONIN$", "r", stdin);

        SetConsoleOutputCP(CP_UTF8);
        SetConsoleCP(CP_UTF8);
        std::ios::sync_with_stdio(true);

        mConsoleWindow = GetConsoleWindow();
        if (mConsoleWindow)
            SetWindowTextW(mConsoleWindow, L"Editor Console");

        auto stdout_sink = std::make_shared<spdlog::sinks::wincolor_stdout_sink_mt>();
        stdout_sink->set_level(spdlog::level::trace);
      
        auto& logger = Log::GetCoreLogger();
        if (logger)
            logger->sinks().push_back(stdout_sink);

        logger = Log::GetClientLogger();

        if (logger)
            logger->sinks().push_back(stdout_sink);

        PLU_CORE_INFO("Console Allocated");
    }

    void WindowsWindow::MakeGLContextCurrent()
    {
        wglMakeCurrent(mHDC, mGLContext);
    }

    void WindowsWindow::SwapBuffer()
    {
        SwapBuffers(mHDC);
    }

    void WindowsWindow::SetCursorVisibility(bool visible)
    {
        ShowCursor(visible);
    }

    IVec2 WindowsWindow::GetCursorPosition()
    {
        POINT p;
        GetCursorPos(&p);
        return IVec2(p.x, p.y);
    }

    void WindowsWindow::SetCursorPosition(IVec2 pos)
    {
        SetCursorPos(pos.x, pos.y);
    }

    bool WindowsWindow::IsRunning()
    {
        return mIsRunning;
    }

    int WindowsWindow::GetWidth()
    {
        RECT rect;
        GetClientRect(static_cast<HWND>(mHandle), &rect);
        return rect.right - rect.left;
    }

    int WindowsWindow::GetHeight()
    {
        RECT rect;
        GetClientRect(static_cast<HWND>(mHandle), &rect);
        return rect.bottom - rect.top;
    }


    bool WindowsWindow::IsWindowMaximized()
    {
        return IsZoomed(static_cast<HWND>(mHandle));
    }

    bool WindowsWindow::IsWindowMinimized()
    {
        return IsIconic(static_cast<HWND>(mHandle));
    }


    void WindowsWindow::Init() {

        WNDCLASS wc = {};
        wc.lpfnWndProc = WindowProc;
        wc.hInstance = GetModuleHandle(nullptr);
        wc.lpszClassName = "EngineWindowClass";

        if (!GetClassInfo(GetModuleHandle(nullptr), wc.lpszClassName, &wc))
        {
            PLU_CORE_INFO("Registering Window Class!");
            if (!RegisterClass(&wc))
            {
                PLU_CORE_ASSERT(false, "RegisterClass failed")
                return;
            }
        } else
        {
            PLU_CORE_INFO("Window class already registered!");
        }

        DWORD dwStyle = WS_OVERLAPPEDWINDOW;

        mHandle = CreateWindowEx(
            WS_EX_APPWINDOW,
            wc.lpszClassName,
            mProperties.Title.CStr(),
            dwStyle,
            CW_USEDEFAULT, CW_USEDEFAULT,
            mProperties.Width, mProperties.Height,
            nullptr, nullptr, GetModuleHandle(nullptr), nullptr
        );

        PLU_CORE_ASSERT(mHandle, "Invalid Handle")

        mHDC = GetDC(mHandle);
        if (!InitOpenGL(mHandle))
        {
            PLU_CORE_CRITICAL("Invalid OpenGL Context!");
            return;
        }
        wglMakeCurrent(mHDC, mGLContext);
        mIsRunning = true;
        ShowWindow(mHandle, SW_SHOW);
        uintptr_t id = (uintptr_t)mHandle;
        PLU_CORE_INFO("New Window ID: {}", id);
        windows[id] = this;
        CreateImGuiContext();
        PLU_CORE_WARN("Windows Window Initialized");
        SetWindowPos(mHandle, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
    }

    void WindowsWindow::OnUpdate(float deltaTime)
    {
        MSG msg = {};
        while (PeekMessage(&msg, mHandle, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    void WindowsWindow::Shutdown() {
        DestroyOpenGL();
        DestroyWindow(mHandle);
    }

    void WindowsWindow::DestroyOpenGL()
    {
        if (mGLContext)
        {
            wglMakeCurrent(nullptr, nullptr);
            wglDeleteContext(mGLContext);
            mGLContext = nullptr;
        }
    }

    HGLRC WindowsWindow::InitOpenGL(HWND hWnd)
    {
        HDC hDC = GetDC(hWnd);

        // --- KROK 1: Tymczasowy kontekst (Legacy) ---
        if (!SetupPixelFormat(hDC)) return nullptr;

        HGLRC tempContext = wglCreateContext(hDC);
        wglMakeCurrent(hDC, tempContext);

        // --- KROK 2: Inicjalizacja GLAD i WGL ---
        if (!gladLoadGL() || !gladLoadWGL(hDC)) {
            // Błąd inicjalizacji
            wglMakeCurrent(NULL, NULL);
            wglDeleteContext(tempContext);
            return nullptr;
        }

        // --- KROK 3: Tworzenie nowoczesnego kontekstu 4.6 Core ---
        // Atrybuty dla OpenGL 4.6 Core Profile
        int attribs[] = {
            WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
            WGL_CONTEXT_MINOR_VERSION_ARB, 6,
            WGL_CONTEXT_PROFILE_MASK_ARB,  WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
    #ifdef _DEBUG
            WGL_CONTEXT_FLAGS_ARB,         WGL_CONTEXT_DEBUG_BIT_ARB,
    #endif
            0
        };

        HGLRC modernContext = nullptr;
        if (wglCreateContextAttribsARB) {
            modernContext = wglCreateContextAttribsARB(hDC, 0, attribs);
        }

        // --- KROK 4: Sprzątanie ---
        wglMakeCurrent(NULL, NULL);
        wglDeleteContext(tempContext); // Usuwamy stary, mamy nowy!

        if (modernContext) {
            wglMakeCurrent(hDC, modernContext);
        }

        mGLContext = modernContext;
        return modernContext;
    }

    bool WindowsWindow::SetupPixelFormat(HDC hdc)
    {
        PIXELFORMATDESCRIPTOR pfd = {
            sizeof(PIXELFORMATDESCRIPTOR),
            1,
            PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
            PFD_TYPE_RGBA,
            32, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            24, // 24-bitowy bufor głębi (depth buffer)
            8,  // 8-bitowy bufor szablonu (stencil buffer)
            0, PFD_MAIN_PLANE, 0, 0, 0, 0
        };

        int pixelFormat = ChoosePixelFormat(hdc, &pfd);
        if (pixelFormat == 0) return false;

        return SetPixelFormat(hdc, pixelFormat, &pfd);
    }
} // namespace Engine

#endif
