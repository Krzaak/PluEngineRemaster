//
// Created by Plutex on 2026-03-28.
//

#include "AssetBrowserPanel.h"

#include "UI/IconsFontAwesome7.h"

Plu::String Plu::AssetBrowserPanel::GetPanelName()
{
	return ICON_FA_FOLDER_CLOSED " Asset Browser";
}

void Plu::AssetBrowserPanel::OnUpdate(float deltaTime)
{
	constexpr int boxSize = 70;
	constexpr int numAssets = 20;
	if (BeginPanel()) {
		ImVec2 space = ImGui::GetContentRegionAvail();
		ImVec2 startPos = ImGui::GetCursorPos();
		ImGui::Text("%f, %f", space.x, space.y);
		int currentLine = 0;
		for (int i = 0; i < numAssets; ++i) {
			ImGui::PushID(i);
			if (ImGui::Button(String::FromInt(i).CStr(), ImVec2(boxSize, boxSize))) {

			}
			if (ImGui::GetContentRegionAvail().x > boxSize + ImGui::GetStyle().ItemSpacing.x) {
				ImGui::SameLine();
			}
			ImGui::PopID();
		}
	}
	EndPanel();
}

void Plu::AssetBrowserPanel::OnHide()
{
}

void Plu::AssetBrowserPanel::OnShow()
{
}
