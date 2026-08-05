//
// Created by Plutex on 12/30/25.
//

#include "PluEngine/Window/Window.h"

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

    // Engine window ids are handed out here and never reused within a session; the first window
    // created (the main one) gets 0, which the rest of the engine treats as "the main window".
    static UInt32 gNextWindowID = 0;

    TOwningPointer<IWindow> IWindow::PlutexCreateWindow(const WindowProperties& properties, const TUsePointer<EngineObjectManager>& objectManager, ApplicationInfo *
                                                        applicationInfo)
    {
#ifdef PLU_PLATFORM_LINUX
        TUsePointer<SDLWindow> windowUser = objectManager->CreateObject(SDLWindow::GetStaticClass());
        TOwningPointer<SDLWindow> window = objectManager->GetObjectAsOwner<SDLWindow>(windowUser->GetObjectHandle());
#elif defined(PLU_PLATFORM_WINDOWS)
        TUsePointer<WindowsWindow> windowUser = objectManager->CreateObject(WindowsWindow::GetStaticClass());
        TOwningPointer<WindowsWindow> window = objectManager->GetObjectAsOwner<WindowsWindow>(windowUser->GetObjectHandle());
#else
        return nullptr;
#endif
        window->SetWindowProperties(properties);
        window->mApplicationInfo = applicationInfo;
        window->mWindowID = gNextWindowID++;
        return window;
    }
}
