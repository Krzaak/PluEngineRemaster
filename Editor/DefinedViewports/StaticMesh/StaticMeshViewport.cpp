//
// Created by Plutex on 1/13/26.
//

#include "StaticMeshViewport.h"

#include "StaticMeshDetailsPanel.h"

void Plu::StaticMeshViewport::OnClosed()
{
}

void Plu::StaticMeshViewport::OnOpened()
{
	AddPanel(StaticMeshDetailsPanel::GetStaticClass(), false);
}

void Plu::StaticMeshViewport::OnPanelRegister()
{
	StaticMeshDetailsPanel* staticMeshDetailsPanel = GetPanelSlow<StaticMeshDetailsPanel>();
	if (staticMeshDetailsPanel) {
		ImGuiID dockspaceID = GetWindowDockID();

		ImGui::DockBuilderRemoveNode(dockspaceID);
		ImGui::DockBuilderAddNode(dockspaceID, ImGuiDockNodeFlags_DockSpace);
		ImGui::DockBuilderSetNodeSize(dockspaceID, ImGui::GetWindowSize());

		ImGui::DockBuilderDockWindow(staticMeshDetailsPanel->GetPanelTitle().CStr(), dockspaceID);
		ImGui::DockBuilderFinish(dockspaceID);
	}
}

void Plu::StaticMeshViewport::OnUpdate(float deltaTime)
{
	if (BeginWindow()) {
		UpdatePanels(deltaTime);
	}
	EndWindow();
}
