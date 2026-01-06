//
// Created by Plutex on 12/30/25.
//

#ifndef PLUENGINE_APPLICATION_H
#define PLUENGINE_APPLICATION_H
#include <PluSTL_FWD.h>
#include "PluEngine/Core.h"

namespace Plu
{
    class EngineObjectManager;
    class Renderer;
    class IWindow;

    struct ApplicationInfo
    {
        TOwningPointer<IWindow> AppWindow;
        TOwningPointer<Renderer> AppRenderer;
        TOwningPointer<EngineObjectManager> AppObjectManager;
    };

    class PLU_API Application
    {
    protected:
        TOwningPointer<IWindow> mWindow;
        TOwningPointer<Renderer> mRenderer;
        TOwningPointer<EngineObjectManager> mObjectManager;

        ApplicationInfo mApplicationInfo;
    public:
        Application();
        virtual ~Application();

        void Run();
        void Close();

        virtual void OnInit() = 0;
        virtual void OnPostInit() = 0;
        virtual void OnShutdown() = 0;
        //We expose the option to do something when ImGui is active
        virtual void OnImGuiRender() = 0;

        TUsePointer<EngineObjectManager> GetAppObjectManager();
        TUsePointer<IWindow> GetAppWindow();
    protected:
        void EngineInit();
        void EngineShutdown();
    };
}

#endif //PLUENGINE_APPLICATION_H