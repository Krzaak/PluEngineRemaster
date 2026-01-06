//
// Created by Plutex on 31.12.2025.
//

#include "WindowsWindow.h"

#include "PluEngine/Application.h"
#include "PluEngine/Log.h"

#ifdef PLU_PLATFORM_WINDOWS

#include "glad/glad_wgl.h"

namespace Plu {

    WindowsWindow* window;

    LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
        switch (uMsg) {
        case WM_CLOSE:
            window->Close();
            return 0;
        case WM_SIZE:
            {
                if (!window) return 0;
                return DefWindowProc(hwnd, uMsg, wParam, lParam);
            }
        }
        return DefWindowProcW(hwnd, uMsg, wParam, lParam);
    }

    Plu::WindowsWindow::WindowsWindow(const WindowProperties& properties) : IWindow(properties)
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
    }

    void WindowsWindow::Minimize()
    {
    }

    void* WindowsWindow::GetWindowHandle()
    {
        return mHandle;
    }

    bool WindowsWindow::IsVSyncEnabled()
    {
    }

    void WindowsWindow::SetVSyncEnabled(bool enable)
    {
    }

    bool WindowsWindow::IsRunning()
    {
        return mIsRunning;
    }

    int WindowsWindow::GetHeight()
    {
    }

    int WindowsWindow::GetWidth()
    {
    }

    bool WindowsWindow::IsMaximized()
    {
    }

    bool WindowsWindow::IsMinimized()
    {
    }

    void WindowsWindow::Init() {

        WNDCLASSW wc = {};
        wc.lpfnWndProc = WindowProc;
        wc.hInstance = GetModuleHandle(nullptr);
        wc.lpszClassName = L"EngineWindowClass";

        if (!RegisterClassW(&wc))
        {
            PLU_CORE_ASSERT(false, "RegisterClass failed")
            return;
        }
        DWORD dwStyle = WS_OVERLAPPEDWINDOW;
        mHandle = CreateWindowExW(
            WS_EX_APPWINDOW,
            wc.lpszClassName,
            std::wstring(mProperties.Title.Begin(), mProperties.Title.End()).c_str(),
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
        PLU_CORE_WARN("Window Initialized");
    }

    void WindowsWindow::OnUpdate(float deltaTime)
    {
        MSG msg = {};
        while (PeekMessage(&msg, mHandle, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        SwapBuffers(mHDC);
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
