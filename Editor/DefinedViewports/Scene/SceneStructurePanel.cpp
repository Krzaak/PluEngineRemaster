//
// Created by Plutex on 1/14/26.
//

#include "SceneStructurePanel.h"

#include "EditorAppContext.h"
#include "glm/gtc/type_ptr.hpp"
#include "Managers/Scene/EditorCamera.h"
#include "PluEngine/AssetTypes/StaticMesh/StaticMesh.h"
#include "PluEngine/GameObject/GameObject.h"
#include "PluEngine/Application.h"
#include "PluEngine/BasicEngineClasses/Components/StaticMeshComponent.h"
#include "PluEngine/Scenes/SceneManager.h"
#include "PluEngine/Managers/ScenesManager.h"
#include "PluEngine/Scenes/SceneWorld.h"
#include "PluEngine/Window/Window.h"
#include "UI/IconsFontAwesome7.h"

extern Plu::EditorAppContext* gEditorAppContext;
extern Plu::TUsePointer<Plu::EngineObjectManager> gEngineObjectManager;
extern Plu::ApplicationInfo* gApplicationInfo;

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
		TUsePointer<SceneInfo> scene = gApplicationInfo->AppAssetManager->GetAssetData(GetParentViewport()->GetAssetDescriptor());
		if (scene && gEditorAppContext->EditorScenesManager->IsAnySceneOpen())
		{
			TUsePointer<SceneWorld> sceneWorld = gEditorAppContext->EditorScenesManager->GetCurrentWorld();
			if (ImGui::BeginMenu(ICON_FA_PLUS " Spawn Game Object"))
			{
				static DynamicArray<TypeInfo*> componentTypes;
				if (componentTypes.IsEmpty()) {
					for (auto type : *TypeRegistry::GetInstance()->GetTypeMap()) {
						if (type.second->IsDerivedOfOrSame(GameObject::GetStaticClass())) {
							componentTypes.PushBack(type.second);
						}
					}
				}
				if (ImGui::Button("Refresh")) {
					componentTypes.Clear();
					for (auto type : *TypeRegistry::GetInstance()->GetTypeMap()) {
						if (type.second->IsDerivedOfOrSame(GameObject::GetStaticClass())) {
							componentTypes.PushBack(type.second);
						}
					}
				}
				ImGui::Separator();
				for (auto type : componentTypes) {
					if (ImGui::Button(type->TypeName.CStr())) {
						sceneWorld->SpawnGameObject(type);
					}
				}
				ImGui::EndMenu();
			}
			static DynamicArray<String> names;
			sceneWorld->GetFormattedGameObjectNames(&names);
			UInt64 numObjs = names.Size();
			if (ImGui::Shortcut(ImGuiMod_Ctrl + ImGuiKey_D)) {
				if (gEngineObjectManager->IsValid(gEditorAppContext->EditorState.SelectedGameObject)) {
					TUsePointer<GameObject> obj = gEngineObjectManager->GetObjectAsUser<GameObject>(gEditorAppContext->EditorState.SelectedGameObject);
					JSON j = TypeSerializer<TUsePointer<GameObject>>::Serialize(&obj);
					j["uuid"] = PluUUID().getUUID();
					gEditorAppContext->EditorScenesManager->LoadGameObjectFromJSON(gEditorAppContext->EditorScenesManager->GetCurrentWorld(), j);
				}
			}
			for (UInt64 i = 0; i < numObjs; ++i) {
				if (ImGui::Selectable(names[i].CStr(), *sceneWorld->GetAllGameObjects().At(i)->GetEngineObjectHandle() == gEditorAppContext->EditorState.SelectedGameObject)) {
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
						gEditorAppContext->EditorScenesManager->LoadGameObjectFromJSON(gEditorAppContext->EditorScenesManager->GetCurrentWorld(), j);
						ImGui::CloseCurrentPopup();
					}
					static int numTimesToDupe = 1;
					if (ImGui::Button("Duplicate N times")) {
						JSON j = TypeSerializer<TUsePointer<GameObject>>::Serialize(&sceneWorld->GetAllGameObjects().At(i));
						for (int n = 0; n < numTimesToDupe; ++n) {
							j["uuid"] = PluUUID().getUUID();
							gEditorAppContext->EditorScenesManager->LoadGameObjectFromJSON(gEditorAppContext->EditorScenesManager->GetCurrentWorld(), j);
						}
						ImGui::CloseCurrentPopup();
					}
					ImGui::SameLine();
					ImGui::DragInt("##NtimeToDupe", &numTimesToDupe);
					if (ImGui::Button("Fit In View")) {
						TUsePointer<GameObject> obj = gEngineObjectManager->GetObjectAsUser<GameObject>(gEditorAppContext->EditorState.SelectedGameObject);
						BoundingBox boundingBox = {{-0.1,0.1},{-0.1,0.1},{-0.1,0.1}};
						for (const auto& comp : *obj->GetObjectWorldComponents()) {
							if (comp->GetClass()->IsDerivedOfOrSame(StaticMeshComponent::GetStaticClass())) {
								BoundingBox newBox = CreateBoundingBoxForStaticMesh(DynamicCast<StaticMeshComponent>(comp)->GetStaticMesh().GetRaw());
								newBox = newBox.Multiply(DynamicCast<WorldComponent>(comp)->GetWorldScale());
								boundingBox = boundingBox.Add(newBox);
							}
						}
						Vec3 newLoc = boundingBox.FitCamera(obj->GetObjectLocation(),
							gEditorAppContext->EditorSceneCamera->GetCameraRotation(),
							Vec2(gApplicationInfo->AppWindow->GetWidth(), gApplicationInfo->AppWindow->GetHeight()),
							gEditorAppContext->EditorSceneCamera->GetCameraOptions()->FieldOfView
							);
						DynamicCast<EditorSceneCamera>(gEditorAppContext->EditorSceneCamera)->SetCameraLocation(newLoc);
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
