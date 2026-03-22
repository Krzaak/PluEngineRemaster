//
// Created by Plutex on 2026-02-18.
//

#include "InputViewerPanel.h"

#include "PluEngine/Application.h"
#include "PluEngine/Input/InputInfo.h"
#include "PluEngine/Input/InputManager.h"
#include "UI/IconsFontAwesome7.h"

Plu::String Plu::InputViewerPanel::GetPanelName()
{
	return ICON_FA_EYE" Input Viewer";
}

void Plu::InputViewerPanel::OnUpdate(float deltaTime)
{
	if (BeginPanel()) {
		static DynamicArray<Key> keysToShow;
		//TODO Searcher
		for (auto key : keysToShow) {
			ImGui::Text("Key %d is %b", static_cast<int>(key), mApplicationInfo->AppInputManager->GetInputBackend()->GetKeyboard().IsDown(key));
		}
	}
	EndPanel();
}

void Plu::InputViewerPanel::OnHide()
{
}

void Plu::InputViewerPanel::OnShow()
{
}
