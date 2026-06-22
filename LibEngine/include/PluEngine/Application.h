//
// Created by Plutex on 12/30/25.
//

#ifndef PLUENGINE_APPLICATION_H
#define PLUENGINE_APPLICATION_H
#include <PluSTL_FWD.h>
#include <argparse/argparse.hpp>

#include "PluEngine/Core.h"

namespace Plu
{
    struct DeserializationContext;
    class EngineAssetManager;
    class WindowsManager;
    class InputManager;
    class GameClient;
    class RenderingManager;
    class IAssetManager;
    class IShaderManager;
    class SceneManager;
    class EngineObjectManager;
    class Renderer;
    class IWindow;

    struct PLU_API ApplicationInfo
    {
        TUsePointer<IWindow> AppWindow;
        TUsePointer<EngineObjectManager> AppObjectManager;
        TUsePointer<SceneManager> AppScenesManager;
        TUsePointer<IShaderManager> AppShaderManager;
        TUsePointer<EngineAssetManager> AppAssetManager;
        TUsePointer<RenderingManager> AppRenderingManager;
        TUsePointer<InputManager> AppInputManager;
        TUsePointer<WindowsManager> AppWindowsManager;
        TUsePointer<class IPythonManager> AppPythonManager;

        TUsePointer<GameClient> Client;

        [[nodiscard]] DeserializationContext* ConstructDeserializationContext() const;
    };

    class PLU_API Application
    {
    protected:
        TOwningPointer<EngineObjectManager> mObjectManager;
        ApplicationInfo mApplicationInfo;
        bool mUpdateInput = true;

        argparse::ArgumentParser* mArgumentParser;
    public:
        Application();
        virtual ~Application();

        void InjectArguments(argparse::ArgumentParser* parser);

        void Run();
        void Close();

        virtual bool OnInit() = 0;
        virtual void OnPostInit() = 0;
        virtual void OnShutdown() = 0;
        //We expose the option to do something when ImGui is active
        virtual void OnImGuiRender() = 0;
        virtual void OnImGuiRenderEX(UInt64 windowID) {}
        virtual void OnTick(float deltaTime) = 0;

        virtual void OnRequestedExit() = 0;

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

    PLU_FUNCTION()
    void ExitGame();
}

#endif //PLUENGINE_APPLICATION_H