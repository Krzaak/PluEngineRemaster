//
// Created by Plutex on 1/13/26.
//

#include "SceneViewport.h"

#include "SceneStructurePanel.h"
#include "SceneViewportPanel.h"

void Plu::SceneViewport::OnClosed()
{
}

void Plu::SceneViewport::OnOpened()
{
	AddPanel(SceneStructurePanel::GetStaticClass(), false);
	AddPanel(SceneViewportPanel::GetStaticClass(), false);
}

void Plu::SceneViewport::OnPanelRegister()
{
	SceneStructurePanel* sceneDetailsPanel = GetPanelSlow<SceneStructurePanel>();
	SceneViewportPanel* sceneViewport = GetPanelSlow<SceneViewportPanel>();
	if (sceneDetailsPanel && sceneViewport)
	{
		ImGuiID dockspaceID = GetWindowDockID();

		ImGui::DockBuilderRemoveNode(dockspaceID);
		ImGui::DockBuilderAddNode(dockspaceID, ImGuiDockNodeFlags_DockSpace);
		ImGui::DockBuilderSetNodeSize(dockspaceID, ImGui::GetWindowSize());

		ImGuiID left, right;
		ImGui::DockBuilderSplitNode(dockspaceID, ImGuiDir_Left, 0.7f, &left, &right);
		ImGui::DockBuilderDockWindow(sceneDetailsPanel->GetPanelTitle().CStr(), right);
		ImGui::DockBuilderDockWindow(sceneViewport->GetPanelTitle().CStr(), left);
		ImGui::DockBuilderFinish(dockspaceID);
	}
}

void Plu::SceneViewport::OnUpdate(float deltaTime)
{
	if (BeginWindow()) {
		UpdatePanels(deltaTime);
	}
	EndWindow();
}
