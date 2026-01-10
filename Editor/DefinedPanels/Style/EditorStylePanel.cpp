//
// Created by Plutex on 1/5/26.
//

#include "EditorStylePanel.h"
#include "Managers/Project/EditorProjectManager.h"
#include "EditorAppContext.h"
#include "json.hpp"
#include "json_fwd.hpp"
#include "detail/meta/std_fs.hpp"
#include "PluEngine/Managers/DiskManager.h"

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
	if (mEditorAppContext->EditorProjectManager->IsAnyProjectOpen()) {
		if (ImGui::Button("Save")) {
			isDirty = false;
			nlohmann::json config = {
				{"fontSize", fontSize}
			};
			StringW savePath = mEditorAppContext->EditorProjectManager->GetProjectConfigDirectory().ToString();
			savePath += L"EditorStyle.json";
			DiskManager::SaveJson(savePath, config);
		}
	}
	if (ImGui::DragFloat("Font Size", &fontSize)) {
		isDirty = true;
		ImGuiStyle& style = ImGui::GetStyle();
		style.FontSizeBase = fontSize;
		style._NextFrameFontSizeBase = style.FontSizeBase;
	}
	ImGui::End();
}
