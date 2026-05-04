//
// Created by Plutex on 1/1/26.
//

#include "EngineStatsPanel.h"

#include <iostream>

#include "EditorAppContext.h"
#include "TextureViewerPanel.h"
#include "Managers/Project/EditorProjectManager.h"
#include "Panels/EditorPanelManager.h"
#include "PluEngine/Application.h"
#include "PluEngine/PluUtils.h"
#include "PluEngine/Managers/DiskManager.h"
#include "PluEngine/Objects/EngineObjectManager.h"
#include "PluEngine/Renderer/Renderer.h"
#include "UI/IconsFontAwesome7.h"
#include "PluEngine/Reflection/TypeTraits.h"

Plu::String Plu::EngineStatsPanel::GetPanelName()
{
	return ICON_FA_GEARS " Engine Stats";
}

void Plu::EngineStatsPanel::OnUpdate(float deltaTime)
{
	if (BeginPanel()) {
		static bool showDemoWindow = false;
		static String buildInfo = "Build Info: " + String(PLU_BUILD_TIME);
		ImGui::Text("%s",buildInfo.CStr());
		ImGui::Checkbox("Demo Window", &showDemoWindow);
		if (showDemoWindow) {
			ImGui::ShowDemoWindow(&showDemoWindow);
		}
		if (ImGui::Button("Log executable path")) {
			PLU_ERROR("{}", GetExePath().ToString().ToNarrow().CStr());
		}
		if (ImGui::Button("Log User path")) {
			PLU_ERROR("{}", GetSystemUserPath().ToString().CStr());
		}
		static int numElements = 50;
		ImGui::DragInt("Num Elements to show", &numElements, 1, 0, mApplicationInfo->AppObjectManager->GetNumberOfObjects());
		DynamicArray<String> names = mApplicationInfo->AppObjectManager->GetObjectNames(numElements);
		for (UInt32 i = 0; i < names.Size(); i++) {
			String* obj = &names[i];
			if (ImGui::Selectable(obj->CStr())) {
				String clickInfo = "Click on ";
				clickInfo += *obj;
				PLU_TRACE(clickInfo.CStr());
			}
			if (ImGui::BeginPopupContextItem()) {
				if (mApplicationInfo->AppObjectManager->GetObjectOnIndex(i)->GetClass()->IsDerivedOfOrSame(Texture::GetStaticClass())) {
					if (ImGui::MenuItem("View Texture")) {
						TUsePointer<TextureViewerPanel> viewer = mEditorPanelManager->AddPanel<TextureViewerPanel>();
						viewer->TextureToView = mApplicationInfo->AppObjectManager->GetObjectOnIndex(i);
					}
				}
				if (ImGui::MenuItem("Serialize")) {
					nlohmann::json json;
					json = TypeSerializer<TypeInfo*>::Serialize(mApplicationInfo->AppObjectManager->GetObjectOnIndex(i)->GetClass(), mApplicationInfo->AppObjectManager->GetObjectOnIndex(i).GetRaw());
					String fileName = mApplicationInfo->AppObjectManager->GetObjectOnIndex(i)->GetDisplayName() + ".json";
					std::filesystem::create_directories((mEditorAppContext->EditorProjectManager->GetProjectCacheDirectory().ToString() + L"/SerializerTests/").CStr());
					DiskManager::SaveJson(mEditorAppContext->EditorProjectManager->GetProjectCacheDirectory().ToString() + L"/SerializerTests/" + fileName.ToWide(), json);
					PLU_WARN("Saved TestSerialize Test to: {}", (mEditorAppContext->EditorProjectManager->GetProjectCacheDirectory().ToString().ToNarrow() + "/SerializerTests/" + fileName).CStr());
				}
				ImGui::EndPopup();
			}
		}
	}
	EndPanel();
}

void Plu::EngineStatsPanel::OnHide()
{
}

void Plu::EngineStatsPanel::OnShow()
{
	SetCanClose(true);
}
