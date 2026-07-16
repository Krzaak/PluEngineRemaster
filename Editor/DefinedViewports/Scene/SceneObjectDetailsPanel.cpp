//
// Created by Plutex on 1/14/26.
//

#include "SceneObjectDetailsPanel.h"

#include "EditorAppContext.h"
#include "glm/gtc/type_ptr.hpp"
#include "PluEngine/AssetTypes/StaticMesh/StaticMesh.h"
#include "PluEngine/GameObject/GameObject.h"
#include "PluEngine/GameObject/GameObjectComponent.h"
#include "PluEngine/GameObject/WorldComponent.h"
#include "PluEngine/Objects/EngineObjectManager.h"
#include "PluEngine/Scenes/SceneManager.h"
#include "UI/IconsFontAwesome7.h"
#include "Utils/RGBTransformDragger.h"
#include "PluEngine/Managers/ScenesManager.h"

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

void GatherParents(Plu::TypeInfo* typeInfo, DynamicArray<Plu::TypeInfo*>* parents)
{
	if (!typeInfo) return;
	parents->PushBack(typeInfo);
	GatherParents(typeInfo->BaseType, parents);
}

void WorldComponentTree(const Plu::TUsePointer<Plu::WorldComponent>& component)
{
	ImGuiTreeNodeFlags flags = 0;
	if (component->GetChildren().IsEmpty()) {
		flags = ImGuiTreeNodeFlags_Leaf;
	}
	if (ImGui::TreeNodeEx(std::format("{} ({})", component->GetComponentName().CStr(), component->GetClass()->TypeName.CStr()).c_str(), flags)) {
		if (ImGui::IsItemClicked()) {
			gEditorAppContext->EditorState.SelectedGameObjectComponent = *component->GetEngineObjectHandle();
		}
		for (const auto& comp : component->GetChildren()) {
			WorldComponentTree(comp);
		}
		ImGui::TreePop();
	}
}

void Plu::SceneInspectorPanel::OnUpdate(float deltaTime)
{
	if (BeginPanel())
	{
		TUsePointer<SceneInfo> scene = gEditorAppContext->EditorAssetManager->GetAssetData(GetParentViewport()->GetAssetDescriptor());
		if (scene && gEditorAppContext->EditorScenesManager->IsAnySceneOpen() && gEngineObjectManager->IsValid(gEditorAppContext->EditorState.SelectedGameObject))
		{
			TUsePointer<GameObject> gameObj = gEngineObjectManager->GetObjectAsUser<GameObject>(gEditorAppContext->EditorState.SelectedGameObject);

			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0,1,0,1));
			if (ImGui::BeginMenu(ICON_FA_PLUS "")) {
				//obj->AddComponent(WorldComponent::GetStaticClass());
				static DynamicArray<TypeInfo*> componentTypes;
				if (componentTypes.IsEmpty()) {
					for (auto type : *TypeRegistry::GetInstance()->GetTypeMap()) {
						if (type.second->IsDerivedOfOrSame(GameObjectComponent::GetStaticClass()) && !type.second->IsAbstract) {
							componentTypes.PushBack(type.second);
						}
					}
				}
				for (auto type : componentTypes) {
					if (ImGui::Button(type->TypeName.CStr())) {
						gameObj->AddComponent(type, type->TypeName + "New");
						if (!gEditorAppContext->EditorScenesManager->IsInPIE()) {
							PanelChangedAsset();
						}
					}
				}
				ImGui::EndMenu();
			}
			ImGui::PopStyleColor();

			//Component Tree
			for (const auto& worldComp : gameObj->GetDirectlyAttachedWorldComponents()) {
				WorldComponentTree(worldComp);
				// if (ImGui::Selectable(std::format("{} ({})", worldComp->GetComponentName().CStr(), worldComp->GetClass()->TypeName.CStr()).c_str())) {
				// 	gEditorAppContext->EditorState.SelectedGameObjectComponent = *worldComp->GetEngineObjectHandle();
				// }
			}
			ImGui::Separator();
			for (const auto& comp : *gameObj->GetObjectComponents()) {
				if (ImGui::Selectable(std::format("{} ({})", comp->GetComponentName().CStr(), comp->GetClass()->TypeName.CStr()).c_str())) {
					gEditorAppContext->EditorState.SelectedGameObjectComponent = *comp->GetEngineObjectHandle();
				}
			}
			ImGui::Separator();
			EngineObject* obj = nullptr;
			if (gEngineObjectManager->IsValid(gEditorAppContext->EditorState.SelectedGameObjectComponent)) {
				obj = gEngineObjectManager->GetObjectAsUser<EngineObject>(gEditorAppContext->EditorState.SelectedGameObjectComponent).GetRaw();
				if (obj->GetClass()->IsDerivedOf(WorldComponent::GetStaticClass())) {
					Vec3 location = dynamic_cast<WorldComponent*>(obj)->GetRelativeLocation();
					if (RGBTransformDrag3("R Location", glm::value_ptr(location), 3, 0.1f,nullptr,nullptr,"%.3f",0))
					{
						dynamic_cast<WorldComponent*>(obj)->SetRelativeLocation(location);
						if (!gEditorAppContext->EditorScenesManager->IsInPIE()) {
							PanelChangedAsset();
						}
					}
					Vec3 rotation = dynamic_cast<WorldComponent*>(obj)->GetRelativeRotation();
					if (RGBTransformDrag3("R Rotation", glm::value_ptr(rotation), 3, 0.1f,nullptr,nullptr,"%.3f",0))
					{
						dynamic_cast<WorldComponent*>(obj)->SetRelativeRotation(rotation);
						if (!gEditorAppContext->EditorScenesManager->IsInPIE()) {
							PanelChangedAsset();
						}
					}
					Vec3 scale = dynamic_cast<WorldComponent*>(obj)->GetRelativeScale();
					if (RGBTransformDrag3("R Scale", glm::value_ptr(scale), 3, 0.1f,nullptr,nullptr,"%.3f",0))
					{
						dynamic_cast<WorldComponent*>(obj)->SetRelativeScale(scale);
						if (!gEditorAppContext->EditorScenesManager->IsInPIE()) {
							PanelChangedAsset();
						}
					}
				}
			} else {
				obj = gEngineObjectManager->GetObjectAsUser<EngineObject>(gEditorAppContext->EditorState.SelectedGameObject).GetRaw();
				Vec3 location = dynamic_cast<GameObject*>(obj)->GetObjectLocation();
				if (RGBTransformDrag3("Location", glm::value_ptr(location), 3, 0.1f,nullptr,nullptr,"%.3f",0))
				{
					dynamic_cast<GameObject*>(obj)->SetObjectLocation(location);
					if (!gEditorAppContext->EditorScenesManager->IsInPIE()) {
						PanelChangedAsset();
					}
				}
				Vec3 rotation = dynamic_cast<GameObject*>(obj)->GetObjectRotation();
				if (RGBTransformDrag3("Rotation", glm::value_ptr(rotation), 3, 0.1f,nullptr,nullptr,"%.3f",0))
				{
					dynamic_cast<GameObject*>(obj)->SetObjectRotation(rotation);
					if (!gEditorAppContext->EditorScenesManager->IsInPIE()) {
						PanelChangedAsset();
					}
				}
				Vec3 scale = dynamic_cast<GameObject*>(obj)->GetObjectScale();
				if (RGBTransformDrag3("Scale", glm::value_ptr(scale), 3, 0.1f,nullptr,nullptr,"%.3f",0))
				{
					dynamic_cast<GameObject*>(obj)->SetObjectScale(scale);
					if (!gEditorAppContext->EditorScenesManager->IsInPIE()) {
						PanelChangedAsset();
					}
				}
			}
			if (TypeSerializer<TypeInfo*>::EditorControl(obj->GetClass(), obj) && !gEditorAppContext->EditorScenesManager->IsInPIE()) {
				PanelChangedAsset();
			}
		}
	}
	EndPanel();
}
