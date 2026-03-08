//
// Created by Plutex on 31.12.2025.
//

#include "WindowsWindow.h"

#include "PluEngine/Application.h"
#include "PluEngine/Log.h"

#ifdef PLU_PLATFORM_WINDOWS


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
        if (LRESULT imgui = ImGui_ImplWin32_WndProcHandlerEx(hwnd, uMsg, wParam, lParam, window->GetImGuiContext()->IO))
        {
            return imgui;
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
                POINT pt;
                GetCursorPos(&pt);
                ScreenToClient(hwnd, &pt);
                //PLU_CORE_INFO("Cursor: {} {}", pt.x, pt.y);

                int windowWidth = window->GetWidth();
                int windowHeight = window->GetHeight();

                if (pt.y >= 5 && pt.y <= 30) {
                    return HTCAPTION;
                }
                if (pt.y >= 0 && pt.y <= 5)
                {
                    return HTTOP;
                }
                if (pt.x >= 0 && pt.x <= 5)
                {
                    return HTLEFT;
                }
                if (pt.y >= windowHeight - 5 && pt.y <= windowHeight)
                {
                    return HTBOTTOM;
                }
                if (pt.x >= windowWidth - 5 && pt.x <= windowWidth)
                {
                    return HTRIGHT;
                }



                return DefWindowProc(hwnd, uMsg, wParam, lParam);
            }
        }
        return DefWindowProcW(hwnd, uMsg, wParam, lParam);
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
        AllocConsole();

        FILE* fp;
        freopen_s(&fp, "CONOUT$", "w", stdout);
        freopen_s(&fp, "CONOUT$", "w", stderr);
        freopen_s(&fp, "CONIN$", "r", stdin);

        mConsoleWindow = GetConsoleWindow();
        SetWindowTextW(mConsoleWindow, L"Editor Console");
        SetConsoleOutputCP(CP_UTF8);
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


    bool WindowsWindow::IsMaximized()
    {
        return IsZoomed(static_cast<HWND>(mHandle));
    }

    bool WindowsWindow::IsMinimized()
    {
        return IsIconic(static_cast<HWND>(mHandle));
    }


    void WindowsWindow::Init() {

        WNDCLASSW wc = {};
        wc.lpfnWndProc = WindowProc;
        wc.hInstance = GetModuleHandle(nullptr);
        wc.lpszClassName = L"EngineWindowClass";

        if (!GetClassInfoW(GetModuleHandle(nullptr), wc.lpszClassName, &wc))
        {
            PLU_CORE_INFO("Registering Window Class!");
            if (!RegisterClassW(&wc))
            {
                PLU_CORE_ASSERT(false, "RegisterClass failed")
                return;
            }
        } else
        {
            PLU_CORE_INFO("Window class already registered!");
        }

        DWORD dwStyle = WS_POPUP;
        mHandle = CreateWindowExW(
            WS_EX_APPWINDOW,
            wc.lpszClassName,
            StringW::FromNarrow(mProperties.Title.CStr()).CStr(),
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
        UpdateWindow(mHandle);
        uintptr_t id = (uintptr_t)mHandle;
        PLU_CORE_INFO("New Window ID: {}", id);
        windows[id] = this;
        CreateImGuiContext();
        PLU_CORE_WARN("Window Initialized");
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
