//
// Created by Plutex on 1/13/26.
//

#include "MaterialDetailsPanel.h"

#include "MaterialViewport.h"

void Plu::MaterialInfoViewport::OnClosed()
{
}

void Plu::MaterialInfoViewport::OnOpened()
{
	AddPanel(MaterialDetailsPanel::GetStaticClass(), false);
}

void Plu::MaterialInfoViewport::OnPanelRegister()
{
	MaterialDetailsPanel* materialDetailsPanel = GetPanelSlow<MaterialDetailsPanel>();
	if (materialDetailsPanel) {
		ImGuiID dockspaceID = GetWindowDockID();

		ImGui::DockBuilderRemoveNode(dockspaceID);
		ImGui::DockBuilderAddNode(dockspaceID, ImGuiDockNodeFlags_DockSpace);
		ImGui::DockBuilderSetNodeSize(dockspaceID, ImGui::GetWindowSize());

		ImGui::DockBuilderDockWindow(materialDetailsPanel->GetPanelTitle().CStr(), dockspaceID);
		ImGui::DockBuilderFinish(dockspaceID);
	}
}

void Plu::MaterialInfoViewport::OnUpdate(float deltaTime)
{
	if (BeginWindow()) {
		UpdatePanels(deltaTime);
	}
	EndWindow();
}
