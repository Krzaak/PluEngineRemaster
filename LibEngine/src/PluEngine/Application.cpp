//
// Created by Plutex on 12/30/25.
//

#include "PluEngine/Application.h"

#include "Platforms/Linux/SDLGLContext.h"
#include "Platforms/Linux/SdlWindow.h"
#include "Platforms/Windows/WindowsWindow.h"
#include "PluEngine/Engine.h"
#include "PluEngine/Log.h"
#include "PluEngine/Timer.h"
#include "PluEngine/Assets/EngineAssetManager.h"
#include "PluEngine/Managers/RenderingManager.h"
#include "PluEngine/Managers/ScenesManager.h"
#include "PluEngine/Window/Window.h"
#include "PluEngine/Objects/EngineObjectManager.h"
#include "PluEngine/GameCore/GameClient.h"
#include "PluEngine/Input/InputManager.h"
#include "PluEngine/Physics/JoltIntializer.h"
#include "PluEngine/PluUtils.h"
#include "PluEngine/Renderer/RenderSnapshotBuilder.h"
#include "PluEngine/Renderer/RenderThreading.h"
#include "PluEngine/Scenes/SceneManager.h"
#include "PluEngine/Threading/ThreadAffinity.h"
#include "PluEngine/Threading/TripleBuffer.h"

extern void InitLibEngineReflection();

namespace Plu
{
    DeserializationContext * ApplicationInfo::ConstructDeserializationContext() const
    {
        DeserializationContext* context = new DeserializationContext();
        context->assetManager = AppAssetManager;
        context->scenesManager = AppScenesManager;
        context->shaderManager = AppShaderManager;
        return context;
    }

    Application::Application()
    {
        PLU_TIMER_START("EngineInit");
        EngineInit();
    }

    Application::~Application()
    {
        EngineShutdown();
    }

    void Application::InjectArguments(argparse::ArgumentParser *parser)
    {
        mArgumentParser = parser;
    }

    void Application::Run()
    {
        if (!OnInit()) {
            PLU_CORE_CRITICAL("Error during initialization! Aborting launch!");
            return;
        }
        mApplicationInfo.AppWindow->Init();
        mApplicationInfo.AppWindow->GetObjectEventDispatcher()->Subscribe("WindowCloseRequested", [this](void*) {
            OnRequestedWindowClose(mApplicationInfo.AppWindow);
        });
        mApplicationInfo.AppInputManager->GetInputBackend()->Init();
#ifdef PLU_PLATFORM_WINDOWS
        //DynamicCast<WindowsWindow>(mApplicationInfo.AppWindow)->SpawnConsoleWindow();
        //PLU_CORE_TRACE("Console Window Spawned!");
#endif
#ifdef PLU_PLATFORM_LINUX
        SDL_GLContext context = static_cast<SDL_GLContext>(mApplicationInfo.AppWindow->GetGLContext());
        SDLGLContext::InitGLContext(mApplicationInfo.AppWindow, context);
#endif

        mApplicationInfo.AppScenesManager->Initialize(&mApplicationInfo);
        OnPostInit();

        PLU_CORE_TRACE("Initialized Successfully!");
        PLU_TIMER_END("EngineInit");

        //Just here we drop off the GL context from main thread to the render thread
        //I assume we have done all the preparations, and now we are ready to give control to render thread
        //It will from now on handle shader compilation, loading meshes and textures and of course rendering
        TripleBuffer<RenderSnapshot*> renderTripleBuffer;
        RenderSnapshotBuilder renderSnapshotBuilder = RenderSnapshotBuilder(&renderTripleBuffer, &mApplicationInfo);

        mApplicationInfo.AppWindow->ReleaseGLContext();
        mApplicationInfo.AppRenderingManager->Initialize(&renderTripleBuffer);

        std::chrono::high_resolution_clock::time_point lastFrame = std::chrono::high_resolution_clock::now();

        while (mApplicationInfo.AppWindow && mApplicationInfo.AppWindow->IsRunning()) {
            float deltaTime = std::chrono::duration<float>(std::chrono::high_resolution_clock::now() - lastFrame).count();
            lastFrame = std::chrono::high_resolution_clock::now();
            SetMainThreadDeltaTime(deltaTime);
#ifdef PLU_PLATFORM_LINUX
            SDLWindow::HandleSDLEvents();
#elif defined(PLU_PLATFORM_WINDOWS)
            mApplicationInfo.AppWindow->OnUpdate(deltaTime);
#endif
            PLU_PROFILE_SCOPE("Frame");
            {
                PLU_PROFILE_SCOPE("Input Update");
                if (mUpdateInput && mApplicationInfo.AppWindow->HasWindowFocus()) mApplicationInfo.AppInputManager->GetInputBackend()->Update();
            }
            {
                PLU_PROFILE_SCOPE("App OnTick");
                OnTick(deltaTime);
            }
            {
                PLU_PROFILE_SCOPE("Scenes Update");
                if (mApplicationInfo.AppScenesManager) mApplicationInfo.AppScenesManager->OnUpdate(deltaTime);
            }
            {
                PLU_PROFILE_SCOPE("Process Pending Asset Loads");
                // Drain deferred load requests posted by the render thread (GetAssetDataNoLoad
                // misses) so their CPU data is in cache before the next snapshot is consumed.
                if (mApplicationInfo.AppAssetManager) mApplicationInfo.AppAssetManager->ProcessPendingLoads();
            }
            {
                PLU_PROFILE_SCOPE("Render Snapshot Building");
                renderSnapshotBuilder.BuildSnapshotAndPublish(deltaTime);
                //mApplicationInfo.AppRenderer->OnUpdate(deltaTime);
            }
            {
                PLU_PROFILE_SCOPE("Input EndFrame");
                mApplicationInfo.AppInputManager->GetInputBackend()->EndFrame();
            }
        }
        PLU_TIMER_START("EngineEnd");
        // Stop & join the render thread FIRST. OnShutdown() destroys scenes, cameras, viewports
        // and managers that the render thread reads every frame (textures/meshes/shaders via the
        // RenderingManager and the snapshot). If the render thread is still alive during teardown
        // it races those destructions and ends up calling SwapBuffer()/GetWidth() on an already
        // destroyed window (mWindow == nullptr) -> segfault.
        mApplicationInfo.AppRenderingManager->Shutdown();
        OnShutdown();
#ifdef PLU_PLATFORM_LINUX
        if (context)
        {
            SDL_GL_DestroyContext(context);
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
        return mApplicationInfo.AppWindow;
    }

    ApplicationInfo * Application::GetAppInfo()
    {
        return &mApplicationInfo;
    }

    TUsePointer<GameClient> gGameClient;

    void Application::StartGame()
    {
        EngineObjectHandle gameClientHandle = mObjectManager->CreateObject<GameClient>(mObjectManager, mApplicationInfo.AppScenesManager, mApplicationInfo.AppInputManager, mApplicationInfo.AppWindow);
        mApplicationInfo.Client = mObjectManager->GetObjectAsUser<GameClient>(gameClientHandle);
        mApplicationInfo.AppInputManager->Init(mApplicationInfo.Client, mApplicationInfo.AppWindow);
        gGameClient = mApplicationInfo.Client;
        PLU_CORE_INFO("Started Game!");
    }

    void Application::EndGame()
    {
        if (!mApplicationInfo.Client) return;
        mObjectManager->DestroyObject(*mApplicationInfo.Client->GetEngineObjectHandle());
        mApplicationInfo.Client = nullptr;
    }

    void Application::DispatchWindowClose(TUsePointer<IWindow> window)
    {
        window->Close();
    }

    static Application* gApplication;

    void Application::EngineInit()
    {
        Plu::RegisterMainThread();
        gApplication = this;
        Plu::Log::Init();
        InitLibEngineReflection();
        Engine::CreateEngine();
        PLU_CORE_INFO("Engine Init");
        mObjectManager = Plu::CreateOwning<EngineObjectManager>();
        TypeRegistry::GetInstance()->mApplicationInfo = &mApplicationInfo;
        mApplicationInfo.AppRenderingManager = mObjectManager->GetObjectAsOwner<RenderingManager>(mObjectManager->CreateObject<RenderingManager>(&mApplicationInfo));
        mApplicationInfo.AppObjectManager = mObjectManager;

        mApplicationInfo.AppAssetManager = mObjectManager->CreateObject(EngineAssetManager::GetStaticClass());
        mApplicationInfo.AppAssetManager->Initialize(&mApplicationInfo);

        mApplicationInfo.AppScenesManager = mObjectManager->CreateObject(SceneManager::GetStaticClass());

#ifdef PLU_PLATFORM_LINUX
        SDLWindow::InitSDL();
#endif

        JoltPhysics::Init();
    }

    void Application::EngineShutdown()
    {
        JoltPhysics::Shutdown();
        mObjectManager->DestroyObject(*mApplicationInfo.AppRenderingManager->GetEngineObjectHandle());
        mApplicationInfo.AppRenderingManager = nullptr;
        Engine::DestroyEngine();
        PLU_CORE_WARN("Engine Shutdown");
        mObjectManager = nullptr;
        //mWindow->Shutdown();
        PLU_TIMER_END("EngineEnd");
    }

    TUsePointer<GameClient> GetGameClient()
    {
        return gGameClient;
    }

    void ExitGame()
    {
        gApplication->OnRequestedGameExit();
    }
}
