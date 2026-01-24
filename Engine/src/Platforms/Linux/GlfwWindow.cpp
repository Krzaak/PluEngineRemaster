#include "GlfwWindow.h"
#include <iostream>
#include <spdlog/fmt/bundled/base.h>
#include "imgui.h"
#include "PluEngine/Core.h"
#include "PluEngine/Log.h"

#ifdef PLU_PLATFORM_LINUX

using namespace Plu;

// Inicjalizacja statycznych zmiennych
int GLFWWindow::sWindowCount = 0;
bool GLFWWindow::sGLFWInitialized = false;

GLFWWindow::GLFWWindow()
{
    mIsVSyncEnabled = false;
    mGLFWWindow = nullptr;
}

GLFWWindow::~GLFWWindow()
{
    PLU_CORE_INFO("GLFWWindow destruktor");
    
    // Zniszcz okno
    if (mGLFWWindow)
    {
        glfwDestroyWindow(mGLFWWindow);
        mGLFWWindow = nullptr;
        PLU_CORE_INFO("Okno GLFW zniszczone");
        
        // Zmniejsz licznik okien
        sWindowCount--;
        
        // Jeśli to ostatnie okno, zakończ GLFW
        if (sWindowCount <= 0 && sGLFWInitialized)
        {
            glfwTerminate();
            sGLFWInitialized = false;
            PLU_CORE_INFO("GLFW terminated");
        }
    }
}

void GLFWWindow::GLFWErrorCallback(int error, const char* description)
{
    PLU_CORE_ERROR("GLFW Error ({0}): {1}", error, description);
}

void GLFWWindow::Init()
{
    PLU_CORE_INFO("GLFW Window Init");
    
    // Inicjalizuj GLFW tylko raz
    if (!sGLFWInitialized)
    {
        if (!glfwInit())
        {
            PLU_CORE_ERROR("Failed to initialize GLFW");
            return;
        }
        
        // Ustaw callback błędów
        glfwSetErrorCallback(GLFWErrorCallback);
        
        sGLFWInitialized = true;
        PLU_CORE_INFO("GLFW initialized");
    }
    
    // Konfiguracja okna GLFW
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
    // Dla debugowania - możesz to usunąć w release
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
    //glfwInitHint(GLFW_CURSOR_DISABLED, GLFW_TRUE);

    
    mGLFWWindow = glfwCreateWindow(mProperties.Width, mProperties.Height, "Ohio Window", nullptr, nullptr);
    if (!mGLFWWindow) 
    {
        PLU_CORE_ERROR("Failed to create GLFW window");
        return;
    }
    
    glfwMakeContextCurrent(mGLFWWindow);
    SetVSyncEnabled(false);
    glfwSetWindowAttrib(mGLFWWindow, GLFW_DECORATED, GLFW_FALSE);
    
    // Inkrementuj licznik okien
    sWindowCount++;
    
    PLU_CORE_INFO("GLFW window created successfully. Total windows: {0}", sWindowCount);
}

void GLFWWindow::OnUpdate(float deltaTime)
{
    // Debug: monitoruj zużycie pamięci co 60 klatek
    static int frameCount = 0;
    frameCount++;
    
    if (frameCount % 60 == 0 && mGLFWWindow)
    {
        // Sprawdź, czy okno nie jest uszkodzone
        if (glfwWindowShouldClose(mGLFWWindow))
        {
            PLU_CORE_WARN("Window should close flag set");
        }
    }
    
    if (mGLFWWindow)
    {
        glfwSwapBuffers(mGLFWWindow);
        glfwPollEvents();
    }
}

void GLFWWindow::Shutdown()
{
    PLU_CORE_INFO("GLFWWindow Shutdown");
    
    // Zniszcz okno (to samo co w destruktorze)
    if (mGLFWWindow)
    {
        glfwDestroyWindow(mGLFWWindow);
        mGLFWWindow = nullptr;
        sWindowCount--;
        
        if (sWindowCount <= 0 && sGLFWInitialized)
        {
            glfwTerminate();
            sGLFWInitialized = false;
            PLU_CORE_INFO("GLFW terminated from Shutdown()");
        }
    }
}

void GLFWWindow::Close()
{
    if (mGLFWWindow)
    {
        glfwSetWindowShouldClose(mGLFWWindow, GLFW_TRUE);
        PLU_CORE_INFO("Window close requested");
    }
}

bool GLFWWindow::IsRunning()
{
    if (!mGLFWWindow) return false;
    return !glfwWindowShouldClose(mGLFWWindow);
}

int GLFWWindow::GetHeight()
{
    int height = 0;
    if (mGLFWWindow) {
        glfwGetWindowSize(mGLFWWindow, nullptr, &height);
    }
    return height;
}

int GLFWWindow::GetWidth()
{
    int width = 0;
    if (mGLFWWindow) {
        glfwGetWindowSize(mGLFWWindow, &width, nullptr);
    }
    return width;
}

bool GLFWWindow::IsMaximized()
{
    if (!mGLFWWindow) return false;
    return glfwGetWindowAttrib(mGLFWWindow, GLFW_MAXIMIZED);
}

bool GLFWWindow::IsMinimized()
{
    if (!mGLFWWindow) return false;
    return glfwGetWindowAttrib(mGLFWWindow, GLFW_ICONIFIED);
}

void GLFWWindow::Maximize()
{
    if (mGLFWWindow) {
        glfwMaximizeWindow(mGLFWWindow);
    }
}

void GLFWWindow::Minimize()
{
    if (mGLFWWindow) {
        glfwIconifyWindow(mGLFWWindow);
    }
}

bool GLFWWindow::IsVSyncEnabled()
{
    return mIsVSyncEnabled;
}

void GLFWWindow::SetVSyncEnabled(bool enable)
{
    if (mGLFWWindow) {
        glfwMakeContextCurrent(mGLFWWindow);
        glfwSwapInterval(enable ? 1 : 0);
        mIsVSyncEnabled = enable;
        PLU_CORE_INFO("VSync {0}", enable ? "enabled" : "disabled");
    }
}

void* GLFWWindow::GetWindowHandle()
{
    return mGLFWWindow;
}

void * GLFWWindow::GetGLContext()
{
    return nullptr;
}

#endif
