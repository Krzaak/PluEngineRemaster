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
#include "PluEngine/Objects/EngineObjectManager.h"

namespace Plu
{
    void IWindow::SetWindowProperties(const WindowProperties &properties)
    {
        mProperties = properties;
    }

    TOwningPointer<IWindow> IWindow::PlutexCreateWindow(const WindowProperties& properties, const TUsePointer<EngineObjectManager>& objectManager)
    {
#ifdef PLU_PLATFORM_LINUX
        TOwningPointer<SDLWindow> window = objectManager->CreateObject(SDLWindow::GetStaticClass());
        window->SetWindowProperties(properties);
        return window;
#elif defined(PLU_PLATFORM_WINDOWS)
        return Plu::CreateOwning<WindowsWindow>(properties);
#endif
        return nullptr;
    }
}
