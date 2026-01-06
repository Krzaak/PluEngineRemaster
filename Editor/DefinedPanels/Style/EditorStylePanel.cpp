//
// Created by Plutex on 1/5/26.
//

#include "EditorStylePanel.h"

bool isDirty = false;
float fontSize = -1;

void Plu::EditorStylePanel::OnHide()
{
}

void Plu::EditorStylePanel::OnShow()
{
	fontSize = 13; //Load from somewhere
}

void Plu::EditorStylePanel::OnUpdate(float deltaTime)
{
	ImGui::Begin("Style Editor", nullptr, isDirty ? ImGuiWindowFlags_UnsavedDocument : 0);
	if (ImGui::Button("Save")) {
		isDirty = false;
	}
	if (ImGui::DragFloat("Font Size", &fontSize)) {
		isDirty = true;
		ImGuiStyle& style = ImGui::GetStyle();
		style.FontSizeBase = fontSize;
		style._NextFrameFontSizeBase = style.FontSizeBase;
	}
	ImGui::End();
}
