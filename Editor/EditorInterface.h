//
// Created by Plutex on 2026-02-20.
//

#ifndef PLUENGINE_EDITORINTERFACE_H
#define PLUENGINE_EDITORINTERFACE_H
#include <filesystem>

#include "imgui.h"
#include "imgui_internal.h"
#include "imgui/misc/cpp/imgui_stdlib.h"
#include "ImGuiFileDialog.h"
#include "DefinedPanels/EngineClassTreePanel.h"
#include "DefinedPanels/EngineStatsPanel.h"
#include "DefinedPanels/Style/EditorStylePanel.h"
#include "Managers/Project/EditorProjectManager.h"
#include "PluEngine/PluPaths.h"
#include "PluEngine/Managers/DiskManager.h"
#include "UI/IconsFontAwesome7.h"
#include "Managers/Python/EditorPythonManager.h"
#include "Panels/EditorPanelManager.h"
#include "PluEngine/GameCore/GameClient.h"
#include "EditorApp.h"
#include "PluEngine/Window/Window.h"
#include "EditorAppContext.h"
#include "DefinedPanels/EditorSettingsPanel.h"
#include "DefinedPanels/Project/ProjectSettings/ProjectSettingsPanel.h"
#include "Managers/Shaders/EditorShaderManager.h"
#include "PluEngine/PluUtils.h"
#include "nlohmann/json.hpp"
#include "nlohmann/json_fwd.hpp"
#include "PluEngine/Assets/EngineAssetManager.h"
#include "PluEngine/Scenes/SceneManager.h"
#include "PluEngine/Window/WindowManager.h"

extern Plu::TUsePointer<Plu::EngineObjectManager> gEngineObjectManager;
extern Plu::EditorAppContext* gEditorAppContext;
extern Plu::ApplicationInfo* gApplicationInfo;
extern Plu::PluEditor* gPluEditor;

namespace Plu
{
    inline ImGuiWindowClass* gWindowClass;
    inline ImGuiID gDockspaceId;

    inline float DrawToolbarWindow(float toolbarHeight, int windowID)
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
        ImVec2 sizeForPlayButton = ImGui::GetContentRegionAvail();
        ImGui::BeginMenuBar();
        if (ImGui::BeginMenu("Project"))
        {
            if (ImGui::MenuItem("New Project")) {
                gEditorAppContext->NewProjectPopup = true;
            }
            if (ImGui::MenuItem("Open Project")) {
                ImGuiFileDialog::Instance()->OpenDialog(
                    "OpenProject",
                    "Select project",
                    PLU_PROJECT_EXT,
                    IGFD::FileDialogConfig(".", "","", 1, IGFDUserDatas(), ImGuiFileDialogFlags_Modal)
                );
            }
            if (ImGui::BeginMenu("Recent Projects")) {
                if (!std::filesystem::exists(EditorProjectManager::GetRecentProjectsJSONPath().CStr())) {
                    ImGui::Text("No recent Projects!");
                } else {
                    nlohmann::json json = DiskManager::LoadJson(EditorProjectManager::GetRecentProjectsJSONPath());
                    for (const auto& project : json["projects"]) {
                        Path projectPath = project.get<std::string>().c_str();
                        if (ImGui::Selectable(projectPath.GetStem().CStr())) {
                            gEditorAppContext->EditorProjectManager->OpenProject(StringW::FromNarrow(projectPath.CStr()));
                        }
                    }
                }
                ImGui::EndMenu();
            }
            if (gEditorAppContext->EditorProjectManager->IsAnyProjectOpen()) {
                ImGui::Separator();
                bool disabled = gEditorAppContext->EditorProjectManager->GetProjectFileVersion() < 0.1;
                ImGui::BeginDisabled(disabled);
                if (ImGui::MenuItem("Project Settings")) {
                    gEditorAppContext->EditorPanelManager->AddPanel(ProjectSettingsPanel::GetStaticClass());
                }
                if (disabled) {
                    ImGui::SetItemTooltip("Outdated project file! Settings not supported!");
                }
                ImGui::EndDisabled();
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("View")) {
            if (ImGui::MenuItem("Editor Style")) {
                gEditorAppContext->EditorPanelManager->AddPanel(EditorStylePanel::GetStaticClass());
            }
            if (ImGui::MenuItem("Editor Settings")) {
                gEditorAppContext->EditorPanelManager->AddPanel<EditorSettingsPanel>();
            }
            if (ImGui::BeginMenu("Open Any Panel"))
            {
                static DynamicArray<TypeInfo*> panelTypes;
                if (panelTypes.IsEmpty()) {
                    for (auto type : *TypeRegistry::GetInstance()->GetTypeMap()) {
                        if (type.second->IsDerivedOf(EditorPanel::GetStaticClass())) {
                            panelTypes.PushBack(type.second);
                        }
                    }
                }
                for (auto type : panelTypes) {
                    if (ImGui::Button(type->TypeName.CStr()))
                    {
                        gEditorAppContext->EditorPanelManager->AddPanel(type);
                    }
                }
                ImGui::EndMenu();
            }
            ImGui::EndMenu();
        }
        static bool triedLoadingPythonScriptDirs = false;
        static PathW workDir = L"SELECT WORKDIR!";
        static PathW scriptPath = L"SELECT SCRIPT!";
        static String args;
        if (!triedLoadingPythonScriptDirs) {
            triedLoadingPythonScriptDirs = true;
            PathW exePath = GetExePath().GetParentPath();
            exePath /= L"ScriptExeInfo.json";
            std::optional<JSON> json = DiskManager::LoadJson(exePath);
            if (json.has_value()) {
                JSON j = json.value();
                JSON wd = j["workDir"];
                workDir = StringW::FromNarrow(wd.get<std::string>().c_str());
                scriptPath = StringW::FromNarrow(j["scriptDir"].get<std::string>().c_str());
                args = j["args"].get<std::string>().c_str();
            }
        }
        if (ImGui::BeginMenu("Scripts")) {
            if (ImGui::BeginMenu("Python Script")) {
                ImGui::Text("Script:");
                ImGui::Text(scriptPath.ToString().ToNarrow());
                ImGui::SameLine();
                if (ImGui::Button(ICON_FA_FOLDER "##Script")) {
                    ImGuiFileDialog::Instance()->OpenDialog(
                        "Script",
                        "Select .py script",
                        ".py",
                        IGFD::FileDialogConfig(".", "","", 1, IGFDUserDatas(), ImGuiFileDialogFlags_Modal)
                    );
                }

                ImGui::Text("Work Dir:");
                ImGui::Text(workDir.ToString().ToNarrow());
                ImGui::SameLine();
                if (ImGui::Button(ICON_FA_FOLDER "##WorkDir")) {

                }

                ImGui::Text("Args:");
                std::string tmp = args.CStr();
                if (ImGui::InputText("##", &tmp)) {
                    args = tmp.c_str();
                }
                if (ImGui::Button(ICON_FA_ROCKET "Run Script")) {
                    PathW exePath = GetExePath().GetParentPath();
                    exePath /= L"ScriptExeInfo.json";
                    JSON json = {
                        {"scriptDir", scriptPath.CStr()},
                        {"workDir", workDir.CStr()},
                        {"args", args.CStr()}
                    };
                    DiskManager::SaveJson(exePath.ToString(), json);
                    gEditorAppContext->EditorPythonManager->RunScript(scriptPath, workDir, args);
                }
                ImGui::EndMenu();
            }
            if (ImGui::MenuItem("Clear Python Modules (Hotreload)")) {
                gEditorAppContext->EditorPythonManager->ClearProjectScripts();
                gEditorAppContext->EditorPythonManager->RunProjectScripts();
            }
            ImGui::EndMenu();
        }
        if (ImGuiFileDialog::Instance()->Display("Script"))
        {
            if (ImGuiFileDialog::Instance()->IsOk())
            {
                std::string filePath = ImGuiFileDialog::Instance()->GetFilePathName();
                scriptPath = StringW::FromNarrow(filePath.c_str());
                workDir = scriptPath.GetParentPath();
                PathW exePath = GetExePath().GetParentPath();
                exePath /= L"ScriptExeInfo.json";
                JSON json = {
                    {"scriptDir", scriptPath.CStr()},
                    {"workDir", workDir.CStr()},
                    {"args", args.CStr()}
                };
                DiskManager::SaveJson(exePath.ToString(), json);
            }

            ImGuiFileDialog::Instance()->Close();
        }
        if (gEditorAppContext->EditorProjectManager->IsAnyProjectOpen()) {
            if (ImGui::BeginMenu("Scene")) {
                if (ImGui::BeginMenu("Create New")) {
                    std::string previewTemp;
                    static String sceneName;
                    if (ImGui::InputTextWithHint("Scene Name", "Hint", &previewTemp)) {
                        sceneName = previewTemp.c_str();
                    }
                    if (ImGui::Button("Create")) {
                        //gEditorAppContext->EditorScenesManager->CreateNewScene(sceneName, gEditorAppContext->EditorProjectManager->GetProjectAssetsDirectory());
                    }
                    ImGui::EndMenu();
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Build")) {
                if (ImGui::MenuItem("Build Project")) {
                    gEditorAppContext->EditorAssetManager->PrepareAssetsForDistribution(gEditorAppContext->EditorProjectManager->GetProjectCacheDirectory().ToString().ToNarrow());
                    gEditorAppContext->EditorShaderManager->PrepareShaderCodesForDistribution(gEditorAppContext->EditorProjectManager->GetProjectCacheDirectory().ToString().ToNarrow());
                    gEditorAppContext->EditorProjectManager->BuildProjectForShipment(gEditorAppContext->EditorProjectManager->GetProjectCacheDirectory());
                    PLU_INFO("Project is ready for shipment!");
                }
                ImGui::EndMenu();
            }
        }
        ImGui::SameLine();
        ImVec2 const buttonDimensions = ImVec2(toolbarHeight,toolbarHeight);
        if (gEditorAppContext->EditorProjectManager->IsAnyProjectOpen() && gEditorAppContext->EditorScenesManager->IsAnySceneOpen()) {
            ImGui::SetCursorPosX((sizeForPlayButton.x / 2) - (toolbarHeight / 2));
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1,1,1,0.3));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1,1,1,0.8));
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1,0,0,0));
            if (gEditorAppContext->EditorScenesManager->IsInPIE()) {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
                if (ImGui::Button(ICON_FA_X "", buttonDimensions)) {
                    gEditorAppContext->EditorScenesManager->ExitPIE();
                    gPluEditor->EndGame();
                    gApplicationInfo->AppWindow->SetCursorVisibility(true);
                }
                ImGui::PopStyleColor();
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.8f, 0.0f, 1.0f));
                if (!gPluEditor->mUpdateInput) {
                    if (ImGui::Button(ICON_FA_GAMEPAD "")) {
                        gPluEditor->mUpdateInput = true;
                        gApplicationInfo->AppWindow->SetCursorVisibility(false);
                        gApplicationInfo->AppWindow->UpdateImGui = false;
                    }
                }
            } else {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 1.0f, 0.0f, 1.0f));
                if (ImGui::Button(ICON_FA_PLAY "", buttonDimensions)) {
                    gPluEditor->StartGame();
                    if (gEditorAppContext->EditorScenesManager->EnterPIE()) {
                        gApplicationInfo->Client->JoinGameLocally();
                        gApplicationInfo->AppWindow->UpdateImGui = false;
                    } else {
                        gPluEditor->EndGame();
                    }
                }
                if (ImGui::BeginPopupContextItem())
                {
                    if (ImGui::Button(ICON_FA_CIRCLE_PLAY " Play In FullScreen")) {
                        gPluEditor->StartGame();
                        if (gEditorAppContext->EditorScenesManager->EnterPIE()) {
                            gApplicationInfo->Client->JoinGameLocally();
                            gEditorAppContext->PIEFullscreen = true;
                            gApplicationInfo->AppWindow->SetCursorVisibility(false);
                        } else {
                            gPluEditor->EndGame();
                        }
                    }
                    ImGui::Separator();
                    if (ImGui::Button("Close"))
                        ImGui::CloseCurrentPopup();
                    ImGui::EndPopup();
                }
            }
            ImGui::PopStyleColor(4);
            ImGui::PopStyleVar();
        }
        constexpr float textWidth = 400;
        float availableWidth = ImGui::GetContentRegionAvail().x;
        float xCursor = ImGui::GetCursorPosX();
        ImGui::SetCursorPosX(xCursor + availableWidth - textWidth - ImGui::GetStyle().FontSizeBase - buttonDimensions.x * 4);
        if (gEditorAppContext->EditorProjectManager->IsAnyProjectOpen()) {
            if (gEditorAppContext->EditorScenesManager->IsAnySceneOpen()) {
                String msg = String::FromWide(gEditorAppContext->EditorProjectManager->GetProjectName().CStr());
                msg += " > ";
                msg += gEditorAppContext->EditorScenesManager->GetCurrentWorldName();
                ImGui::TextAligned(1, textWidth, msg.CStr());
            } else {
                ImGui::TextAligned(1, textWidth, String::FromWide(gEditorAppContext->EditorProjectManager->GetProjectName().CStr()).CStr());
            }
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
            gApplicationInfo->AppWindowsManager->GetWindowAt(windowID)->Minimize();
        }
        if (ImGui::Button(ICON_FA_EXPAND "",buttonDimensions))
        {
            gApplicationInfo->AppWindowsManager->GetWindowAt(windowID)->Maximize();
        }
        ImGui::PopStyleColor(3);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.5,0,0,1));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1,0,0,1));
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1,0,0,0));
        if (ImGui::Button(ICON_FA_XMARK "",buttonDimensions))
        {
            gApplicationInfo->AppWindowsManager->CloseWindow(windowID);
        }
        ImGui::PopStyleColor(3);
        ImGui::PopStyleVar(2);
        float h = ImGui::GetWindowHeight();
        ImGui::EndMenuBar();
        ImGui::End();
        ImGui::PopStyleVar(3);
        return h;
    }

    inline void DrawMainEngineWindow(int windowID)
    {
        static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_NoDockingSplit;
        ImGuiStyle& style = ImGui::GetStyle();
        float toolbarHeight = style.FontSizeBase * 1.3;
        toolbarHeight = toolbarHeight + 12;

        float realToolbarHeight = DrawToolbarWindow(toolbarHeight, windowID);

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

        ImGui::Begin(("PluEngine" + String::FromInt(windowID)).CStr(), nullptr, window_flags);
        ImGui::PopStyleVar(3);

        ImGuiIO& io = ImGui::GetIO();
        //Here we do dockspace for asset Viewports
        if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
        {
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 10));
            ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2(10, 4));
            ImGui::PushStyleVar(ImGuiStyleVar_TabRounding, 8.0f);
            gWindowClass->ClassId = ImGui::GetID(("EditorViewport" + String::FromInt(windowID)).CStr());
            gDockspaceId = ImGui::GetID(("AssetDockspace" + String::FromInt(windowID)).CStr());
            ImGui::DockSpace(gDockspaceId, ImVec2(0.0f, 0.0f), dockspace_flags, gWindowClass);
            ImGui::PopStyleVar(3);
        }
        ImGui::End();
    }
}

#endif //PLUENGINE_EDITORINTERFACE_H