//
// Created by Plutex on 12/29/25.
//

#include "EditorApp.h"

#include "DefinedPanels/EngineStatsPanel.h"
#include "DefinedPanels/Style/EditorStylePanel.h"
#include "Managers/Project/EditorProjectManager.h"
#include "PluEngine/Log.h"
#include "PluEngine/Objects/EngineObjectHandle.h"
#include "PluEngine/Objects/EngineObjectManager.h"
#include "PluEngine/Window/Window.h"
#include "PluEngine/Renderer/Renderer.h"
#include "Panels/EditorPanelManager.h"

extern void InitEditorReflection();

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
    Plu::WindowProperties props;
    props.Title = "Plu Editor";
    mWindow = Plu::IWindow::PlutexCreateWindow(props);
    const EngineObjectHandle rendererHandle = mObjectManager->CreateObject<Renderer>();
    mRenderer = mObjectManager->GetObjectAsOwner<Renderer>(rendererHandle);
    mEditorProjectManager = mObjectManager->CreateObject(EditorProjectManager::GetStaticClass());
    const EngineObjectHandle panelManagerHandle = mObjectManager->CreateObject<EditorPanelManager>();
    mPanelManager = mObjectManager->GetObjectAsOwner<EditorPanelManager>(panelManagerHandle);
    PLU_INFO("Editor Init");
    mRenderer->Init(this);
    mPanelManager->Init(&mApplicationInfo);
    mPanelManager->Init();
}

void Plu::PluEditor::OnPostInit()
{
    mPanelManager->AddPanel<EngineStatsPanel>();
}

void Plu::PluEditor::OnShutdown()
{
    PLU_INFO("Editor Shutdown");
    mPanelManager->Shutdown();
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
            mEditorProjectManager->CreateNewProject();
        }
        ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("View")) {
        if (ImGui::MenuItem("Editor Style")) {
            mPanelManager->AddPanel(EditorStylePanel::GetStaticClass());
        }
        ImGui::EndMenu();
    }
    ImGui::SameLine();
    constexpr float textWidth = 300;
    ImVec2 const buttonDimensions = ImVec2(toolbarHeight,toolbarHeight);
    float availableWidth = ImGui::GetContentRegionAvail().x;
    float viewportSize = ImGui::GetMainViewport()->Size.x;
    float xCursor = ImGui::GetCursorPosX();
    ImGui::SetCursorPosX(xCursor + availableWidth - textWidth - ImGui::GetStyle().FontSizeBase - buttonDimensions.x * 4);
    if (mEditorProjectManager->IsAnyProjectOpen()) {
        MaxUInt32 len = wcstombs(nullptr, mEditorProjectManager->GetProjectName().CStr(), 0);
        String result(nullptr,len);
        wcstombs(&result[0], mEditorProjectManager->GetProjectName().CStr(), len);
        //TODO
        ImGui::TextAligned(1, textWidth, result.CStr());
    } else {
        ImGui::TextAligned(1, textWidth, "No Project Open!");
    }
    ImGui::SetCursorPosX(xCursor + availableWidth - buttonDimensions.x * 4);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);
    if (ImGui::Button("-",buttonDimensions))
    {
        mWindow->Minimize();
    }
    if (ImGui::Button("[]",buttonDimensions))
    {
        mWindow->Maximize();
    }
    if (ImGui::Button("X",buttonDimensions))
    {
        mWindow->Close();
    }
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
    mPanelManager->OnUpdate(0);
}


