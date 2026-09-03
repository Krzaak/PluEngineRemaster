//
// Created by Plutex on 1/5/26.
//

#include "EditorStylePanel.h"
#include "Managers/Project/EditorProjectManager.h"
#include "EditorAppContext.h"
#include "json.hpp"
#include "json_fwd.hpp"
#include "detail/meta/std_fs.hpp"
#include "PluEngine/Core/DiskManager.h"

bool isDirty = false;
float fontSize = -1;

Plu::String Plu::EditorStylePanel::GetPanelName()
{
	return "Style Editor";
}

void Plu::EditorStylePanel::OnHide()
{
}

void Plu::EditorStylePanel::OnShow()
{
	SetCanClose(true);
	fontSize = 13; //Load from somewhere
}

void Plu::EditorStylePanel::OnUpdate(float deltaTime)
{
	if (BeginPanel()) {
		if (mEditorAppContext->EditorProjectManager->IsAnyProjectOpen()) {
			if (ImGui::Button("Save")) {
				isDirty = false;
				nlohmann::json config = {
					{"fontSize", fontSize}
				};
				StringW savePath = mEditorAppContext->EditorProjectManager->GetProjectConfigDirectory().ToString();
				savePath += L"/EditorStyle.json";
				DiskManager::SaveJson(savePath, config);
			}
		}
		if (ImGui::DragFloat("Font Size", &fontSize)) {
			isDirty = true;
			ImGuiStyle& style = ImGui::GetStyle();
			style.FontSizeBase = fontSize;
			style._NextFrameFontSizeBase = style.FontSizeBase;
		}
	}
	EndPanel();
}
