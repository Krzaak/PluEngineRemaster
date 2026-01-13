//
// Created by Plutex on 12/29/25.
//

#include "EditorApp.h"

#include "EditorAppContext.h"
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
#include "EditorViewports/EditorViewportManager.h"
#include "Managers/Assets/EditorAssetManager.h"
#include "Managers/Scene/EditorScenesManager.h"
#include "PluEngine/Engine.h"
#include "PluEngine/PluPaths.h"
#include "UI/IconsFontAwesome7.h"

extern void InitEditorReflection();

//For editor only
Plu::TUsePointer<Plu::EngineObjectManager> gEngineObjectManager;
Plu::EditorAppContext* gEditorAppContext;

Plu::PluEditor::PluEditor() : Application()
{
    mWindow = nullptr;
    mWindowClass = new ImGuiWindowClass();
    mWindowClass->DockNodeFlagsOverrideSet = ImGuiDockNodeFlags_NoSplit | ImGuiDockNodeFlags_NoCloseButton | ImGuiDockNodeFlags_NoWindowMenuButton;
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
    mWindow = Plu::IWindow::PlutexCreateWindow(props);
    const EngineObjectHandle rendererHandle = mObjectManager->CreateObject<Renderer>();
    mRenderer = mObjectManager->GetObjectAsOwner<Renderer>(rendererHandle);
    mEditorProjectManager = mObjectManager->CreateObject(EditorProjectManager::GetStaticClass());
    mEditorProjectManager->SetEditorAppContext(mEditorAppContext, &mApplicationInfo);
    mEditorAppContext->EditorAssetManager = mObjectManager->CreateObject(EditorAssetManager::GetStaticClass());
    mEditorAppContext->EditorScenesManager = mObjectManager->CreateObject(EditorScenesManager::GetStaticClass());
    mEditorAppContext->EditorViewportManager = mObjectManager->CreateObject(EditorViewportManager::GetStaticClass());
    const EngineObjectHandle panelManagerHandle = mObjectManager->CreateObject<EditorPanelManager>();
    mPanelManager = mObjectManager->GetObjectAsOwner<EditorPanelManager>(panelManagerHandle);
    PLU_INFO("Editor Init");
    gEditorAppContext = mEditorAppContext;
    gEngineObjectManager = mObjectManager;
    mRenderer->Init(this);
    mEditorAppContext->EditorPanelManager = mPanelManager;
    mEditorAppContext->EditorProjectManager =  mEditorProjectManager;
    mPanelManager->Init(&mApplicationInfo, mEditorAppContext);
    mPanelManager->Init();
}

void Plu::PluEditor::OnPostInit()
{
    ImGui::SetCurrentContext(Engine::GetEngine()->GetImGuiContext());
    //Fonts
    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->Clear();
    io.Fonts->AddFontDefault(); // Ładujemy standardową czcionkę
    PLU_TRACE("Default Font Added");

    static const ImWchar icons_ranges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
    ImFontConfig icons_config;
    icons_config.MergeMode = true; // KLUCZOWE: to łączy ikony z tekstem
    icons_config.PixelSnapH = true;

    // Ścieżka do pliku .ttf
    // String path = "";
    // path += PLU_FONTS_DIR;
    // path += "Font Awesome 7 Free-Regular-400.otf";

    std::string pathStd = PLU_FONTS_DIR;
    pathStd += "Font Awesome 7 Free-Regular-400.otf";
    std::string path2 = PLU_FONTS_DIR;
    path2 += "Font Awesome 7 Free-Solid-900.otf";

    io.Fonts->AddFontFromFileTTF(pathStd.c_str(), 13.0f, &icons_config, icons_ranges);
    io.Fonts->AddFontFromFileTTF(path2.c_str(), 13.0f, &icons_config, icons_ranges);
    PLU_TRACE("Font Awesome Added");

    mPanelManager->AddPanel<EngineStatsPanel>();
}

void Plu::PluEditor::OnShutdown()
{
    PLU_INFO("Editor Shutdown");
    mPanelManager->Shutdown();
    delete mEditorAppContext;
}

float Plu::PluEditor::DrawToolbarWindow(float toolbarHeight)
{
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);

    // Wymuszamy wysokość, ale musimy zadbać o to, by padding okna nie dodawał pustego miejsca
    ImGui::SetNextWindowSize(ImVec2(viewport->WorkSize.x, toolbarHeight));
    ImGui::SetNextWindowViewport(viewport->ID);

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar |
                             ImGuiWindowFlags_NoResize |
                             ImGuiWindowFlags_NoMove |
                             ImGuiWindowFlags_NoScrollbar |
                             ImGuiWindowFlags_NoSavedSettings |
                             ImGuiWindowFlags_NoDocking |
                             ImGuiWindowFlags_MenuBar;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

    // KLUCZ: Ustaw WindowPadding na 0, aby okno nie było większe niż pasek menu
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

    // Opcjonalnie: dostosuj FramePadding, aby precyzyjnie kontrolować wysokość wnętrza menu
    // Wysokość menu = FontSize + (FramePadding.y * 2)
    float targetFramePaddingY = (toolbarHeight - ImGui::GetFontSize()) / 2.0f;
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, targetFramePaddingY));

    ImGui::Begin("Toolbar", nullptr, flags);
    ImGui::BeginMenuBar();
    if (ImGui::BeginMenu("Project"))
    {
        ImGui::Text("Project Name");
        if (ImGui::MenuItem("New Project")) {
            mNewProjectPopup = true;
        }
        if (ImGui::MenuItem("Open Project")) {
            ImGuiFileDialog::Instance()->OpenDialog(
                "OpenProject",
                "Select project",
                PLU_PROJECT_EXT,
                IGFD::FileDialogConfig(".", "","", 1, IGFDUserDatas(), ImGuiFileDialogFlags_Modal)
            );
        }
        ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("View")) {
        if (ImGui::MenuItem("Editor Style")) {
            mPanelManager->AddPanel(EditorStylePanel::GetStaticClass());
        }
        ImGui::EndMenu();
    }
    if (mEditorProjectManager->IsAnyProjectOpen()) {
        if (ImGui::BeginMenu("Scene")) {
            if (ImGui::BeginMenu("Create New")) {
                std::string previewTemp;
                static String sceneName;
                if (ImGui::InputTextWithHint("Scene Name", "Hint", &previewTemp)) {
                    sceneName = previewTemp.c_str();
                }
                if (ImGui::Button("Create")) {
                    mEditorAppContext->EditorScenesManager->CreateNewScene(sceneName, mEditorProjectManager->GetProjectAssetsDirectory());
                }
                ImGui::EndMenu();
            }
            ImGui::EndMenu();
        }
    }
    ImGui::SameLine();
    constexpr float textWidth = 300;
    ImVec2 const buttonDimensions = ImVec2(toolbarHeight,toolbarHeight);
    float availableWidth = ImGui::GetContentRegionAvail().x;
    float viewportSize = ImGui::GetMainViewport()->Size.x;
    float xCursor = ImGui::GetCursorPosX();
    ImGui::SetCursorPosX(xCursor + availableWidth - textWidth - ImGui::GetStyle().FontSizeBase - buttonDimensions.x * 4);
    if (mEditorProjectManager->IsAnyProjectOpen()) {
        ImGui::TextAligned(1, textWidth, String::FromWide(mEditorProjectManager->GetProjectName().CStr()).CStr());
    } else {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1,0.6,0.6,1));
        ImGui::TextAligned(1, textWidth, "No Project Open!");
        ImGui::PopStyleColor();
    }
    ImGui::SetCursorPosX(xCursor + availableWidth - buttonDimensions.x * 4);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1,1,1,0.3));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1,1,1,0.8));
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1,0,0,0));
    if (ImGui::Button(ICON_FA_MINUS "",buttonDimensions))
    {
        mWindow->Minimize();
    }
    if (ImGui::Button(ICON_FA_EXPAND "",buttonDimensions))
    {
        mWindow->Maximize();
    }
    ImGui::PopStyleColor(3);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.5,0,0,1));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1,0,0,1));
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1,0,0,0));
    if (ImGui::Button(ICON_FA_XMARK "",buttonDimensions))
    {
        mWindow->Close();
    }
    ImGui::PopStyleColor(3);
    ImGui::PopStyleVar(2);
    float h = ImGui::GetWindowHeight();
    ImGui::EndMenuBar();
    ImGui::End();
    ImGui::PopStyleVar(3);
    return h;
}

void Plu::PluEditor::DrawMainEngineWindow()
{
    static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_NoDockingSplit;
    ImGuiStyle& style = ImGui::GetStyle();
    float toolbarHeight = style.FontSizeBase * 1.3;
    toolbarHeight = toolbarHeight + 12;

    float realToolbarHeight = DrawToolbarWindow(toolbarHeight);

    const ImGuiViewport* viewport = ImGui::GetMainViewport();

    ImGui::SetNextWindowPos(ImVec2(viewport->WorkPos.x, viewport->WorkPos.y + realToolbarHeight));
    ImGui::SetNextWindowSize(ImVec2(viewport->WorkSize.x, viewport->WorkSize.y - realToolbarHeight));
    ImGui::SetNextWindowViewport(viewport->ID);

    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking |
                                    ImGuiWindowFlags_NoTitleBar |
                                    ImGuiWindowFlags_NoCollapse |
                                    ImGuiWindowFlags_NoResize |
                                    ImGuiWindowFlags_NoMove |
                                    ImGuiWindowFlags_NoBringToFrontOnFocus |
                                    ImGuiWindowFlags_NoNavFocus;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

    ImGui::Begin("PluEngine", nullptr, window_flags);
    ImGui::PopStyleVar(3);

    ImGuiIO& io = ImGui::GetIO();
    //Here we do dockspace for asset Viewports
    if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
    {
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 10));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2(10, 4));
        ImGui::PushStyleVar(ImGuiStyleVar_TabRounding, 8.0f);
        mWindowClass->ClassId = ImGui::GetID("EditorViewport");
        mDockspaceId = ImGui::GetID("AssetDockspace");
        ImGui::DockSpace(mDockspaceId, ImVec2(0.0f, 0.0f), dockspace_flags, mWindowClass);
        ImGui::PopStyleVar(3);
    }
    ImGui::End();
}

void Plu::PluEditor::OnImGuiRender()
{
    DrawMainEngineWindow();
    if (mNewProjectPopup) ImGui::OpenPopup("New Project");
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
            mNewProjectPopup = false;
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

    mPanelManager->OnUpdate(0);
    mEditorAppContext->EditorViewportManager->Tick(0);
}


