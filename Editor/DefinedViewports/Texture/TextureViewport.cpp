//
// Created by Plutex on 1/13/26.
//

#include "TextureDisplayPanel.h"

#include "TextureViewport.h"

void Plu::TextureViewport::OnClosed()
{
}

void Plu::TextureViewport::OnOpened()
{
	AddPanel(TextureDisplayPanel::GetStaticClass(), false);
}

void Plu::TextureViewport::OnPanelRegister()
{
	TextureDisplayPanel* textureDisplayPanel = GetPanelSlow<TextureDisplayPanel>();
	if (textureDisplayPanel) {
		ImGuiID dockspaceID = GetWindowDockID();

		ImGui::DockBuilderRemoveNode(dockspaceID);
		ImGui::DockBuilderAddNode(dockspaceID, ImGuiDockNodeFlags_DockSpace);
		ImGui::DockBuilderSetNodeSize(dockspaceID, ImGui::GetWindowSize());

		ImGui::DockBuilderDockWindow(textureDisplayPanel->GetPanelTitle().CStr(), dockspaceID);
		ImGui::DockBuilderFinish(dockspaceID);
	}
}

void Plu::TextureViewport::OnUpdate(float deltaTime)
{
	if (BeginWindow()) {
		UpdatePanels(deltaTime);
	}
	EndWindow();
}
