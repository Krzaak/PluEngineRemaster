//
// Created by Plutex on 1/14/26.
//

#include "SceneObjectDetailsPanel.h"

#include "EditorAppContext.h"
#include "Managers/Assets/EditorAssetObject.h"
#include "Managers/Scene/EditorScenesManager.h"
#include "PluEngine/AssetTypes/StaticMesh/StaticMesh.h"
#include "PluEngine/GameObject/GameObject.h"
#include "PluEngine/GameObject/GameObjectComponent.h"
#include "PluEngine/GameObject/WorldComponent.h"
#include "PluEngine/Objects/EngineObjectManager.h"
#include "UI/IconsFontAwesome7.h"

extern Plu::EditorAppContext* gEditorAppContext;
extern Plu::TUsePointer<Plu::EngineObjectManager> gEngineObjectManager;

Plu::String Plu::SceneInspectorPanel::GetPanelName()
{
	return "Inspector";
}

void Plu::SceneInspectorPanel::OnClosed()
{
}

void Plu::SceneInspectorPanel::OnOpened()
{
}

void Plu::SceneInspectorPanel::OnUpdate(float deltaTime)
{
	if (BeginPanel())
	{
		EditorAssetObject<SceneInfo>* scene = dynamic_cast<EditorAssetObject<SceneInfo>*>(GetParentViewport()->GetAssetObject().GetRaw());
		if (scene && gEditorAppContext->EditorScenesManager->IsAnySceneOpen() && gEngineObjectManager->IsValid(gEditorAppContext->EditorState.SelectedGameObject))
		{
			TUsePointer<GameObject> gameObj = gEngineObjectManager->GetObjectAsUser<GameObject>(gEditorAppContext->EditorState.SelectedGameObject);

			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0,1,0,1));
			if (ImGui::BeginMenu(ICON_FA_PLUS "")) {
				//obj->AddComponent(WorldComponent::GetStaticClass());
				static DynamicArray<TypeInfo*> componentTypes;
				if (componentTypes.IsEmpty()) {
					for (auto type : *TypeRegistry::GetInstance()->GetTypeMap()) {
						if (type.second->IsDerivedOfOrSame(GameObjectComponent::GetStaticClass())) {
							componentTypes.PushBack(type.second);
						}
					}
				}
				for (auto type : componentTypes) {
					if (ImGui::Button(type->TypeName.CStr())) {
						gameObj->AddComponent(type);
					}
				}
				ImGui::EndMenu();
			}
			ImGui::PopStyleColor();

			//Component Tree
			for (auto worldComp : *gameObj->GetObjectWorldComponents()) {
				if (ImGui::Selectable(worldComp->GetDisplayName().CStr())) {
					gEditorAppContext->EditorState.SelectedGameObjectComponent = *worldComp->GetEngineObjectHandle();
				}
			}
			ImGui::Separator();
			for (auto comp : *gameObj->GetObjectComponents()) {
				if (ImGui::Selectable(comp->GetDisplayName().CStr())) {
					gEditorAppContext->EditorState.SelectedGameObjectComponent = *comp->GetEngineObjectHandle();
				}
			}
			ImGui::Separator();
			EngineObject* obj = nullptr;
			if (gEngineObjectManager->IsValid(gEditorAppContext->EditorState.SelectedGameObjectComponent)) {
				obj = gEngineObjectManager->GetObjectAsUser<EngineObject>(gEditorAppContext->EditorState.SelectedGameObjectComponent).GetRaw();
			} else {
				obj = gEngineObjectManager->GetObjectAsUser<EngineObject>(gEditorAppContext->EditorState.SelectedGameObject).GetRaw();
			}
			for (PropertyInfo* prop : obj->GetClass()->Properties)
			{
				prop->EditorControlPtr(prop->GetPtr(obj), prop->PropertyName);
			}
		}
	}
	EndPanel();
}
