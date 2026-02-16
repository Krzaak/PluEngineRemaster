//
// Created by Plutex on 2026-02-16.
//

#include "ProjectLauncherPanel.h"

#include "EditorAppContext.h"
#include "ImGuiFileDialog.h"
#include "PluEngine/PluPaths.h"
#include "UI/IconsFontAwesome7.h"

Plu::String Plu::ProjectLauncherPanel::GetPanelName()
{
	return ICON_FA_SIGNAL " Project Launcher";
}

void Plu::ProjectLauncherPanel::OnUpdate(float deltaTime)
{
	if (BeginPanel()) {
		ImVec2 avail = ImGui::GetContentRegionAvail();
		ImVec2 buttonSize = ImVec2(50, 50);
		ImGui::SetCursorPos(ImVec2((avail.x / 2) - (buttonSize.x / 2), (avail.y / 2) - (buttonSize.y / 2) ));
		if (ImGui::Button("New", buttonSize)) {
			mEditorAppContext->NewProjectPopup = true;
		}
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3,0.3,1,1));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1,1,1,1));
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0,0,0.8,1));
		ImGui::SameLine();
		if (ImGui::Button("Open", buttonSize)) {
			ImGuiFileDialog::Instance()->OpenDialog(
				"OpenProject",
				"Select project",
				PLU_PROJECT_EXT,
				IGFD::FileDialogConfig(".", "","", 1, IGFDUserDatas(), ImGuiFileDialogFlags_Modal)
			);
		}
		ImGui::PopStyleColor(3);
	}
	EndPanel();
}

void Plu::ProjectLauncherPanel::OnHide()
{
}

void Plu::ProjectLauncherPanel::OnShow()
{
}
