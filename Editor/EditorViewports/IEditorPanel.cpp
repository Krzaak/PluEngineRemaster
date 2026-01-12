#include "IEditorPanel.h"

#include <imgui/imgui_internal.h>

#include <utility>

#include "EditorViewportManager.h"

bool Plu::IEditorPanel::BeginPanel()
{
    if (!ImGui::GetWindowDockNode()) {
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.4f, 0.4f, 0.4f, 1));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
    }
    ImGui::SetNextWindowClass(GetParentViewport()->GetViewportWindowClass());
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse;
    bool open = ImGui::Begin(GetPanelTitle().CStr(), mCanClose ? &mIsOpen : nullptr, flags);
    //TODO
    //if (ImGui::IsWindowHovered()) mpEditorState->ViewportManager->SetHoveredPanel(this);
    if (!mIsOpen) return false;
    return open;
}

void Plu::IEditorPanel::EndPanel()
{
    ImGui::End();
    if (!ImGui::GetWindowDockNode()) {
        ImGui::PopStyleVar();
        ImGui::PopStyleColor();
    }    
}

Plu::IEditorPanel::IEditorPanel()
= default;

void Plu::IEditorPanel::Initialize(TUsePointer<IEditorViewport> viewport, bool canClose)
{
    mEditorViewport = std::move(viewport);
    mCanClose = canClose;
}

Plu::String Plu::IEditorPanel::GetPanelTitle()
{
    return GetPanelName() + "##" + GetParentViewport()->GetWindowName();
}
