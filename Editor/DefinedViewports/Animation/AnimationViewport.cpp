//
// Created by Plutex on 7/7/26.
//

#include "AnimationViewport.h"

#include "AnimationTimelinePanel.h"

void Plu::AnimationViewport::OnOpened()
{
	AddPanel(AnimationTimelinePanel::GetStaticClass(), false);
}

void Plu::AnimationViewport::OnClosed()
{
}

void Plu::AnimationViewport::OnPanelRegister()
{
	// Single panel — just fill the dockspace with it.
	AnimationTimelinePanel* timelinePanel = GetPanelSlow<AnimationTimelinePanel>();
	if (timelinePanel) {
		ImGuiID dockspaceID = GetWindowDockID();

		ImGui::DockBuilderRemoveNode(dockspaceID);
		ImGui::DockBuilderAddNode(dockspaceID, ImGuiDockNodeFlags_DockSpace);
		ImGui::DockBuilderSetNodeSize(dockspaceID, ImGui::GetWindowSize());
		ImGui::DockBuilderDockWindow(timelinePanel->GetPanelTitle().CStr(), dockspaceID);
		ImGui::DockBuilderFinish(dockspaceID);
	}
}

void Plu::AnimationViewport::OnUpdate(float deltaTime)
{
	if (BeginWindow()) {
		UpdatePanels(deltaTime);
	}
	EndWindow();
}
