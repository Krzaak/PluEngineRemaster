//
// Created by Plutex on 12/30/25.
//

#include "PluEngine/Application.h"

#include "PluEngine/Engine.h"
#include "PluEngine/Log.h"
#include "PluEngine/Renderer/Renderer.h"
#include "PluEngine/Window/Window.h"
#include "PluEngine/Objects/EngineObjectManager.h"

extern void InitEngineReflection();

namespace Plu
{
    Application::Application()
    {
        EngineInit();
    }

    Application::~Application()
    {
        EngineShutdown();
    }

    void Application::Run()
    {
        OnInit();
        if (!mWindow) {
            OnShutdown();
            PLU_CORE_CRITICAL("Application has no Active Window!");
            return;
        }
        if (!mRenderer) {
            OnShutdown();
            PLU_CORE_CRITICAL("Application has no Active Renderer!");
            return;
        }

        mApplicationInfo.AppObjectManager = mObjectManager;
        mApplicationInfo.AppWindow = mWindow;
        mApplicationInfo.AppRenderer = mRenderer;

        mWindow->Init();
        mRenderer->Init(mWindow);

        OnPostInit();

        PLU_CORE_TRACE("Initialized Successfully!");

        while (mWindow->IsRunning()) {
            mRenderer->OnUpdate(0);
            mWindow->OnUpdate(0);
        }
        OnShutdown();
    }

    void Application::Close()
    {
    }

    TUsePointer<EngineObjectManager> Application::GetAppObjectManager()
    {
        return mObjectManager;
    }

    TUsePointer<IWindow> Application::GetAppWindow()
    {
        return mWindow;
    }

    TUsePointer<IScenesManager> Application::GetAppSceneManager() const
    {
        return mApplicationInfo.AppScenesManager;
    }

    void Application::EngineInit()
    {
        Plu::Log::Init();
        InitEngineReflection();
        Engine::CreateEngine();
        PLU_CORE_INFO("Engine Init");
        mObjectManager = Plu::CreateOwning<EngineObjectManager>();
    }

    void Application::EngineShutdown()
    {
        mRenderer->OnShutdown();
        Engine::DestroyEngine();
        PLU_CORE_WARN("Engine Shutdown");
        mObjectManager = nullptr;
        PLU_CORE_INFO("Engine has successfully shut down and only error you'll get is from SDL2 Bug or my shitty Panel Stuff");
        mWindow->Shutdown();
    }
}
