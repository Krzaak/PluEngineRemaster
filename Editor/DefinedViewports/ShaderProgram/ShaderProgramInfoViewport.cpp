//
// Created by Plutex on 6/11/26.
//

#include "ShaderProgramInfoViewport.h"

#include "ShaderProgramInfoPanel.h"

void Plu::ShaderProgramInfoViewport::OnClosed()
{
}

void Plu::ShaderProgramInfoViewport::OnOpened()
{
    AddPanel(ShaderProgramInfoPanel::GetStaticClass(), false);
}

void Plu::ShaderProgramInfoViewport::OnPanelRegister()
{
    ShaderProgramInfoPanel* shaderInfoPanel = GetPanelSlow<ShaderProgramInfoPanel>();
    if (shaderInfoPanel) {
        ImGuiID dockspaceID = GetWindowDockID();

        ImGui::DockBuilderRemoveNode(dockspaceID);
        ImGui::DockBuilderAddNode(dockspaceID, ImGuiDockNodeFlags_DockSpace);
        ImGui::DockBuilderSetNodeSize(dockspaceID, ImGui::GetWindowSize());

        ImGui::DockBuilderDockWindow(shaderInfoPanel->GetPanelTitle().CStr(), dockspaceID);
        ImGui::DockBuilderFinish(dockspaceID);
    }
}

void Plu::ShaderProgramInfoViewport::OnUpdate(float deltaTime)
{
    if (BeginWindow()) {
        UpdatePanels(deltaTime);
    }
    EndWindow();
}
