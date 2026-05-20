//
// Created by Plutex on 12/30/25.
//

#ifndef PLUENGINE_APPLICATION_H
#define PLUENGINE_APPLICATION_H
#include <PluSTL_FWD.h>
#include "PluEngine/Core.h"

namespace Plu
{
    class EngineAssetManager;
    class WindowsManager;
    class InputManager;
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
        TUsePointer<EngineAssetManager> AppAssetManager;
        TUsePointer<RenderingManager> AppRenderingManager;
        TUsePointer<InputManager> AppInputManager;
        TUsePointer<WindowsManager> AppWindowsManager;

        TUsePointer<GameClient> Client;
    };

    class PLU_API Application
    {
    protected:
        TOwningPointer<EngineObjectManager> mObjectManager;
        ApplicationInfo mApplicationInfo;
        bool mUpdateInput = true;
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
        virtual void OnImGuiRenderEX(UInt64 windowID) {}
        virtual void OnTick(float deltaTime) = 0;

        TUsePointer<EngineObjectManager> GetAppObjectManager();
        TUsePointer<IWindow> GetAppWindow();
        ApplicationInfo* GetAppInfo();
    protected:
        void StartGame();
        void EndGame();

        void EngineInit();
        void EngineShutdown();
    };

    TUsePointer<GameClient> GetGameClient();
}

#endif //PLUENGINE_APPLICATION_H