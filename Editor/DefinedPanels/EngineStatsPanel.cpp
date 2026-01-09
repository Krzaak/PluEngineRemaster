//
// Created by Plutex on 1/1/26.
//

#include "EngineStatsPanel.h"

#include <iostream>

#include "PluEngine/Application.h"
#include "PluEngine/Objects/EngineObjectManager.h"
#include "PluEngine/Renderer/Renderer.h"
#include "UI/IconsFontAwesome7.h"

void Plu::EngineStatsPanel::OnUpdate(float deltaTime)
{
	ImGui::Begin(ICON_FA_GEARS " Engine Stats", nullptr);
	static bool showDemoWindow = false;
	ImGui::Checkbox("Demo Window", &showDemoWindow);
	if (showDemoWindow) {
		ImGui::ShowDemoWindow(&showDemoWindow);
	}
	static int numElements = 50;
	ImGui::DragInt("Num Elements to show", &numElements, 1, 0, mApplicationInfo->AppObjectManager->GetNumberOfObjects());
	for (String& obj: mApplicationInfo->AppObjectManager->GetObjectNames(numElements)) {
		if (ImGui::Selectable(obj.CStr())) {
			String clickInfo = "Click on ";
			clickInfo += obj;
			PLU_TRACE(clickInfo.CStr());
			std::cout << sizeof(wchar_t);
			std::cout << sizeof(char32_t);
			std::cout << sizeof(char16_t);
		}
	}
	ImGui::End();
}

void Plu::EngineStatsPanel::OnHide()
{
}

void Plu::EngineStatsPanel::OnShow()
{
}
