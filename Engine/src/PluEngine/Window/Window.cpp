//
// Created by Plutex on 12/30/25.
//

#include "PluEngine/Window/Window.h"

#ifdef PLU_PLATFORM_WINDOWS
#include "Platforms/Windows/WindowsWindow.h"
#elif defined(PLU_PLATFORM_LINUX)
#include "Platforms/Linux/SdlWindow.h"
#include "Platforms/Linux/GlfwWindow.h"
#endif
#include "PluEngine/Core.h"

namespace Plu
{
    IWindow::IWindow(const WindowProperties &properties)
    {
        mProperties = properties;
    }

    TOwningPointer<IWindow> IWindow::PlutexCreateWindow(const WindowProperties& properties)
    {
#ifdef PLU_PLATFORM_LINUX
        return Plu::CreateOwning<SDLWindow>(properties);
#elif defined(PLU_PLATFORM_WINDOWS)
        return Plu::CreateOwning<WindowsWindow>(properties);
#endif
        return nullptr;
    }
}
