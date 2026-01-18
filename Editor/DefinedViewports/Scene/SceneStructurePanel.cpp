//
// Created by Plutex on 1/14/26.
//

#include "SceneStructurePanel.h"

#include "EditorAppContext.h"
#include "Managers/Assets/EditorAssetObject.h"
#include "Managers/Scene/EditorScenesManager.h"
#include "PluEngine/AssetTypes/StaticMesh/StaticMesh.h"
#include "PluEngine/GameObject/GameObject.h"
#include "UI/IconsFontAwesome7.h"

extern Plu::EditorAppContext* gEditorAppContext;

Plu::String Plu::SceneStructurePanel::GetPanelName()
{
	return "Structure";
}

void Plu::SceneStructurePanel::OnClosed()
{
}

void Plu::SceneStructurePanel::OnOpened()
{
}

void Plu::SceneStructurePanel::OnUpdate(float deltaTime)
{
	if (BeginPanel())
	{
		EditorAssetObject<SceneInfo>* scene = dynamic_cast<EditorAssetObject<SceneInfo>*>(GetParentViewport()->GetAssetObject().GetRaw());
		if (scene && gEditorAppContext->EditorScenesManager->IsAnySceneOpen())
		{
			TUsePointer<SceneWorld> sceneWorld = gEditorAppContext->EditorScenesManager->GetCurrentEditorScene();
			if (ImGui::BeginMenu(ICON_FA_PLUS " Spawn Game Object"))
			{
				if (ImGui::Button("Empty Object")) {
					sceneWorld->SpawnGameObject(GameObject::GetStaticClass());
				}
				ImGui::EndMenu();
			}
			static DynamicArray<String> names;
			sceneWorld->GetFormattedGameObjectNames(&names);
			for (auto obj : names) {
				if (ImGui::Selectable(obj.CStr())) {
					PLU_TRACE("Click on {}", obj.CStr());
				}
			}
		}
	}
	EndPanel();
}
