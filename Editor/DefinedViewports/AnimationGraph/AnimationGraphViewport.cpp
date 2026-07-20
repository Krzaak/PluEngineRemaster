//
// Created by Plutex on 7/20/26.
//

#include "AnimationGraphViewport.h"

#include "AnimationGraphViewportPanel.h"
#include "AnimationGraphDetailsPanel.h"

void Plu::AnimationGraphViewport::OnClosed()
{
}

void Plu::AnimationGraphViewport::OnOpened()
{
	// Canvas first so it takes the larger (left) region; details dock to its right.
	AddPanel(AnimationGraphViewportPanel::GetStaticClass(), false);
	AddPanel(AnimationGraphDetailsPanel::GetStaticClass(), false);
}

void Plu::AnimationGraphViewport::OnPanelRegister()
{
	AnimationGraphViewportPanel* viewportPanel = GetPanelSlow<AnimationGraphViewportPanel>();
	AnimationGraphDetailsPanel* detailsPanel = GetPanelSlow<AnimationGraphDetailsPanel>();
	if (viewportPanel && detailsPanel) {
		ImGuiID dockspaceID = GetWindowDockID();

		ImGui::DockBuilderRemoveNode(dockspaceID);
		ImGui::DockBuilderAddNode(dockspaceID, ImGuiDockNodeFlags_DockSpace);
		ImGui::DockBuilderSetNodeSize(dockspaceID, ImGui::GetWindowSize());

		// Canvas on the left (~75%), details strip on the right.
		ImGuiID left, right;
		ImGui::DockBuilderSplitNode(dockspaceID, ImGuiDir_Right, 0.25f, &right, &left);

		ImGui::DockBuilderDockWindow(viewportPanel->GetPanelTitle().CStr(), left);
		ImGui::DockBuilderDockWindow(detailsPanel->GetPanelTitle().CStr(), right);
		ImGui::DockBuilderFinish(dockspaceID);
	}
}

void Plu::AnimationGraphViewport::OnUpdate(float deltaTime)
{
	if (BeginWindow()) {
		UpdatePanels(deltaTime);
	}
	EndWindow();
}
