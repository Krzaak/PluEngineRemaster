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
#include "PluEngine/Physics/JoltIntializer.h"
#include "PluEngine/Window/WindowManager.h"

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
        mApplicationInfo.AppWindowsManager->ProcessNewWindows();
        if (!mApplicationInfo.AppWindowsManager->GetFirstWindow()) {
            PLU_CORE_ERROR("Launching in CLI mode!");
        }
        if (!mApplicationInfo.AppRenderer) {
            OnShutdown();
            PLU_CORE_CRITICAL("Application has no Active Renderer!");
            return;
        }

        mApplicationInfo.AppWindow = mApplicationInfo.AppWindowsManager->GetFirstWindow();
        mApplicationInfo.AppInputManager->GetInputBackend()->Init();
#ifdef PLU_PLATFORM_WINDOWS
        //DynamicCast<WindowsWindow>(mWindow)->SpawnConsoleWindow();
#endif
        mApplicationInfo.AppRenderer->Init(mApplicationInfo.AppWindowsManager->GetFirstWindow());
#ifdef PLU_PLATFORM_LINUX
        SDL_GLContext context = mApplicationInfo.AppWindowsManager->GetFirstWindow()->GetGLContext();
#endif

        OnPostInit();

        PLU_CORE_TRACE("Initialized Successfully!");
        PLU_TIMER_END("EngineInit");

        std::chrono::high_resolution_clock::time_point lastFrame = std::chrono::high_resolution_clock::now();

        while (mApplicationInfo.AppWindowsManager->GetFirstWindow() && mApplicationInfo.AppWindowsManager->GetFirstWindow()->IsRunning()) {
            float deltaTime = std::chrono::duration<float>(std::chrono::high_resolution_clock::now() - lastFrame).count();
            lastFrame = std::chrono::high_resolution_clock::now();
#ifdef PLU_PLATFORM_LINUX
            SDLWindow::HandleSDLEvents();
#elif defined(PLU_PLATFORM_WINDOWS)
            mApplicationInfo.AppWindowsManager->UpdateEvents();
#endif
            mApplicationInfo.AppInputManager->GetInputBackend()->Update();
            OnTick(deltaTime);
            mApplicationInfo.AppScenesManager->TickScene(deltaTime);
            mApplicationInfo.AppRenderingManager->Tick(deltaTime);
            mApplicationInfo.AppRenderer->OnUpdate(deltaTime);
            mApplicationInfo.AppInputManager->GetInputBackend()->EndFrame();
            mApplicationInfo.AppWindowsManager->ProcessNewWindows();
        }
        OnShutdown();
#ifdef PLU_PLATFORM_LINUX
        if (context)
        {
            SDL_GL_DeleteContext(context);
            context = nullptr;
        }
        SDL_Quit();
#endif
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
        return mApplicationInfo.AppWindowsManager->GetFirstWindow();
    }

    ApplicationInfo * Application::GetAppInfo()
    {
        return &mApplicationInfo;
    }

    TUsePointer<GameClient> gGameClient;

    void Application::StartGame()
    {
        EngineObjectHandle gameClientHandle = mObjectManager->CreateObject<GameClient>(mObjectManager, mApplicationInfo.AppScenesManager, mApplicationInfo.AppInputManager);
        mApplicationInfo.Client = mObjectManager->GetObjectAsUser<GameClient>(gameClientHandle);
        mApplicationInfo.AppInputManager->Init(mApplicationInfo.Client);
        gGameClient = mApplicationInfo.Client;
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
        mApplicationInfo.AppWindowsManager = mObjectManager->CreateObject(WindowsManager::GetStaticClass());
        mApplicationInfo.AppWindowsManager->Init(mObjectManager, &mApplicationInfo);

#ifdef PLU_PLATFORM_LINUX
        SDLWindow::InitSDL();
#endif

        JoltPhysics::Init();
    }

    void Application::EngineShutdown()
    {
        JoltPhysics::Shutdown();
        mApplicationInfo.AppRenderer->OnShutdown();
        mApplicationInfo.AppRenderingManager->Shutdown();
        mObjectManager->DestroyObject(*mApplicationInfo.AppRenderingManager->GetEngineObjectHandle());
        mApplicationInfo.AppRenderingManager = nullptr;
        Engine::DestroyEngine();
        PLU_CORE_WARN("Engine Shutdown");
        mObjectManager = nullptr;
        //mWindow->Shutdown();
    }

    TUsePointer<GameClient> GetGameClient()
    {
        return gGameClient;
    }
}
