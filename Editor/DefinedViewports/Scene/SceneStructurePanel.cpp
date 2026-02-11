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
			UInt64 numObjs = names.Size();
			for (UInt64 i = 0; i < numObjs; ++i) {
				if (ImGui::Selectable(names[i].CStr())) {
					gEditorAppContext->EditorState.SelectedGameObject = *sceneWorld->GetAllGameObjects().At(i)->GetEngineObjectHandle();
					gEditorAppContext->EditorState.SelectedGameObjectComponent = EngineObjectHandle();
				}
				if (ImGui::BeginPopupContextItem()) // <-- use last item id as popup id
				{
					gEditorAppContext->EditorState.SelectedGameObject = *sceneWorld->GetAllGameObjects().At(i)->GetEngineObjectHandle();
					gEditorAppContext->EditorState.SelectedGameObjectComponent = EngineObjectHandle();
					ImGui::SetNextItemShortcut(ImGuiMod_Ctrl | ImGuiKey_D);
					if (ImGui::Button("Duplicate")) {
						JSON j = TypeSerializer<TUsePointer<GameObject>>::Serialize(&sceneWorld->GetAllGameObjects().At(i));
						j["uuid"] = PluUUID().getUUID();
						gEditorAppContext->EditorScenesManager->LoadGameObjectFromJSON(gEditorAppContext->EditorScenesManager->GetCurrentEditorScene(), j);
						ImGui::CloseCurrentPopup();
					}
					ImGui::Separator();
					if (ImGui::Button("Close"))
						ImGui::CloseCurrentPopup();
					ImGui::EndPopup();
				}
			}
		}
	}
	EndPanel();
}
