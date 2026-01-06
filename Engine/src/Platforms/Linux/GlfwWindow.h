#ifndef PLUENGINE_GLFWWINDOW_H
#define PLUENGINE_GLFWWINDOW_H

#include "PluEngine/Core.h"

#ifdef PLU_PLATFORM_LINUX
#include <GLFW/glfw3.h>
#include <PluSTL_FWD.h>
#include <memory>

#include "PluEngine/Window/Window.h"

namespace Plu
{
    class GLFWWindow : public IWindow  // DODAJ: public (było bez!)
    {
        GLFWwindow* mGLFWWindow;
        bool mIsVSyncEnabled;

        // Statyczne zarządzanie GLFW
        static int sWindowCount;
        static bool sGLFWInitialized;

    public:
        GLFWWindow(const WindowProperties& properties);
        virtual ~GLFWWindow();

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

        bool IsVSyncEnabled() override;
        void SetVSyncEnabled(bool enable) override;

        void *GetWindowHandle() override;
        void *GetGLContext() override;

        // Callbacki dla debugowania
        static void GLFWErrorCallback(int error, const char* description);
    };
}
#endif

#endif //PLUENGINE_GLFWWINDOW_H