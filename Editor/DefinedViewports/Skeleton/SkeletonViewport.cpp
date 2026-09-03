//
// Created by Plutex on 7/6/26.
//

#include "SkeletonViewport.h"

#include "SkeletonHierarchyPanel.h"
#include "SkeletonViewportPanel.h"
#include "PluEngine/Core/Objects/EngineObjectManager.h"

void Plu::SkeletonViewport::OnInit()
{
	// Skeleton is a binary asset, so Ctrl+S / "Save All" are off unless asked for explicitly —
	// attach point edits are asset data and have to be saveable from here.
	SetCanBeSaved(true);
}

void Plu::SkeletonViewport::OnClosed()
{
}

void Plu::SkeletonViewport::OnOpened()
{
	AddPanel(SkeletonHierarchyPanel::GetStaticClass(), false);
	AddPanel(SkeletonViewportPanel::GetStaticClass(), false);
}

void Plu::SkeletonViewport::OnPanelRegister()
{
	SkeletonHierarchyPanel* hierarchyPanel = GetPanelSlow<SkeletonHierarchyPanel>();
	SkeletonViewportPanel* viewportPanel = GetPanelSlow<SkeletonViewportPanel>();
	if (hierarchyPanel && viewportPanel) {
		ImGuiID dockspaceID = GetWindowDockID();

		ImGui::DockBuilderRemoveNode(dockspaceID);
		ImGui::DockBuilderAddNode(dockspaceID, ImGuiDockNodeFlags_DockSpace);
		ImGui::DockBuilderSetNodeSize(dockspaceID, ImGui::GetWindowSize());

		ImGuiID left, right;
		ImGui::DockBuilderSplitNode(dockspaceID, ImGuiDir_Left, 0.3f, &left, &right);

		ImGui::DockBuilderDockWindow(hierarchyPanel->GetPanelTitle().CStr(), left);
		ImGui::DockBuilderDockWindow(viewportPanel->GetPanelTitle().CStr(), right);
		ImGui::DockBuilderFinish(dockspaceID);
	}
}

void Plu::SkeletonViewport::OnUpdate(float deltaTime)
{
	if (BeginWindow()) {
		UpdatePanels(deltaTime);
	}
	EndWindow();
}
