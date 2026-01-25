//
// Created by Plutex on 12/30/25.
//

#ifndef PLUENGINE_GLFWWINDOW_H
#define PLUENGINE_GLFWWINDOW_H

#include "PluEngine/Core.h"

#ifdef PLU_PLATFORM_WINDOWS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <PluSTL_FWD.h>
#include "PluEngine/Renderer/Renderer.h"
#include "PluEngine/Objects/EngineObjectManager.h"
#include "PluEngine/Window/Window.h"
#include "WindowsWindow.generated.h"

namespace Plu
{
    PLU_CLASS()
    class WindowsWindow : public IWindow
    {
        REFLECTION_BODY_WINDOWSWINDOW()
    private:
        HWND mHandle;
        HDC mHDC;
        HGLRC mGLContext;
        HICON mhIcon;

        HWND mConsoleWindow;


        bool  mVSyncEnabled = true;
        bool mIsRunning;

        void DestroyOpenGL();
        HGLRC InitOpenGL(HWND hWnd);
        bool SetupPixelFormat(HDC hdc);
    public:
        explicit WindowsWindow();
        virtual ~WindowsWindow();

        void Init() override;
        void OnUpdate(float deltaTime) override;
        void Shutdown() override;

        void Close() override;
        bool IsRunning() override;

        int GetHeight() override;
        int GetWidth() override;

        bool IsMaximized() override;
        bool IsMinimized() override;
        void Maximize() override;
        void Minimize() override;
        void* GetWindowHandle() override;

        bool IsVSyncEnabled() override;
        void SetVSyncEnabled(bool enable) override;

        void* GetGLContext() override;
        void SpawnConsoleWindow();
    };
}
#endif

#endif //PLUENGINE_GLFWWINDOW_H