//
// Created by Plutex on 12/30/25.
//

#include "PluEngine/Application.h"

#include "Platforms/Linux/SdlWindow.h"
#include "Platforms/Windows/WindowsWindow.h"
#include "PluEngine/Engine.h"
#include "PluEngine/Log.h"
#include "PluEngine/Timer.h"
#include "PluEngine/Managers/RenderingManager.h"
#include "PluEngine/Managers/ScenesManager.h"
#include "PluEngine/Renderer/Renderer.h"
#include "PluEngine/Window/Window.h"
#include "PluEngine/Objects/EngineObjectManager.h"
#include "PluEngine/GameCore/GameClient.h"
#include "PluEngine/Input/InputManager.h"

extern void InitEngineReflection();

namespace Plu
{
    Application::Application()
    {
        PLU_TIMER_START("EngineInit");
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

        mApplicationInfo.AppWindow = mWindow;
        mApplicationInfo.AppRenderer = mRenderer;

        mWindow->Init();
        mApplicationInfo.AppInputManager->GetInputBackend()->Init();
#ifdef PLU_PLATFORM_WINDOWS
        //DynamicCast<WindowsWindow>(mWindow)->SpawnConsoleWindow();
#endif
        mRenderer->Init(mWindow);

        OnPostInit();

        PLU_CORE_TRACE("Initialized Successfully!");
        PLU_TIMER_END("EngineInit");

        while (mWindow->IsRunning()) {
#ifdef PLU_PLATFORM_LINUX
            SDLWindow::HandleSDLEvents();
#endif
            mApplicationInfo.AppInputManager->GetInputBackend()->Update();
            OnTick(0);
            mApplicationInfo.AppScenesManager->TickScene(0);
            mApplicationInfo.AppRenderingManager->Tick(0);
            mRenderer->OnUpdate(0);
            mWindow->OnUpdate(0);
            mApplicationInfo.AppInputManager->GetInputBackend()->EndFrame();
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

    ApplicationInfo * Application::GetAppInfo()
    {
        return &mApplicationInfo;
    }

    void Application::StartGame()
    {
        EngineObjectHandle gameClientHandle = mObjectManager->CreateObject<GameClient>(mObjectManager, mApplicationInfo.AppScenesManager, mApplicationInfo.AppInputManager);
        mApplicationInfo.Client = mObjectManager->GetObjectAsUser<GameClient>(gameClientHandle);
        mApplicationInfo.AppInputManager->Init(mApplicationInfo.Client);
        PLU_CORE_INFO("Started Game!");
    }

    void Application::EndGame()
    {
        if (!mApplicationInfo.Client) return;
        mObjectManager->DestroyObject(*mApplicationInfo.Client->GetEngineObjectHandle());
        mApplicationInfo.Client = nullptr;
    }

    void Application::EngineInit()
    {
        Plu::Log::Init();
        InitEngineReflection();
        Engine::CreateEngine();
        PLU_CORE_INFO("Engine Init");
        mObjectManager = Plu::CreateOwning<EngineObjectManager>();
        TypeRegistry::GetInstance()->mApplicationInfo = &mApplicationInfo;
        mApplicationInfo.AppRenderingManager = mObjectManager->GetObjectAsOwner<RenderingManager>(mObjectManager->CreateObject<RenderingManager>(&mApplicationInfo));
        mApplicationInfo.AppObjectManager = mObjectManager;

#ifdef PLU_PLATFORM_LINUX
        SDLWindow::InitSDL();
#endif
    }

    void Application::EngineShutdown()
    {
        mRenderer->OnShutdown();
        mApplicationInfo.AppRenderingManager->Shutdown();
        mObjectManager->DestroyObject(*mApplicationInfo.AppRenderingManager->GetEngineObjectHandle());
        mApplicationInfo.AppRenderingManager = nullptr;
        Engine::DestroyEngine();
        PLU_CORE_WARN("Engine Shutdown");
        mObjectManager = nullptr;
        mWindow->Shutdown();
    }
}
