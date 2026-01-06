//
// Created by Plutex on 12/30/25.
//

#ifndef PLUENGINE_WINDOW_H
#define PLUENGINE_WINDOW_H

#include <PluSTL_FWD.h>
#include "PluEngine/Core.h"

namespace Plu
{
    struct WindowProperties
    {
        Plu::String Title;
        int Width;
        int Height;

        WindowProperties() : Title("New Window"), Width(1000), Height(720) {}
    };

    class PLU_API IWindow
    {
    protected:
        WindowProperties mProperties;
    public:
        explicit IWindow(const WindowProperties& properties);
        virtual ~IWindow() = default;
        virtual void Init() = 0;
        virtual void OnUpdate(float deltaTime) = 0;
        virtual void Shutdown() = 0;

        virtual bool IsRunning() = 0;
        virtual void Close() = 0;

        virtual int GetWidth() = 0;
        virtual int GetHeight() = 0;

        virtual void Minimize() = 0;
        virtual void Maximize() = 0;
        virtual bool IsMinimized() = 0;
        virtual bool IsMaximized() = 0;

        virtual bool IsVSyncEnabled() = 0;
        virtual void SetVSyncEnabled(bool enabled) = 0;

        virtual void* GetWindowHandle() = 0;
        virtual void* GetGLContext() = 0;

        static Plu::TOwningPointer<IWindow> PlutexCreateWindow(const WindowProperties& properties);
    };
}

#endif //PLUENGINE_WINDOW_H