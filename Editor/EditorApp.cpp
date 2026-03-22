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
#include "PluEngine/Objects/EngineObjectHandle.h"
#include "PluEngine/Objects/EngineObjectManager.h"
#include "PluEngine/Window/Window.h"
#include "PluEngine/Renderer/Renderer.h"
#include "Panels/EditorPanelManager.h"
#include "imgui/misc/cpp/imgui_stdlib.h"

#include "ImGuiFileDialog.h"
#include "json_fwd.hpp"
#include "DefinedPanels/EngineClassTreePanel.h"
#include "EditorViewports/EditorViewportManager.h"
#include "EditorWindows/EditorWindowsManager.h"
#include "Managers/Assets/EditorAssetManager.h"
#include "Managers/Python/EditorPythonManager.h"
#include "Managers/Scene/EditorScenesManager.h"
#include "Managers/Shaders/EditorShaderManager.h"
#include "PluEngine/Engine.h"
#include "PluEngine/PluPaths.h"
#include "PluEngine/GameCore/GameClient.h"
#include "PluEngine/Input/InputManager.h"
#include "PluEngine/Managers/DiskManager.h"
#include "PluEngine/Window/WindowManager.h"
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

void Plu::PluEditor::OnInit()
{
    InitEditorReflection();
    mEditorAppContext = new EditorAppContext;
    Plu::WindowProperties props;
    props.Title = "Plu Editor";
    mApplicationInfo.AppWindowsManager->AddWindow(props);
    const EngineObjectHandle rendererHandle = mObjectManager->CreateObject<Renderer>();
    mApplicationInfo.AppRenderer = mObjectManager->GetObjectAsOwner<Renderer>(rendererHandle);
    mEditorProjectManager = mObjectManager->CreateObject(EditorProjectManager::GetStaticClass());
    mEditorProjectManager->SetEditorAppContext(mEditorAppContext, &mApplicationInfo);
    mEditorAppContext->EditorPythonManager = mObjectManager->CreateObject(EditorPythonManager::GetStaticClass());
    mEditorAppContext->EditorAssetManager = mObjectManager->CreateObject(EditorAssetManager::GetStaticClass());
    mEditorAppContext->EditorScenesManager = mObjectManager->CreateObject(EditorScenesManager::GetStaticClass());
    mEditorAppContext->EditorViewportManager = mObjectManager->CreateObject(EditorViewportManager::GetStaticClass());
    mEditorAppContext->EditorShaderManager = mObjectManager->CreateObject(EditorShaderManager::GetStaticClass());
    const EngineObjectHandle panelManagerHandle = mObjectManager->CreateObject<EditorPanelManager>();
    mPanelManager = mObjectManager->GetObjectAsOwner<EditorPanelManager>(panelManagerHandle);
    PLU_INFO("Editor Init");
    gEditorAppContext = mEditorAppContext;
    gEngineObjectManager = mObjectManager;
    gApplicationInfo = &mApplicationInfo;
    mEditorAppContext->EditorShaderManager->PreInit(mEditorProjectManager);
    mApplicationInfo.AppRenderer->Init(this);
    mEditorAppContext->EditorPanelManager = mPanelManager;
    mEditorAppContext->EditorProjectManager =  mEditorProjectManager;
    mPanelManager->Init(&mApplicationInfo, mEditorAppContext, &gDockspaceId);
    mPanelManager->Init();
    mApplicationInfo.AppScenesManager = mEditorAppContext->EditorScenesManager;
    mApplicationInfo.AppShaderManager = mEditorAppContext->EditorShaderManager;
    mApplicationInfo.AppAssetManager = mEditorAppContext->EditorAssetManager;
    mEditorAppContext->EditorWindowsManager = mObjectManager->CreateObject(EditorWindowsManager::GetStaticClass());

    mApplicationInfo.AppInputManager = mObjectManager->CreateObject(InputManager::GetStaticClass());
}

void Plu::PluEditor::OnPostInit()
{
    ImGui::SetCurrentContext(mApplicationInfo.AppWindow->GetImGuiContext());
    //Fonts
    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->Clear();
    io.Fonts->AddFontDefault(); // Ładujemy standardową czcionkę
    PLU_TRACE("Default Font Added");

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

    io.Fonts->AddFontFromFileTTF(pathStd.c_str(), 13.0f, &icons_config, icons_ranges);
    io.Fonts->AddFontFromFileTTF(path2.c_str(), 13.0f, &icons_config, icons_ranges);
    PLU_TRACE("Font Awesome Added");
}

void Plu::PluEditor::OnShutdown()
{
    PLU_INFO("Editor Shutdown");
    mEditorAppContext->EditorScenesManager->ExitPIE();
    EndGame();
    mEditorAppContext->EditorScenesManager->Shutdown();
    mEditorAppContext->EditorAssetManager->Shutdown();
    mPanelManager->Shutdown();
    mEditorAppContext->EditorViewportManager->Shutdown();
    mObjectManager->DestroyObject(*mEditorAppContext->EditorViewportManager->GetEngineObjectHandle());
    mObjectManager->DestroyObject(*mEditorAppContext->EditorScenesManager->GetEngineObjectHandle());
    mObjectManager->DestroyObject(*mEditorAppContext->EditorAssetManager->GetEngineObjectHandle());
    mObjectManager->DestroyObject(*mEditorAppContext->EditorPanelManager->GetEngineObjectHandle());
    mObjectManager->DestroyObject(*mEditorAppContext->EditorProjectManager->GetEngineObjectHandle());
    delete mEditorAppContext;
}

void Plu::PluEditor::OnImGuiRender()
{
    ImGui::SetCurrentContext(mApplicationInfo.AppWindow->GetImGuiContext());
    if (mEditorAppContext->PIEFullscreen) {
        mApplicationInfo.AppRenderer->GetMainBuffer()->BlitTo(nullptr);
        if (mApplicationInfo.AppInputManager->GetInputBackend()->GetKeyboard().IsDown(Key::Escape)) {
            mEditorAppContext->EditorScenesManager->ExitPIE();
            EndGame();
            mEditorAppContext->PIEFullscreen = false;
            IWindow::SetCursorVisibility(true);
        }
        return;
    }
    if (gEditorAppContext->EditorScenesManager->IsInPIE()) {
        if (mApplicationInfo.AppInputManager->GetInputBackend()->GetKeyboard().IsDown(Key::F8)) {
            mUpdateInput = false;
            IWindow::SetCursorVisibility(true);
            gApplicationInfo->AppWindow->UpdateImGui = true;
        }
        if (mApplicationInfo.AppInputManager->GetInputBackend()->GetKeyboard().IsDown(Key::Escape)) {
            mEditorAppContext->EditorScenesManager->ExitPIE();
            EndGame();
            IWindow::SetCursorVisibility(true);
            gApplicationInfo->AppWindow->UpdateImGui = true;
        }
    }
    DrawMainEngineWindow(0);
    if (mEditorAppContext->NewProjectPopup) ImGui::OpenPopup("New Project");
    if (ImGui::BeginPopupModal("New Project")) {
        if (ImGui::Button("Select Path")) {
            ImGuiFileDialog::Instance()->OpenDialog(
                "NewProject",
                "Wybierz katalog",
                nullptr,
                IGFD::FileDialogConfig(".", "","", 1, IGFDUserDatas(), ImGuiFileDialogFlags_Modal)
            );
        }
        static String pathToNewProject;
        static String projectName;
        static bool firstTime;
        if (firstTime) {
            projectName.Reserve(30);
            firstTime = false;
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
        if (ImGuiFileDialog::Instance()->Display("NewProject"))
        {
            if (ImGuiFileDialog::Instance()->IsOk())
            {
                std::string filePath = ImGuiFileDialog::Instance()->GetCurrentPath();
                pathToNewProject = filePath.c_str();
            }

            ImGuiFileDialog::Instance()->Close();
        }
        ImGui::EndPopup();
    }

    if (ImGuiFileDialog::Instance()->Display("OpenProject"))
    {
        if (ImGuiFileDialog::Instance()->IsOk())
        {
            std::string filePath = ImGuiFileDialog::Instance()->GetFilePathName();
            mEditorProjectManager->OpenProject(StringW::FromNarrow(filePath.c_str()));
        }

        ImGuiFileDialog::Instance()->Close();
    }

    mPanelManager->OnUpdate(0, 0);
    mEditorAppContext->EditorViewportManager->Tick(0);
}

void Plu::PluEditor::OnImGuiRenderEX(UInt64 windowID)
{
    ImGui::SetCurrentContext(Engine::GetEngine()->GetImGuiContext());
    DrawMainEngineWindow(static_cast<int>(windowID));
    mPanelManager->OnUpdate(0, static_cast<int>(windowID));
}

void Plu::PluEditor::OnTick(float deltaTime)
{
    mEditorAppContext->EditorWindowsManager->OnUpdate(deltaTime);
}


