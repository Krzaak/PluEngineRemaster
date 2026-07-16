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
    class InputManager;
    class GameClient;
    class RenderingManager;
    class IAssetManager;
    class IShaderManager;
    class SceneManager;
    class EngineObjectManager;
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
        TUsePointer<class IPythonManager> AppPythonManager;

        TUsePointer<GameClient> Client;

        [[nodiscard]] DeserializationContext* ConstructDeserializationContext() const;
    };

    class PLU_API Application
    {
    protected:
        TOwningPointer<EngineObjectManager> mObjectManager;
        ApplicationInfo mApplicationInfo;

        argparse::ArgumentParser* mArgumentParser;
        friend class IWindow;
    public:
        Application();
        virtual ~Application();

        void InjectArguments(argparse::ArgumentParser* parser);

        void Run();
        void Close();

        virtual bool OnInit() = 0;
        virtual void OnPostInit() = 0;
        virtual void OnShutdown() = 0;
        // NOTE: the engine no longer drives an OnImGuiRender() callback. An app that wants a
        // UI builds its own ImGui frame (ImGui::NewFrame()/.../ImGui::Render()) wherever it
        // likes and hands the result to the render thread via
        // RenderingManager::SubmitImGuiDrawData(). Apps without UI (e.g. Runtime) skip it.
        virtual void OnTick(float deltaTime) = 0;

        virtual void OnRequestedGameExit() = 0;
        virtual void OnRequestedWindowClose(TUsePointer<IWindow> window) = 0;

        TUsePointer<EngineObjectManager> GetAppObjectManager();
        TUsePointer<IWindow> GetAppWindow();
        ApplicationInfo* GetAppInfo();
    protected:
        void StartGame();
        void EndGame();

        void DispatchWindowClose(TUsePointer<IWindow> window);

        void EngineInit();
        void EngineShutdown();
    };

    TUsePointer<GameClient> GetGameClient();

    PLU_FUNCTION()
    PLU_API void ExitGame();
}

#endif //PLUENGINE_APPLICATION_H