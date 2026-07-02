//
// Created by Plutex on 12/29/25.
//

#include "EditorApp.h"

#include "EditorAppContext.h"
#include "EditorInterface.h"
#include "DefinedPanels/EngineStatsPanel.h"
#include "DefinedPanels/Style/EditorStylePanel.h"
#include "Managers/Project/EditorProjectManager.h"
#include "PluEngine/Log.h"
#include "PluEngine/Timer.h"
#include "PluEngine/Managers/RenderingManager.h"
#include "PluEngine/Objects/EngineObjectHandle.h"
#include "PluEngine/Objects/EngineObjectManager.h"
#include "PluEngine/Window/Window.h"
#include "Panels/EditorPanelManager.h"
#include "imgui_stdlib.h"

#ifdef PLU_PLATFORM_WINDOWS
#include "imgui_impl_win32.h"
#elif defined(PLU_PLATFORM_LINUX)
#include "imgui_impl_sdl3.h"
#endif

#include "nfd.h"
#include "ImGuizmo.h"
#include "json_fwd.hpp"
#include "DefinedPanels/EngineClassTreePanel.h"
#include "EditorViewports/EditorViewportManager.h"
#include "EditorWindows/EditorWindowsManager.h"
#include "Managers/Assets/EditorAssetManager.h"
#include "Managers/Python/EditorPythonManager.h"
#include "Managers/Scene/EditorCamera.h"
#include "Managers/Shaders/EditorShaderManager.h"
#include "PluEngine/Engine.h"
#include "PluEngine/PluPaths.h"
#include "PluEngine/GameCore/GameClient.h"
#include "PluEngine/Input/InputManager.h"
#include "PluEngine/Managers/DiskManager.h"
#include "PluEngine/Scenes/SceneWorld.h"
#include "UI/IconsFontAwesome7.h"

extern void InitEditorReflection();

//For editor only
Plu::TUsePointer<Plu::EngineObjectManager> gEngineObjectManager;
Plu::EditorAppContext* gEditorAppContext;
Plu::ApplicationInfo* gApplicationInfo;

Plu::PluEditor* gPluEditor;

Plu::PluEditor::PluEditor() : Application()
{
    gPluEditor = this;
    gWindowClass = new ImGuiWindowClass();
    gWindowClass->DockNodeFlagsOverrideSet = ImGuiDockNodeFlags_NoSplit | ImGuiDockNodeFlags_NoCloseButton | ImGuiDockNodeFlags_NoWindowMenuButton;
}

Plu::PluEditor::~PluEditor()
{
}

bool Plu::PluEditor::OnInit()
{
    NFD_Init();
    InitEditorReflection();
    mEditorAppContext = new EditorAppContext;
    Plu::WindowProperties props;
    props.Title = "Plu Editor";
    props.Borderless = true;
    mApplicationInfo.AppWindow = IWindow::PlutexCreateWindow(props, mObjectManager, &mApplicationInfo);
    TUsePointer<EditorProjectManager> projectManager = mObjectManager->CreateObject(EditorProjectManager::GetStaticClass());
    mEditorProjectManager = mObjectManager->GetObjectAsOwner<EditorProjectManager>(projectManager->GetObjectHandle());
    mEditorProjectManager->SetEditorAppContext(mEditorAppContext, &mApplicationInfo);
    mEditorAppContext->EditorPythonManager = mObjectManager->CreateObject(EditorPythonManager::GetStaticClass());
    mEditorAppContext->EditorViewportManager = mObjectManager->CreateObject(EditorViewportManager::GetStaticClass());
    mEditorAppContext->EditorShaderManager = mObjectManager->CreateObject(EditorShaderManager::GetStaticClass());
    const EngineObjectHandle panelManagerHandle = mObjectManager->CreateObject<EditorPanelManager>();
    mPanelManager = mObjectManager->GetObjectAsOwner<EditorPanelManager>(panelManagerHandle);
    PLU_INFO("Editor Init");
    gEditorAppContext = mEditorAppContext;
    gEngineObjectManager = mObjectManager;
    gApplicationInfo = &mApplicationInfo;
    mEditorAppContext->EditorAssetManager = gApplicationInfo->AppAssetManager;
    mEditorAppContext->EditorScenesManager = gApplicationInfo->AppScenesManager;
    mEditorAppContext->EditorShaderManager->PreInit(mEditorProjectManager);
    mEditorAppContext->EditorPanelManager = mPanelManager;
    mEditorAppContext->EditorProjectManager =  mEditorProjectManager;
    mPanelManager->Init(&mApplicationInfo, mEditorAppContext, &gDockspaceId);
    mPanelManager->Init();
    mApplicationInfo.AppScenesManager = mEditorAppContext->EditorScenesManager;
    mApplicationInfo.AppShaderManager = mEditorAppContext->EditorShaderManager;
    mEditorAppContext->EditorWindowsManager = mObjectManager->CreateObject(EditorWindowsManager::GetStaticClass());

    EngineObjectHandle inputManagerHandle = mObjectManager->CreateObject<InputManager>();
    mApplicationInfo.AppInputManager = mObjectManager->GetObjectAsUser<InputManager>(inputManagerHandle);

    mApplicationInfo.AppAssetManager->PrepareLoaders();
    return true;
}

void Plu::PluEditor::OnPostInit()
{
    mEditorAppContext->EditorSceneCamera = mObjectManager->CreateObject(EditorSceneCamera::GetStaticClass());
    mApplicationInfo.AppScenesManager->GetObjectEventDispatcher()->Subscribe("EditorCameraWanted", [this](void* data) {
        IRendererCamera** cameraFieldPtr = static_cast<IRendererCamera**>(data);
        *cameraFieldPtr = mEditorAppContext->EditorSceneCamera.GetRaw();
    });

    mApplicationInfo.AppScenesManager->GetObjectEventDispatcher()->Subscribe("EditorCameraLocationToSave", [this](void* data) {
        Vec3* location = static_cast<Vec3*>(data);
        *location = mEditorAppContext->EditorSceneCamera->GetCameraLocation();
    });
    mApplicationInfo.AppScenesManager->GetObjectEventDispatcher()->Subscribe("EditorCameraRotationToSave", [this](void* data) {
        Vec3* rotation = static_cast<Vec3*>(data);
        *rotation = mEditorAppContext->EditorSceneCamera->GetHumanReadableRotation();
    });

    mApplicationInfo.AppScenesManager->GetObjectEventDispatcher()->Subscribe("EditorCameraLocationLoaded", [this](void* data) {
        Vec3* location = static_cast<Vec3*>(data);
        mEditorAppContext->EditorSceneCamera->SetCameraLocation(*location);
    });
    mApplicationInfo.AppScenesManager->GetObjectEventDispatcher()->Subscribe("EditorCameraRotationLoaded", [this](void* data) {
        Vec3* rotation = static_cast<Vec3*>(data);
        mEditorAppContext->EditorSceneCamera->SetCameraRotation(*rotation);
    });


    ImGui::SetCurrentContext(mApplicationInfo.AppWindow->GetImGuiContext());
    //Fonts
    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->Clear();
    //io.Fonts->AddFontDefault(); // Ładujemy standardową czcionkę

    static constexpr ImWchar icons_ranges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
    ImFontConfig icons_config;
    icons_config.MergeMode = true; // KLUCZOWE: to łączy ikony z tekstem
    icons_config.PixelSnapH = true;

    // Ścieżka do pliku .ttf
    // String path = "";
    // path += PLU_FONTS_DIR;
    // path += "Font Awesome 7 Free-Regular-400.otf";

    std::string pathStd = GetEngineResourcesDir().Append(L"ThirdParty/UI/Fonts/").ToString().ToNarrow().CStr();
    pathStd += "Font Awesome 7 Free-Regular-400.otf";
    std::string path2 = GetEngineResourcesDir().Append(L"ThirdParty/UI/Fonts/").ToString().ToNarrow().CStr();
    path2 += "Font Awesome 7 Free-Solid-900.otf";
    std::string pathOpenSans = GetEngineResourcesDir().Append(L"ThirdParty/UI/Fonts/").ToString().ToNarrow().CStr();
    pathOpenSans += "OpenSans-Regular.ttf";

    io.Fonts->AddFontFromFileTTF(pathOpenSans.c_str(), 19.0f);
    io.Fonts->AddFontFromFileTTF(pathStd.c_str(), 13.0f, &icons_config, icons_ranges);
    io.Fonts->AddFontFromFileTTF(path2.c_str(), 13.0f, &icons_config, icons_ranges);
}

void Plu::PluEditor::OnShutdown()
{
    PLU_INFO("Editor Shutdown");
    mEditorAppContext->EditorScenesManager->ExitPIE();
    EndGame();
    mPanelManager->Shutdown();
    mEditorAppContext->EditorViewportManager->Shutdown();
    mEditorAppContext->EditorScenesManager->SetEditorRenderCamera(nullptr);
    mObjectManager->DestroyObject(*mEditorAppContext->EditorSceneCamera->GetEngineObjectHandle());
    mEditorAppContext->EditorProjectManager->Shutdown();
    mObjectManager->DestroyObject(*mEditorAppContext->EditorViewportManager->GetEngineObjectHandle());
    mObjectManager->DestroyObject(*mEditorAppContext->EditorScenesManager->GetEngineObjectHandle());
    mObjectManager->DestroyObject(*mEditorAppContext->EditorAssetManager->GetEngineObjectHandle());
    mObjectManager->DestroyObject(*mEditorAppContext->EditorPanelManager->GetEngineObjectHandle());
    mObjectManager->DestroyObject(*mEditorAppContext->EditorProjectManager->GetEngineObjectHandle());
    delete mEditorAppContext;
    NFD_Quit();
}

float lastDeltaTime = 0.0f;

void Plu::PluEditor::OnImGuiRender()
{
    ImGui::SetCurrentContext(mApplicationInfo.AppWindow->GetImGuiContext());
    if (mEditorAppContext->PIEFullscreen) {
        if (mApplicationInfo.AppInputManager->GetInputBackend()->GetKeyboard().IsDown(Key::Escape)) {
            mEditorAppContext->EditorScenesManager->ExitPIE();
            EndGame();
            mEditorAppContext->PIEFullscreen = false;
            gApplicationInfo->AppRenderingManager->SetImGuiRenderingIgnorance(false);
            gApplicationInfo->AppWindow->SetCursorVisibility(true);
        }
        return;
    }
    if (gEditorAppContext->EditorScenesManager->IsInPIE()) {
        if (mApplicationInfo.AppInputManager->GetInputBackend()->GetKeyboard().IsDown(Key::F8)) {
            mUpdateInput = false;
            gApplicationInfo->AppWindow->SetCursorVisibility(true);
            gApplicationInfo->AppWindow->UpdateImGui = true;
        }
        if (mApplicationInfo.AppInputManager->GetInputBackend()->GetKeyboard().IsDown(Key::Escape)) {
            mEditorAppContext->EditorScenesManager->ExitPIE();
            EndGame();
            gApplicationInfo->AppWindow->SetCursorVisibility(true);
            gApplicationInfo->AppWindow->UpdateImGui = true;
        }
    }
    DrawMainEngineWindow(0);
    if (mEditorAppContext->NewProjectPopup) ImGui::OpenPopup("New Project");
    if (ImGui::BeginPopupModal("New Project")) {
        static String pathToNewProject;
        static String projectName;
        static bool firstTime;
        if (firstTime) {
            projectName.Reserve(30);
            firstTime = false;
        }
        if (ImGui::Button("Select Path")) {
            nfdu8char_t* outPath = nullptr;
            if (NFD_PickFolderU8(&outPath, nullptr) == NFD_OKAY) {
                pathToNewProject = outPath;
                NFD_FreePathU8(outPath);
            }
        }
        String previewPath = pathToNewProject + "/" + projectName;
        ImGui::Text("%s",previewPath.CStr());
        std::string previewTemp;
        if (ImGui::InputTextWithHint("Project Name", "Hint", &previewTemp)) {
            projectName = previewTemp.c_str();
        }
        ImGui::Separator();
        if (ImGui::Button("Create")) {
            mEditorProjectManager->CreateNewProject(StringW::FromNarrow(pathToNewProject.CStr()),projectName);
            pathToNewProject = "";
            projectName = "";
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
            mEditorAppContext->NewProjectPopup = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    static bool dockedSomething = false;
    bool dockedPanels = false;

    if (mPanelManager->AreTherePanelsToDock()) {
        mPanelManager->InitNewPanels();
        dockedPanels = true;
    }

    mEditorAppContext->EditorViewportManager->Tick(lastDeltaTime);
    mPanelManager->OnUpdate(lastDeltaTime, 0);

    if (dockedPanels) {
        mPanelManager->DockNewPanels();
        dockedSomething = true;
    }
    if (mEditorAppContext->EditorViewportManager->AreThereViewportsToDock() && !dockedSomething) {
        mEditorAppContext->EditorViewportManager->DockNewViewports();
        dockedSomething = true;
    }
    dockedSomething = false;
}

void Plu::PluEditor::OnImGuiRenderEX(UInt64 windowID)
{
    ImGui::SetCurrentContext(Engine::GetEngine()->GetImGuiContext());
    DrawMainEngineWindow(static_cast<int>(windowID));
    mPanelManager->OnUpdate(lastDeltaTime, static_cast<int>(windowID));
}

// True if any ImGui texture still has work in flight that the render thread will act on - either a
// pending create/update/destroy (Status != OK) or a destroy QUEUED for next frame
// (WantDestroyNextFrame, Status still OK this frame). A frame that leaves work pending MUST be
// handed to the render thread in lockstep: its draw commands reference those textures and the
// render thread has to upload them BEFORE it draws (otherwise GetTexID()==0 asserts), and the atlas
// must not be mutated again until that pass is done.
//
// WantDestroyNextFrame is the crucial one: when a font-atlas rebuild creates a new texture it marks
// the old one WantDestroyNextFrame but leaves its Status at OK for this frame. Checking Status alone
// would let us drop out of lockstep here, and then NEXT frame the Main thread flips that texture to
// WantDestroy / bumps UnusedFrames while the render thread is concurrently rendering the previous
// snapshot (which still lists the old texture) and destroys it mid-flight -> GetTexID()==0. Staying
// in lockstep until the whole create->destroy->remove flush is done (a few frames, UnusedFrames-
// gated) keeps that destroy frame on the Main thread alone. Call after ImGui::Render() (when
// GetPlatformIO().Textures is up to date).
static bool ImGuiHasPendingTextureWork()
{
    const ImGuiPlatformIO& platformIo = ImGui::GetPlatformIO();
    for (ImTextureData* tex : platformIo.Textures) {
        if (tex->Status != ImTextureStatus_OK || tex->WantDestroyNextFrame) {
            return true;
        }
    }
    return false;
}

void Plu::PluEditor::OnTick(float deltaTime)
{
    lastDeltaTime = deltaTime;
    if (mEditorAppContext->EditorScenesManager->GetCurrentWorld() && !mEditorAppContext->EditorScenesManager->IsInPIE()) {
        mEditorAppContext->EditorScenesManager->GetCurrentWorld()->HandleDestroy();
    }
    mEditorAppContext->EditorWindowsManager->OnUpdate(deltaTime);
    static int frameCounter = 0;
    frameCounter++;
    if (frameCounter >= 100) {
        frameCounter = 0;
        mEditorAppContext->EditorShaderManager->CheckForShaderChanges();
    } else if (frameCounter >= 5 && !mEditorProjectManager->IsAnyProjectOpen() && mArgumentParser) {
        try {
            std::string projectPath = mArgumentParser->get<std::string>("project");
            mEditorProjectManager->OpenProject(StringW::FromNarrow(projectPath.c_str()));
        } catch (...) {

        }
    }

    // Build this frame's ImGui UI on the Main thread, then hand the draw data to the render
    // thread. ImGui_ImplSDL2_NewFrame()/ImGui::NewFrame() run here (input + display size, no
    // GL); the OpenGL3 backend submits the cloned draw data on the render thread.
    if (ImGuiContext* ctx = mApplicationInfo.AppWindow->GetImGuiContext()) {
        PLU_PROFILE_SCOPE("ImGui Build");
        ImGui::SetCurrentContext(ctx);

        // ImGui 1.92 dynamic fonts rebuild/grow the shared font atlas (create a new ImTextureData,
        // destroy the old) whenever glyphs are (re)baked - not only on the frame the font size
        // changes, but on later frames too as new glyphs are lazily baked at the new size. The
        // lock-free ImGui handoff only deep-copies draw lists, not texture state, so if the atlas
        // mutates while the render thread is reading a snapshot the two threads race: the render
        // thread either asserts in ImGui's create/destroy paths, or draws a freshly created texture
        // before it was uploaded (GetTexID()==0). While any texture work is in flight we drive the
        // handoff in lockstep so only one thread touches the atlas at a time.
        //
        // Engage BEFORE NewFrame() when we already know a mutation is coming (queued font-size
        // change) or a previous frame left work in flight - this covers the destroy side, where the
        // mutation would otherwise race a render-thread read of the prior snapshot.
        const ImGuiStyle& style = ImGui::GetStyle();
        bool lockstepEngaged = (style._NextFrameFontSizeBase != 0.0f) || mImGuiAtlasSettling;
        if (lockstepEngaged) {
            mApplicationInfo.AppRenderingManager->BeginImGuiLockstep();
        }

#ifdef PLU_PLATFORM_WINDOWS
        ImGui_ImplWin32_NewFrame();
#elif defined(PLU_PLATFORM_LINUX)
        ImGui_ImplSDL3_NewFrame();
#endif
        ImGui::NewFrame();
        // Musi lecieć raz na klatkę, po NewFrame, zanim którykolwiek panel woła ImGuizmo::Manipulate.
        ImGuizmo::BeginFrame();
        OnImGuiRender();
        if (mAssetSaveConfirmShow) {
            bool closeWindow = false;
            AssetSaveConfirm(&mAssetSaveConfirm, &closeWindow);
            if (mAssetSaveConfirm && closeWindow) {
                DispatchWindowClose(gApplicationInfo->AppWindow);
            }
            if (closeWindow) {
                mAssetSaveConfirmShow = false;
            }
        }
        // Feed the window hit-test (SDL/Win32 drag handling): when an ImGui item is hovered
        // the OS title-bar drag must yield so clicks reach the UI. Previously set on the render
        // thread in Renderer.cpp; that path is gone with the ImGui snapshot handoff, so refresh
        // it here on the Main thread (same thread the hit-test callback runs on).
        mApplicationInfo.AppWindow->ImGuiItemHovered = ImGui::IsAnyItemHovered();
        ImGui::Render();

        // Did this frame actually leave texture work pending (e.g. a lazily-baked glyph just created
        // a new atlas texture on an otherwise free-running frame)? If so its snapshot MUST be
        // uploaded by the render thread before it draws, so engage lockstep now even if we didn't
        // pre-engage. The mutation already happened during building, but a newly created texture is
        // a fresh object the prior snapshot doesn't reference, so engaging here still prevents the
        // render thread from drawing it un-uploaded.
        const bool texturesPending = ImGuiHasPendingTextureWork();
        if (texturesPending && !lockstepEngaged) {
            mApplicationInfo.AppRenderingManager->BeginImGuiLockstep();
            lockstepEngaged = true;
        }

        mApplicationInfo.AppRenderingManager->SubmitImGuiDrawData(ImGui::GetDrawData());

        if (lockstepEngaged) {
            // Hand this snapshot to the render thread for exactly one upload+draw pass, then re-scan:
            // uploaded textures are now OK, but a destroy can still be in flight (UnusedFrames-gated),
            // so stay engaged until everything is back at ImTextureStatus_OK, then resume the normal
            // lock-free free-running handoff.
            mApplicationInfo.AppRenderingManager->StepImGuiLockstep();
            mImGuiAtlasSettling = ImGuiHasPendingTextureWork();
            if (!mImGuiAtlasSettling) {
                mApplicationInfo.AppRenderingManager->EndImGuiLockstep();
            }
        } else {
            mImGuiAtlasSettling = false;
        }
    }
}

void Plu::PluEditor::OnRequestedGameExit()
{
    if (!mEditorAppContext->EditorScenesManager->IsInPIE()) return;
    mEditorAppContext->EditorScenesManager->ExitPIE();
    EndGame();
    gApplicationInfo->AppWindow->UpdateImGui = true;
    mApplicationInfo.AppWindow->SetCursorVisibility(true);
}

void Plu::PluEditor::OnRequestedWindowClose(TUsePointer<IWindow> window)
{
    if (mApplicationInfo.AppAssetManager->AreAnyAssetsDirty()) {
        mAssetSaveConfirmShow = true;
        return;
    }
    DispatchWindowClose(window);
}


