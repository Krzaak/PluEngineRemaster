//
// Created by Plutex on 12/30/25.
//

#ifndef PLUENGINE_APPLICATION_H
#define PLUENGINE_APPLICATION_H
#include <PluSTL_FWD.h>
#include "PluEngine/Core.h"

namespace Plu
{
    class GameClient;
    class RenderingManager;
    class IAssetManager;
    class IShaderManager;
    class IScenesManager;
    class EngineObjectManager;
    class Renderer;
    class IWindow;

    struct ApplicationInfo
    {
        TUsePointer<IWindow> AppWindow;
        TUsePointer<Renderer> AppRenderer;
        TUsePointer<EngineObjectManager> AppObjectManager;
        TUsePointer<IScenesManager> AppScenesManager;
        TUsePointer<IShaderManager> AppShaderManager;
        TUsePointer<IAssetManager> AppAssetManager;
        TUsePointer<RenderingManager> AppRenderingManager;

        TUsePointer<GameClient> Client;
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
        ApplicationInfo* GetAppInfo();
    protected:
        void StartGame();
        void EndGame();

        void EngineInit();
        void EngineShutdown();
    };
}

#endif //PLUENGINE_APPLICATION_H