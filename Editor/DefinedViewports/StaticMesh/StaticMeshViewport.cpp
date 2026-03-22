//
// Created by Plutex on 1/13/26.
//

#include "StaticMeshViewport.h"

#include "StaticMeshDetailsPanel.h"
#include "StaticMeshViewportPanel.h"

void Plu::StaticMeshViewport::OnClosed()
{
}

void Plu::StaticMeshViewport::OnOpened()
{
	AddPanel(StaticMeshDetailsPanel::GetStaticClass(), false);
	AddPanel(StaticMeshViewportPanel::GetStaticClass(), false);
}

void Plu::StaticMeshViewport::OnPanelRegister()
{
	StaticMeshDetailsPanel* staticMeshDetailsPanel = GetPanelSlow<StaticMeshDetailsPanel>();
	StaticMeshViewportPanel* staticMeshViewportPanel = GetPanelSlow<StaticMeshViewportPanel>();
	if (staticMeshDetailsPanel) {
		ImGuiID dockspaceID = GetWindowDockID();

		ImGui::DockBuilderRemoveNode(dockspaceID);
		ImGui::DockBuilderAddNode(dockspaceID, ImGuiDockNodeFlags_DockSpace);
		ImGui::DockBuilderSetNodeSize(dockspaceID, ImGui::GetWindowSize());

		ImGuiID right, left;
		ImGui::DockBuilderSplitNode(dockspaceID, ImGuiDir_Left, 0.7f, &left, &right);

		ImGui::DockBuilderDockWindow(staticMeshDetailsPanel->GetPanelTitle().CStr(), right);
		ImGui::DockBuilderDockWindow(staticMeshViewportPanel->GetPanelTitle().CStr(), left);
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
