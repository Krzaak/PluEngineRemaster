//
// Created by Plutex on 12/30/25.
//

#include "PluEngine/Application.h"

#include <optional>
#include <thread>

#include "Platforms/Linux/SdlWindow.h"
#include "Platforms/Windows/WindowsWindow.h"
#include "PluEngine/Engine.h"
#include "PluEngine/Log.h"
#include "PluEngine/Timer.h"
#include "PluEngine/Assets/EngineAssetManager.h"
#include "PluEngine/AssetTypes/AnimationGraph/AnimationGraph.h"
#include "PluEngine/Managers/DiskManager.h"
#include "PluEngine/Managers/RenderingManager.h"
#include "PluEngine/Managers/ScenesManager.h"
#include "PluEngine/Profiler.h"
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

    void Application::AddEngineArguments(argparse::ArgumentParser& parser)
    {
        parser.add_argument("--profiler-export-after")
            .help("Write the profiler CSV this many seconds after the main loop starts, then keep running")
            .scan<'g', double>();
        parser.add_argument("--profiler-export-path")
            .help("Where --profiler-export-after writes its CSV (default: profiler.csv)");
    }

    namespace
    {
        // Reads an optional argument without caring whether the app's main registered it — argparse
        // throws std::logic_error for a name it has never seen, and not every executable adds the
        // engine arguments.
        template <typename T>
        std::optional<T> PresentArgument(argparse::ArgumentParser* parser, const char* name)
        {
            if (!parser) return std::nullopt;
            try {
                return parser->present<T>(name);
            } catch (...) {
                return std::nullopt;
            }
        }
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
        // Kontekst ImGui (+ styl, backend platformowy) tworzy RenderingManager — jeszcze na
        // main thread, przed inicjalizacją inputu i OnPostInit(), które mogą już dotykać ImGui.
        // Domyślny stan GL (depth test itd.) ustawia render thread w RenderThreadEnter() —
        // wcześniej robił to linuksowy SDLGLContext, przez co Windows startował bez depth testu.
        mApplicationInfo.AppRenderingManager->InitializeImGuiContext();
        mApplicationInfo.AppInputManager->GetInputBackend()->Init();
#ifdef PLU_PLATFORM_WINDOWS
        //DynamicCast<WindowsWindow>(mApplicationInfo.AppWindow)->SpawnConsoleWindow();
        //PLU_CORE_TRACE("Console Window Spawned!");
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

        // One-shot profiler dump (--profiler-export-after). Gives a scripted run the same CSV the
        // Profiler panel exports, which otherwise only comes out of a native save dialog. The app
        // keeps running afterwards — close the window or kill the process when done.
        const std::optional<double> profilerExportAfter = PresentArgument<double>(mArgumentParser, "--profiler-export-after");
        const std::optional<std::string> profilerExportPathArg = PresentArgument<std::string>(mArgumentParser, "--profiler-export-path");
        const String profilerExportPath = profilerExportPathArg ? String(profilerExportPathArg->c_str()) : String("profiler.csv");
        float profilerElapsed = 0.0f;
        bool profilerExported = false;

        while (mApplicationInfo.AppWindow && mApplicationInfo.AppWindow->IsRunning()) {
            const std::chrono::high_resolution_clock::time_point frameStart = std::chrono::high_resolution_clock::now();
            float deltaTime = std::chrono::duration<float>(frameStart - lastFrame).count();
            lastFrame = frameStart;

            if (deltaTime > 1.0f) {
#ifdef PLU_DEBUG
                PLU_CORE_CRITICAL("Unbelievably high deltaTime, capping to 1");
#endif
                deltaTime = 1.0f;
            }

            SetMainThreadDeltaTime(deltaTime);
#ifdef PLU_PLATFORM_LINUX
            SDLWindow::HandleSDLEvents();
#elif defined(PLU_PLATFORM_WINDOWS)
            mApplicationInfo.AppWindow->OnUpdate(deltaTime);
#endif
            PLU_PROFILE_SCOPE("Frame");
            {
                PLU_PROFILE_SCOPE("Input Update");
                if (mApplicationInfo.AppWindow->HasWindowFocus()) mApplicationInfo.AppInputManager->GetInputBackend()->Update();
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
            {
                // Frame pacing. The render thread is paced by VSync (SwapBuffer blocks on the refresh),
                // so with e.g. 60Hz it settles at ~16.6ms/frame while Main, having nothing to block on,
                // free-runs at thousands of fps and burns a core for nothing. Keep Main a touch ahead of
                // the render cadence: target a frame time slightly shorter than the render frame time so a
                // fresh snapshot is always ready before the render thread consumes it, then sleep off the
                // leftover budget. If Main's own work already overran that target there is no budget left —
                // skip the sleep and run at full speed (the intended behaviour when Main is the bottleneck).
                PLU_PROFILE_SCOPE("Main Frame Pacing");
                const float renderFPS = GetRenderThreadFPS();
                if (renderFPS > 0.0f) {
                    // 0.9 -> Main runs ~11% faster than render (e.g. ~66fps vs a 60Hz render). Proportional
                    // margin so it scales with the refresh rate (240Hz -> ~4.16ms render, ~3.75ms target).
                    const float targetFrameTime = (1.0f / renderFPS) * 0.9f;
                    const float workElapsed = std::chrono::duration<float>(
                        std::chrono::high_resolution_clock::now() - frameStart).count();
                    const float sleepFor = targetFrameTime - workElapsed;
                    if (sleepFor > 0.0f) {
                        std::this_thread::sleep_for(std::chrono::duration<float>(sleepFor));
                    }
                }
            }

            // Placed after the frame's work so the dump includes this frame's samples.
            if (profilerExportAfter && !profilerExported) {
                profilerElapsed += deltaTime;
                if (profilerElapsed >= static_cast<float>(*profilerExportAfter)) {
                    profilerExported = true;
                    if (DiskManager::SaveText(profilerExportPath.ToWide(), Profiler::GetInstance()->BuildCsv())) {
                        PLU_CORE_INFO("Profiler exported to {} after {}s", profilerExportPath.CStr(), profilerElapsed);
                    } else {
                        PLU_CORE_ERROR("Profiler export to {} failed", profilerExportPath.CStr());
                    }
                }
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
        // Render thread is joined and the loop is done — nobody reads or writes the snapshot
        // triple buffer anymore. Free the lazily allocated slots (BuildSnapshotAndPublish).
        for (RenderSnapshot*& slot : renderTripleBuffer.GetBuffersForTeardown()) {
            delete slot;
            slot = nullptr;
        }
#ifdef PLU_PLATFORM_LINUX
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

        // Shared by Editor and Runtime so a Runtime build's factory isn't empty (previously only the
        // editor populated it — see AnimationGraphVariableFactory::RegisterBuiltInTypes).
        AnimationGraphVariableFactory::RegisterBuiltInTypes();

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
        // Last thing in the engine's life: free the whole reflection registry
        // (TypeInfo/PropertyInfo/EnumInfo). Nothing may touch reflection past this point.
        TypeRegistry::GetInstance()->Cleanup();
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
